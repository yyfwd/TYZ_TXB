#ifndef _UDPUART_H_
#define _UDPUART_H_

#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>   
#include<unistd.h>    
#include<sys/stat.h>   
#include<fcntl.h>     
#include<termios.h>  
#include<sys/wait.h> 

#define PORT 9780                 
#define BROADCASTPORT 9760         
#define BUF_SIZE 100               
#define ERROR  -1
#define SERV_PORT 8888
//#define TRUE   0

void do_echo(void);
void set_speed(int fd, int speed);
int set_Parity(int fd,int databits,int stopbits,int parity);
int OpenDev(char *Dev);
void socket_init();
int udp_broadcast_service(unsigned char *change);
int send_broadcast(unsigned char *send_buf,int t);
int udp_broadcast_client(unsigned char *change,int t);

#endif  
