#ifndef SERIAL_H_INCLUDED
#define SERIAL_H_INCLUDED

#define SERIAL2   	"/dev/ttyO2"
#define SERIAL3 	"/dev/ttyO3"
#define SERIAL4 	"/dev/ttyO4"
#define	SERIAL5		"/dev/ttyO5"

typedef struct serial_arg
{
	int fd;
	int ser_num;
	char recv_buf[1024];
	char send_buf[1024];
}serial_arg;

int open_dev(char *dev);
int set_speed(int fd, int speed);
int set_parity(int fd, int databits, int stopbits, int parity);
void *recv_date(void *args);
void serial4_LCD_open();
void serial5_Finger_open();

#endif // SERIAL_H_INCLUDED
