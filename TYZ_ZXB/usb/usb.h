#ifndef __USB_H
#define __USB_H

#define KEY   1234 		//һ��������ʶ�����ڴ��ļ�ֵС
#define SIZE  600000 	//ָ������������ڴ��Ĵ�С ������������8ǧ�������ʱע���ֵ��С
#define TMC_DEV "/dev/usb_tmc"
#define USB_SER "/dev/ttyGS0"
void USBSCPI_Dispose(char *scpibuf,int scpilen);
#endif