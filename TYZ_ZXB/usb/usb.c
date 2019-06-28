/********************************************************************
Copyright (C), 2018, Tech. Co., Ltd.
File name:  usb.c
Author: 	Yangyongfeng      
Version: 	V1.0       
Date: 	    2018.6.10
Description: USBͨѶ����������
********************************************************************/
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<netdb.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>   
#include<unistd.h>    
#include<sys/stat.h>   
#include<fcntl.h>     
#include<termios.h>  
#include<sys/wait.h>
#include "Funtion.h"
#include "../vxi11.h"
#include "usb.h"

char readbuf[150000];   //USB���յ�������
share_mem *share_usb;   //�����̹����ڴ�ṹ��
int share_id;           //�����ڴ��ID
int usbread = 0;		//usb��ȡ�ַ����ȼ���
int usbwrite = 0;		//usbд�ַ����ȼ���
int volt_curr_get = 0;  //��ѹ�������ݶ�������ɱ�־λ
int volt_curr_save = 0; //��ѹ�������ݶ�������ɱ�־λ

int SAS_IMP_Flag = 0;  //IMP�����Ƿ������ɱ�־λ 0Ĭ�� 1�������
int SAS_ISC_Flag = 0;  //ISC�����Ƿ������ɱ�־λ 0Ĭ�� 1�������
int SAS_VMP_Flag = 0;  //VMP�����Ƿ������ɱ�־λ 0Ĭ�� 1�������
int SAS_VOC_Flag = 0;  //VOC�����Ƿ������ɱ�־λ 0Ĭ�� 1�������

double IMP_DATA = 0;  //�洢IMP����
double ISC_DATA = 0;  //�洢ISC����
double VMP_DATA = 0;  //�洢VMP����
double VOC_DATA = 0;  //�洢VOC����
double CC_CURR_DATA = 0;  //�洢CC��������
double CC_VOLT_DATA = 0;  //�洢CC��ѹ����
double SAS_Kn = 0;    //SAS��I-Vб��

