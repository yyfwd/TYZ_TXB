
#ifndef __FUNTION_H
#define __FUNTION_H


typedef struct  Scpi_code02{ //定义一个结构体
	char code1[8][20];
}SCPI2;

extern  int scpi_check_id;

extern int USB_ReadCount;
extern char *USB_ReadBuf;
//打印系统时间
void printf_curr_time();

void USB_SCPI_Read(void);

#endif

