#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include "main.h"

#define SPI_SPEED_HZ 2500000

#define WIDTH       128
#define HEIGHT      64
#define PAGE_SIZE   8
#define BUF_SIZE    (WIDTH*PAGE_SIZE*2) // 2 bits per pixel

#define PIN_CS      22
#define PIN_RST     17
#define PIN_A       27

#define FRAME_TIME 9500   // Microseconds

#define CMD_SET_PAGE 0xb0
#define CMD_SET_COLUMN_UPPER 0x10
#define CMD_SET_COLUMN_LOWER 0x00
#define ST7565_STARTBYTES 1

int frame_time = FRAME_TIME;
unsigned char buf[BUF_SIZE];
int running = 0;
pthread_t pt_redraw;

void *redraw_loop(void *param){
  running = 1;

  int col = 0;
  struct timeval t_start, t_end;
  double msec;
 
  while( running ){

    for(col = 0; col < 3; col++){
        gettimeofday(&t_start, NULL);
        lcd_display(col);
        gettimeofday(&t_end, NULL);
        msec = msec = (double)(t_end.tv_usec - t_start.tv_usec) / 1000000 + (double)(t_end.tv_sec - t_start.tv_sec);
        delayMicroseconds(frame_time - msec);
        //delayMicroseconds(1000000);
    }

  }
  return NULL;
}

void sigintHandler(int sig_num){
  running = 0;
  pthread_join(pt_redraw, NULL);
  exit(1);
}

int main(int argc, char *argv[]){
  if( argc > 1 ){
    sscanf(argv[1], "%i", &frame_time);
  }

  signal(SIGINT, sigintHandler);
  wiringPiSetupGpio();
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
  pinMode(PIN_A, OUTPUT);
  //digitalWrite(PIN_CS, LOW);
  memset(buf, 0, BUF_SIZE);
  if(wiringPiSPISetup(0, SPI_SPEED_HZ)==-1){
    printf("Could not initialize SPI\n");
    return;
  };
  lcd_reset();
  
  lcd_command((unsigned char[8]){
	0xA2,
	0xA0,
	0xC8,
	0xA4,
	0xA6,
	0x2F,
	0x60,
	0x27
		},8);
  lcd_command((unsigned char[2]){0x81, 0x2A},2);
  lcd_command((unsigned char[1]){0xAF},1);
  lcd_clear();
  char * lcd_fifo = "/tmp/st7565";
  mkfifo(lcd_fifo, 0777);
  //int fd = open("/dev/urandom", O_RDONLY);
  int fd = open(lcd_fifo, O_RDONLY);
 /*while(read(fd, buf, BUF_SIZE) == 0){

  }*/
  pthread_create(&pt_redraw, NULL, redraw_loop, NULL);
  while(1){
    read(fd, buf, BUF_SIZE);
    /*memset(buf, 0b11000000, BUF_SIZE);
    lcd_display();
    memset(buf, 0b11110000, BUF_SIZE);
    lcd_display();
    memset(buf, 0b11111100, BUF_SIZE);
    lcd_display();
    memset(buf, 0b11111111, BUF_SIZE);*/
  }
  running = 0;
  pthread_join(pt_redraw, NULL);
  close(fd);
  unlink(lcd_fifo);
  return 0;
};

void lcd_clear(){
  unsigned char page = 0;
  for(page = 0;page < 8;page++){
    lcd_set_page(page, 0);
    //int offset = page*128;
    unsigned char t[128];
    memset(t, 0, 128);
    lcd_data(t, 128);
    //memcpy( t, buf + offset, 128 ); 
    //free(t);
  }
}

void lcd_display(unsigned char col){
  unsigned char page = 0;
  for(page = 0; page < 8; page++){
    lcd_set_page(page, 0);
    int offset = page*128*2;
    unsigned char t[128];
    int x = 0;
    int y = 0;
    for( x = 0; x < 128*2; x+=2){
      unsigned char bufA = buf[offset + x];
      unsigned char bufB = buf[offset + x + 1];
      
      if(col == 2){ // White, 11, 10, 01
         t[y] = bufA | bufB;
      }
      if(col == 1){ // Light Grey, 10, 01
         t[y] = bufA ^ bufB; 
      }
      if(col == 0){ // Dark Grey, 01
         t[y] = bufB & (bufA ^ bufB);
      }

      y++;
    }
    lcd_data(t, 128);
    //free(t);
  }
}

void lcd_set_page(unsigned char page, unsigned char column){
  lcd_command((unsigned char[3]){CMD_SET_PAGE | page,
		CMD_SET_COLUMN_LOWER | ((column+ST7565_STARTBYTES) & 0xf),
		CMD_SET_COLUMN_UPPER | (((column+ST7565_STARTBYTES) >> 4) & 0x0f)}, 3);
}

void lcd_reset(){
  digitalWrite(PIN_RST, LOW);
  digitalWrite(PIN_RST, HIGH);
  lcd_command((unsigned char[1]){0xe2}, 1);
}

void lcd_command(unsigned char *command, int len){
  digitalWrite(PIN_A, LOW);
  lcd_send(command, len);
}

void lcd_data(unsigned char *data, int len){
  digitalWrite(PIN_A, HIGH);
  lcd_send(data, len);
}

void lcd_send(unsigned char *values, int len){
  digitalWrite(PIN_CS, LOW);
  wiringPiSPIDataRW(0, values, len);
  digitalWrite(PIN_CS, HIGH);
}
