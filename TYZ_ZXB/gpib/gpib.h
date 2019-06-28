#ifndef _GPIB_H_
#define _GPIB_H_

#define KEY   1234 //一个用来标识共享内存块的键值小
#define SIZE  600000 //指定了所申请的内存块的大小 接收任意曲线8千多个数据时注意该值大小

extern readret1;
void uart_init(void);
void GPIBSCPI_Dispose(char *scpibuf,int scpilen);

#endif