/******************************************/
//����USB���յ����ַ���
/******************************************/
void USBSCPI_Dispose(char *scpibuf,int scpilen)
{
	int i;    //forѭ������
	int back;         //�����������߱�־
	int saveback;     //�����������߱�־
	int id_num = 0;           //SCPIָ�������õ���ID
	char scpi_str[210] = {};  //�洢��Ч��scpiָ���ַ�
	char add_str[4] = "\r\n"; //�ַ�ĩβ����\r\n
	memset(scpi_str,0,210);
	if(scpilen > 200) //�����ַ��������ж��Ƿ�Ϊ�������߰�
	{
		strncpy(scpi_str,scpibuf,100);  //��ȡǰ100���ַ���ָ���ж�	
		strncat(scpi_str,add_str,4);  //�ַ�ĩβ����\r\n
		SCPI_CHECK(scpi_str);         //scpiָ�����
		printf("scpi_id = %d\n",scpi_id);  //��ӡָ�������õ���ID
		id_num = scpi_id;  //�洢ID
		scpi_id = 0;       //����
		back = AhyData_Analy(scpibuf,id_num);
		saveback = AnyData_SaveMode(scpibuf,id_num);
		if(back == 1 || saveback == 1) //Ϊ������������ʱ���������ݰ�
		{					
			if(volt_get ==1) //���͵�ѹ���ݽ������
			{
				volt_get = 0;
				volt_curr_get++;
				//printf("%s\n",share_vxi11->volt_data);
			}
			if(curr_get ==1) //���͵������ݽ������
			{
				curr_get = 0;
				volt_curr_get++;
				//printf("%s\n",share_vxi11->curr_data);
			}
			
			if(volt_save == 1) //�����ѹ���ݽ������
			{
				volt_save = 0;
				volt_curr_save++;
			}
			if(curr_save == 1) //����������ݽ������
			{
				curr_save = 0;
				volt_curr_save++;
			}
			
			if(volt_curr_get == 2) //��ѹ�������ݶ��������
			{
				volt_curr_get = 0;
				share_usb->anydata_fromPC_flag = Enable;          //ѡ�����λ���·�����ģʽ
				printf("\nanydata line download success\n");
			}
			if(volt_curr_save == 2)  //������������ݽ������
			{
				volt_curr_save = 0;
				share_usb->anydata_savetxb_ok = Enable;
				printf("\nanydata save download success\n");
			}
		}
	}
	else  //Ϊ��ͨ��SCPIָ�� ���������߰�
	{
		strncpy(scpi_str,scpibuf,scpilen);  //��ȡ��Ч��scpiָ���ַ�	
		strncat(scpi_str,add_str,4);  //�ַ�ĩβ����\r\n
		SCPI_CHECK(scpi_str);         //scpiָ�����
		printf("scpi_id = %d\n",scpi_id);  //��ӡָ�������õ���ID
		id_num = scpi_id;  //�洢ID
		scpi_id = 0;       //����
		if((id_num>300)&&(id_num<400))  //��ѯID�ķ�ΧΪ301~400
		{
			//Ϊ��ѯָ���ID�� ��ѯָ���Ҫ�������ݸ�ǰ��� ֱ����vxi11��ȡ�����дӹ����ڴ��ȡ������Ϊ����ֵ
			scpi_check_id = id_num; //��ѯָ��ID��
		}
		else if((id_num>0)&&(id_num<300)) //����ID�ķ�ΧΪ0~299
		{
			if(id_num == SERIAL_NUMBER) //����ģ��������
			{
				strcpy(share_usb->save_serial_number,serial_number);  //�洢ģ��������
			}
			printf("___vxi _ now __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
			
			switch(id_num)              //�ж�ID�Ƿ���SAS���ĸ�����������
			{
				case SET_SAS_IMP:
					SAS_IMP_Flag = 1;IMP_DATA = scpi_parameter[0];break; //IMP�����Ƿ������ɱ�־λ 0Ĭ�� 
				case SET_SAS_ISC:
					SAS_ISC_Flag = 1;ISC_DATA = scpi_parameter[0];break; //ISC�����Ƿ������ɱ�־λ 0Ĭ�� 
				case SET_SAS_VMP:
					SAS_VMP_Flag = 1;VMP_DATA = scpi_parameter[0];break; //VMP�����Ƿ������ɱ�־λ 0Ĭ�� 
				case SET_SAS_VOC:
					SAS_VOC_Flag = 1;VOC_DATA = scpi_parameter[0];break; //VOC�����Ƿ������ɱ�־λ 0Ĭ�� 
				case SET_CC_CURR:
					CC_CURR_DATA = scpi_parameter[0];break;
				case SET_CC_VOLT:
					CC_VOLT_DATA = scpi_parameter[0];break;
				default:break;
			}
			
			//����ָ�� �洢ID�Ͳ���������uart���̷������ʹ�������
			if((id_num!=SET_SAS_IMP) && (id_num != SET_SAS_ISC) && (id_num != SET_SAS_VMP) && (id_num != SET_SAS_VOC)) 
			{
				if((id_num == OUTPUT_ON)&&(strncmp(share_usb->vxi11_mode_buf,"TAB",3)==0))
				{
					;                 //�����TABģʽ��ִ�п���ָ������Ӧ
				}
				else if(((strncmp(share_usb->vxi11_mode_buf,"TAB",3) != 0) || (share_usb->anydata_fromPC_flag == Enable)) && (id_num == SET_SEL_NUM)) //�����ǰģʽΪ��TABģʽ ���� ״̬Ϊ"����λ���·�����"����"��λ��ѡ�����ߺſ���"ָ����Ч
				{
					;
				}
				else if(id_num == SET_CC_CURR) //�ж�CC�������� 20>=CC����>=0
				{
					if((CC_CURR_DATA >= 0) && (CC_CURR_DATA <= 20)) //�����С��Χ�������
					{
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_CC_CURR;
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = CC_CURR_DATA;
						share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
						if(share_usb->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
						{
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
							share_usb->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
						}
						printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
					}
				}
				else if(id_num == SET_CC_VOLT) //�ж�CC��ѹ���� 120>=CC��ѹ>=0
				{
					if((CC_VOLT_DATA >= 0) && (CC_VOLT_DATA <= 120)) //�����С��Χ�������
					{
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_CC_VOLT;
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = CC_VOLT_DATA;
						share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
						if(share_usb->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
						{
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
							share_usb->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
						}
						printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
					}						
				}
				else  //ΪSAS/CC�������á���TABģʽ��ִ�п���ָ��ж�ģʽΪ��"TAB"/״̬Ϊ"����λ���·�����" ֮����������
				{
					share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = id_num; //��ID���빲���ڴ棬���ڴ��ڽ���ʶ�����ָ�ǰ���
					//printf("vxi share_usb->vxi_ID_parameter_pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
					//printf("vxi ID_parameter_pack = %f\n",share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count]);
					if(scpi_parameter_count != 0) //�յ��˴�������ָ�� ������VOC ISC
					{
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = scpi_parameter_count; //��¼���˼�������
						for(i=0;i<scpi_parameter_count;i++) //�洢����
						{
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 + i] = scpi_parameter[i]; //��ָ��Ĳ������빲���ڴ�
						}
					}
					else //û�յ��˴�������ָ�� �翪�ػ� ����ģʽ
					{
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 0; //��¼���˼������� 0��
					}
					share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + scpi_parameter_count + 1; //ָ���������һ���洢λ��		
					if(share_usb->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
					{
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
						share_usb->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
					}
					printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
				}
			}
			else  //�����SAS���ò���ָ��
			{
				if((SAS_IMP_Flag == 1)&&(SAS_ISC_Flag == 1)&&(SAS_VMP_Flag == 1)&&(SAS_VOC_Flag == 1)) //ֻ���ĸ����ǵ㶼�����˲Ž���б������
				{
					SAS_IMP_Flag = 0; //�����־λ
					SAS_ISC_Flag = 0;
					SAS_VMP_Flag = 0;
					SAS_VOC_Flag = 0;
					//��ȡ����SAS���ò��������ж� VOC>VMP ISC>IMP 130>=VOC>=0 20>=ISC>=0 VMP>=0 IMP>=0
					if((VOC_DATA>VMP_DATA)&&(ISC_DATA>IMP_DATA)&&(VOC_DATA>=0)&&(VOC_DATA<=130)&&(ISC_DATA>=0)&&(ISC_DATA<=20)&&(VMP_DATA>=0)&&(IMP_DATA>=0))  //�жϴ�С��ϵ
					{
						SAS_Kn = (IMP_DATA/(VOC_DATA-VMP_DATA))+(ISC_DATA/VOC_DATA);
						printf("SAS IV K:%.9f\n",SAS_Kn);
						if(SAS_Kn < 1.6)
						{
							printf("SAS IV success\n");
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_SAS_VMP;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = VMP_DATA;
							share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_usb->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
							{
								share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
								share_usb->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
							}
							
							printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_SAS_VOC;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = VOC_DATA;
							share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_usb->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
							{
								share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
								share_usb->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
							}
							printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
							
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_SAS_ISC;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = ISC_DATA;
							share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_usb->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
							{
								share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
								share_usb->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
							}
							
							printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_SAS_IMP;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = IMP_DATA;
							share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_usb->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
							{
								share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
								share_usb->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
							}
							printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
						}
					}
						
				}
			}
		}			
	}
}

/******************************************/
//������
/******************************************/
int main()
{
	int fd; //�ļ�������
	//fd = open(USB_SER,O_RDWR); //���ʹ��USB������������λ�����͵��������Ӧ�ô� 0d 0a
	fd = open(TMC_DEV,O_RDWR);  //��USB�豸�� USBTMC����
	if(fd != -1)  //���ļ��ɹ�
	{
		printf("open USB_TMC success\n");
	}
	else  //���ļ�ʧ��
	{
		printf("open USB_TMC failed\n");
	}
	
	share_id = shmget((key_t)KEY, SIZE, 0666|IPC_CREAT); //���������ڴ棬���ڽ���֮���ͨѶ ���ڹ����ڶ�ȡ����ֵ���䳤��	
	share_usb = (share_mem *) shmat(share_id,0,0);       //ӳ�乲���ڴ�
	//printf("USB share_id:%d\n",share_id);
	
	SCPI_Init(&scpi_context,scpi_commands,&scpi_interface,scpi_units_def,0, 0, 0, 0,
			scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE); //��ʼ��SCPIָ�������            
	while(1)
	{
		memset(readbuf,0,150000);         //��ս��ջ�����buff
		usbread=read(fd,readbuf,150000);  //��ȡUSB����
		
		if(usbread > 0)  //USB���ݳ���
		{
			printf("\nUSB read len = %d\n",usbread); //��ӡ���ݳ���
			if(usbread < 200)  //�����������������ӡ
			{
				printf("USB read buf = %s\n",readbuf);
			}
			USBSCPI_Dispose(readbuf,usbread);  //����USB���յ����ַ���
			tcflush(fd, TCIFLUSH);             //��������������
		}
		
		if(scpi_check_id != 0)  //�в�ѯָ��
		{
			USB_SCPI_Read();        //��ѯָ�����
			tcflush(fd, TCIFLUSH);  //���дBUFF
			usbwrite = write(fd,USB_ReadBuf,USB_ReadCount);  //����ѯ���������ϴ�����λ��
		}
		else  //��else����ռ��cpuʹ���ʹ���
		{
			usleep(10000);
		}
	}
	close(fd);  //�ر�USB�豸
	return 0;
}

