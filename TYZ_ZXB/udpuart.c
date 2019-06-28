#include"udpuart.h"

int speed_arr[] = { B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
		B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = { 115200, 38400, 19200, 9600, 4800, 2400, 1200, 300, 38400,
		19200, 9600, 4800, 2400, 1200, 300, };

void set_speed(int fd, int speed) {
	int i;
	int status;
	struct termios Opt;
	tcgetattr(fd, &Opt);
	for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) {
		if (speed == name_arr[i]) {
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);
			
			if (status != 0) {
				printf("set status failed\n");
				perror("tcsetattr fd");
				return;
			}
			printf("serial set speed success\n");
			tcflush(fd, TCIOFLUSH);
		}
	}
}

int set_Parity(int fd, int databits, int stopbits, int parity) {
	struct termios options;
	if (tcgetattr(fd, &options) != 0) {
		perror("SetupSerial 1");
		return (ERROR);
	}
	options.c_cflag &= ~CSIZE;
	options.c_iflag &= ~INPCK;
	options.c_iflag |= IGNBRK;
	options.c_iflag &= ~ICRNL;
	options.c_iflag &= ~IXON;
	options.c_lflag &= ~IEXTEN;
	options.c_lflag &= ~ECHOK;
	options.c_lflag &= ~ECHOCTL;
	options.c_lflag &= ~ECHOKE;
	options.c_oflag &= ~ONLCR;
	switch (databits) {
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		return (ERROR);
	}
	switch (parity) {
	case 'n':
	case 'N':
		//options.c_cflag &= ~PARENB;   /* Clear parity enable */
		//options.c_iflag &= ~INPCK;     /* Enable parity checking */
		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		options.c_oflag &= ~OPOST; /*Output*/
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB); /* Å¡Å Å¡Å¡???a??DÂ¡Ã¬?Å¡Å */
		options.c_iflag |= INPCK; /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB; /* Enable parity */
		options.c_cflag &= ~PARODD; /* Â¡Ãa???a??DÂ¡Ã¬?Å¡Å */
		options.c_iflag |= INPCK; /* Disnable parity checking */
		break;
	case 'S':
	case 's': /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
//		fprintf(stderr,"Unsupported parity\n");    
		return (ERROR);
	}
	switch (stopbits) {
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
//		 	fprintf(stderr,"Unsupported stop bits\n");
		return (ERROR);
	}

	/* Set input parity option */

	if ((parity != 'n') && (parity != 'N'))
		options.c_iflag |= INPCK;
	tcflush(fd, TCIFLUSH);
	options.c_cc[VTIME] = 250; //设置最小时间
	options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
	if (tcsetattr(fd, TCSANOW, &options) != 0) {
		//perror("SetupSerial 3");
		return (ERROR);
	}
	return 0;
}

int OpenDev(char *Dev) {
	int fd = open(Dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (-1 == fd) {

		perror("Can't Open Serial Port");
		return -1;
	}

	else {
		//printf("opendev success------fd:%d\n", fd);
		return fd;
	}
}

int udp_broadcast_service(unsigned char *change) {
	int castsockfd, connfd;
	int optval;
	int client_len;
	int revc;
	struct sockaddr_in serv_addr, client_addr;
	if ((castsockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("sockfd error!\n");
		exit(1);
	}
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(BROADCASTPORT);
	serv_addr.sin_addr.s_addr = htons(INADDR_ANY );
	fcntl(castsockfd, F_SETFL, O_NONBLOCK);
	if (bind(castsockfd, (struct sockaddr *) &serv_addr,
			sizeof(struct sockaddr)) < 0) {
		perror("bind failed!\n");
		exit(1);
	}
	printf("bind success..........\n");

	client_len = sizeof(struct sockaddr_in);
	while ((revc = recvfrom(castsockfd, change, BUF_SIZE, 0,
			(struct sockaddr *) &client_addr, &client_len)) <= 0) {
		if (getppid() == 1)
			exit(1);
	}

	sendto(castsockfd, change, BUF_SIZE, 0, (struct sockaddr *) &client_addr,
			client_len);
	close(castsockfd);
	return revc;
}

int send_broadcast(unsigned char *send_buf, int t) {
	int client_sockfd;
	int optval;
	struct sockaddr_in serv_addr;
	if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("sockfd error!\n");
		exit(1);
	}
	printf("client socket create success:%d\n", client_sockfd);
	optval = 1;

	if (setsockopt(client_sockfd, SOL_SOCKET, SO_BROADCAST, (void *) &optval,
			sizeof(int)) < 0) {
		perror("setsockopt error!\n");
		exit(1);
	}
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = inet_addr("192.168.123.180");

	if (sendto(client_sockfd, send_buf, t, 0, (struct sockaddr *) &serv_addr,
			sizeof(struct sockaddr)) < 0) {
		perror("send failed!\n");
		exit(1);
	}
	printf("send data success:%s\n", send_buf);
	close(client_sockfd);
	return 0;
}

void do_echo(void) {
	int n;
	socklen_t len;
	char mesg[80];
	printf("hello,join the server\n");
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	printf("create socket success:%d\n", sockfd);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY );
	servaddr.sin_port = htons(SERV_PORT);

	if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
		perror("bind error");
		exit(1);
	}

	while (1) {
		len = sizeof(cliaddr);
		n = recvfrom(sockfd, mesg, 80, 0, (struct sockaddr *) &cliaddr, &len);
		mesg[n] = '\0';
		printf("recv mesg:%s  -length:%d\n", mesg, n);

		sendto(sockfd, mesg, n + 1, 0, (struct sockaddr *) &cliaddr, len);
	}
}

/*int main()
 {
 pid_t pid,id;
 int fd;
 int nread,nwrite;
 int i=0,sum=0,recv_c=0;
 unsigned char readbuff[100];
 unsigned char buff[100];
 unsigned char check[100];
 char *dev  = "/dev/ttyO1";
 printf("go in server!\n");
 fd = OpenDev(dev);

 printf("opendev id:%d\n",fd);

 set_speed(fd,115200);

 printf("uart speed set success\n");
 if (set_Parity(fd,8,1,'N') == ERROR)
 {
 exit (1);
 }
 pid=fork();              //create jincheng
 if(pid<0)
 {
 exit(1);
 }

 if(pid==0)
 {
 printf( "child pid:%d\n",pid);
 while(1)
 {
 recv_c=udp_broadcast_service(check);
 while((nwrite=write(fd,check,recv_c))>0)
 {
 check[nwrite] = '\0';
 break;
 }
 }
 }
 else
 {
 printf( "parent pid:%d\n",pid);
 while(1)
 {
 tcflush(fd, TCIOFLUSH);
 bzero(readbuff,100);
 bzero(buff,100);
 while((nread = read(fd, buff,1))<=0)
 {

 id=waitpid(pid,NULL,WNOHANG);
 if(id==pid)
 {
 //                             printf("child exit\n");
 exit(1);
 }
 usleep(1000);
 }
 sum=1;
 nread=0;
 usleep(5000);
 //                printf( "parent\n");
 while((nread = read(fd, readbuff,8))>0)
 {

 for(i=0;i<nread;i++)
 {
 buff[sum+i]=readbuff[i];
 }
 sum=nread+sum;
 nread=0;
 bzero(readbuff,100);
 usleep(10000);
 //			printf( "sum=%d\n", sum);
 }
 printf( "buff=%s\n", buff);
 send_broadcast(buff,sum);
 bzero(buff,100);
 tcflush(fd, TCIOFLUSH);
 sum=0;

 }

 }

 close(fd);

 }*/

