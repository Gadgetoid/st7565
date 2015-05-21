all:
	gcc -o main -lwiringPi -lpthread -levent -levent_pthreads main.c
