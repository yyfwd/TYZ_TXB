#ifndef _GPIB_H_
#define _GPIB_H_

#define KEY   1234 //һ��������ʶ�����ڴ��ļ�ֵС
#define SIZE  600000 //ָ������������ڴ��Ĵ�С ������������8ǧ�������ʱע���ֵ��С

extern readret1;
void uart_init(void);
void GPIBSCPI_Dispose(char *scpibuf,int scpilen);

#endif