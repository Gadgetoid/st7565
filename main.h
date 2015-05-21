int main(int argc, char *argv[]);
void lcd_clear();
void lcd_set_page(unsigned char page, unsigned char column);
void lcd_reset();
void lcd_display(unsigned char col);
void lcd_command(unsigned char *command, int len);
void lcd_data(unsigned char *data, int len);
void lcd_send(unsigned char *values, int len);
