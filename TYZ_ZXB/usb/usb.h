#ifndef __USB_H
#define __USB_H

#define KEY   1234 		//一个用来标识共享内存块的键值小
#define SIZE  600000 	//指定了所申请的内存块的大小 接收任意曲线8千多个数据时注意该值大小
#define TMC_DEV "/dev/usb_tmc"
#define USB_SER "/dev/ttyGS0"
void USBSCPI_Dispose(char *scpibuf,int scpilen);
#endif