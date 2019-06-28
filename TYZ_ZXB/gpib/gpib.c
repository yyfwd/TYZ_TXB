/********************************************************************
Copyright (C), 2018, Tech. Co., Ltd.
File name:  glib.c
Author: 	Yangyongfeng      
Version: 	V1.0       
Date: 	    2018.7.10
Description: GPIBͨѶ����������
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "gpmc.h"
#include "NGPIB_IO.h"
#include "IO_PARAM.h"
#include "Funtion.h"
#include "gpib.h"
#include "../vxi11.h"

share_mem *share_gpib;   //�����̹����ڴ�ṹ��
int share_id;            //�����ڴ��ID
char readbuf[150000];    //GPIB���յ�������

int spoll_count = 0;
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
//����GPIB���յ����ַ���
/******************************************/
void GPIBSCPI_Dispose(char *scpibuf,int scpilen)
{
	int i;            //forѭ������
	int id_num = 0;   //SCPIָ�������õ���ID
	int back;         //�����������߱�־
	int saveback;     //�����������߱�־
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
		back = AnyData_Analy(scpibuf,id_num);            /*�������߽�ȡ���ݲ���*/
		saveback = AnyData_SaveMode(scpibuf,id_num);	 /*�������߽�ȡ���ݲ���*/
		if(back == 1 || saveback == 1) //Ϊ������������ʱ���������ݰ�
		{					
			if(volt_get ==1)  //���͵�ѹ���ݽ������
			{
				volt_get = 0;
				volt_curr_get++;
				//printf("%s\n",share_vxi11->volt_data);
			}
			if(curr_get ==1)  //���͵������ݽ������
			{
				curr_get = 0;
				volt_curr_get++;
				//printf("%s\n",share_vxi11->curr_data);
			}
			
			if(volt_save == 1)  //�����ѹ���ݽ������
			{
				volt_save = 0;
				volt_curr_save++;
			}
			if(curr_save == 1)  //����������ݽ������
			{
				curr_save = 0;
				volt_curr_save++;
			}
			
			if(volt_curr_get == 2) //��ѹ�������ݶ��������
			{
				volt_curr_get = 0;
				share_gpib->anydata_fromPC_flag = Enable;  //����λ���·����߱�־λ
				printf("\nanydata line download success\n");
			}
			if(volt_curr_save == 2)  //������������ݽ������
			{
				volt_curr_save = 0;
				share_gpib->anydata_savetxb_ok = Enable;  //�洢��λ�����͵������������ݵ�ͨѶ���־λ
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
				strcpy(share_gpib->save_serial_number,serial_number);  //�洢ģ��������
			}
			printf("___vxi _ now __pack_count = %d\n",share_gpib->vxi_ID_parameter_pack_count);

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
			if((id_num!=SET_SAS_IMP) && (id_num != SET_SAS_ISC) && (id_num != SET_SAS_VMP) && (id_num != SET_SAS_VOC)) //�������ָ���SAS�ĸ�����������ָ��
			{
				if((id_num == OUTPUT_ON)&&(strncmp(share_gpib->vxi11_mode_buf,"TAB",3)==0)) //�����TABģʽ��ִ�п���ָ������Ӧ
				{
					;
				}
				else if(((strncmp(share_gpib->vxi11_mode_buf,"TAB",3) != 0) || (share_gpib->anydata_fromPC_flag == Enable)) && (id_num == SET_SEL_NUM)) //�����ǰģʽΪ��TABģʽ ���� ״̬Ϊ"����λ���·�����"����"��λ��ѡ�����ߺſ���"ָ����Ч
				{
					;
				}
				else if(id_num == SET_CC_CURR) //�ж�CC�������� 20>=CC����>=0
				{
					if((CC_CURR_DATA >= 0) && (CC_CURR_DATA <= 20)) //�����С��Χ�������
					{
						share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = SET_CC_CURR;
						share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1] = 1;
						share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1 + 1 ] = CC_CURR_DATA;
						share_gpib->vxi_ID_parameter_pack_count = share_gpib->vxi_ID_parameter_pack_count + 1 + 1 + 1;
						if(share_gpib->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
						{
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
							share_gpib->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
						}
						printf("___vxi _ next __pack_count = %d\n",share_gpib->vxi_ID_parameter_pack_count);
					}
				}
				else if(id_num == SET_CC_VOLT) //�ж�CC��ѹ���� 120>=CC��ѹ>=0
				{
					if((CC_VOLT_DATA >= 0) && (CC_VOLT_DATA <= 120)) //�����С��Χ�������
					{
						share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = SET_CC_VOLT;
						share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1] = 1;
						share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1 + 1 ] = CC_VOLT_DATA;
						share_gpib->vxi_ID_parameter_pack_count = share_gpib->vxi_ID_parameter_pack_count + 1 + 1 + 1;
						if(share_gpib->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
						{
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
							share_gpib->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
						}
						printf("___vxi _ next __pack_count = %d\n",share_gpib->vxi_ID_parameter_pack_count);
					}						
				}
				else  //ΪSAS/CC�������á���TABģʽ��ִ�п���ָ��ж�ģʽΪ��"TAB"/״̬Ϊ"����λ���·�����" ֮����������
				{
					share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = id_num; //��ID���빲���ڴ棬���ڴ��ڽ���ʶ�����ָ�ǰ���
					//printf("vxi share_gpib->vxi_ID_parameter_pack_count = %d\n",share_gpib->vxi_ID_parameter_pack_count);
					//printf("vxi ID_parameter_pack = %f\n",share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count]);
					if(scpi_parameter_count != 0) //�յ��˴�������ָ�� ������VOC ISC
					{
						share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1] = scpi_parameter_count; //��¼���˼�������
						for(i=0;i<scpi_parameter_count;i++) //�洢����
						{
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1 + 1 + i] = scpi_parameter[i]; //��ָ��Ĳ������빲���ڴ�
						}
					}
					else //û�յ��˴�������ָ�� �翪�ػ� ����ģʽ
					{
						share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1] = 0; //��¼���˼������� 0��
					}
					share_gpib->vxi_ID_parameter_pack_count = share_gpib->vxi_ID_parameter_pack_count + 1 + scpi_parameter_count + 1; //ָ���������һ���洢λ��
					
					if(share_gpib->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
					{
						share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
						share_gpib->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
					}
					printf("___vxi _ next __pack_count = %d\n",share_gpib->vxi_ID_parameter_pack_count);
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
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = SET_SAS_VMP;
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1] = 1;
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1 + 1 ] = VMP_DATA;
							share_gpib->vxi_ID_parameter_pack_count = share_gpib->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_gpib->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
							{
								share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
								share_gpib->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
							}
							printf("___vxi _ next __pack_count = %d\n",share_gpib->vxi_ID_parameter_pack_count);
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = SET_SAS_VOC;
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1] = 1;
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1 + 1 ] = VOC_DATA;
							share_gpib->vxi_ID_parameter_pack_count = share_gpib->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_gpib->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
							{
								share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
								share_gpib->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
							}
							printf("___vxi _ next __pack_count = %d\n",share_gpib->vxi_ID_parameter_pack_count);
							
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = SET_SAS_ISC;
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1] = 1;
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1 + 1 ] = ISC_DATA;
							share_gpib->vxi_ID_parameter_pack_count = share_gpib->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_gpib->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
							{
								share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
								share_gpib->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
							}	
							printf("___vxi _ next __pack_count = %d\n",share_gpib->vxi_ID_parameter_pack_count);
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = SET_SAS_IMP;
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1] = 1;
							share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count + 1 + 1 ] = IMP_DATA;
							share_gpib->vxi_ID_parameter_pack_count = share_gpib->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_gpib->vxi_ID_parameter_pack_count >= 100) //ָ����һ��ID
							{
								share_gpib->ID_parameter_pack[share_gpib->vxi_ID_parameter_pack_count] = -1; //�洢��������-1 ��ʾ����һ��ѭ�����
								share_gpib->vxi_ID_parameter_pack_count = 0; //����ָ���������ʼ�洢λ��
							}
							printf("___vxi _ next __pack_count = %d\n",share_gpib->vxi_ID_parameter_pack_count);
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
int main(void)
{
	Mem_Init();                //�ڴ��ʼ��
	Gpmc_Init();               //Gpmcͨ�ô洢��������ʼ��
	Initialize_Interface();    //��ʼ���ӿ�
	Set_Timeout(0xb,TRUE);     //���ó�ʱֵΪ1��
	Set_4882_Status(SRE,0x10); //����SRE״̬�Ĵ���
	Set_Address_Mode(0);       //���õ�һ����ַģʽ
	Change_Primary_Address(1); //GPIB��ַ����Ϊ1
	
	share_id = shmget((key_t)KEY, SIZE, 0666|IPC_CREAT); //���������ڴ棬���ڽ���֮���ͨѶ ���ڹ����ڶ�ȡ����ֵ���䳤��
	share_gpib = (share_mem *) shmat(share_id,0,0);      //ӳ�乲���ڴ�
	//printf("GPIB share_id:%d\n",share_id);
	
	SCPI_Init(&scpi_context,scpi_commands,&scpi_interface,scpi_units_def,0, 0, 0, 0,
			scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE); //��ʼ��SCPIָ�������
	
	share_gpib->gpib_addr_id = 1;                         //��ʼ�������ڴ����GPIB��ַΪ1
	pthread_t thread;                                     //�����߳�ID
	pthread_create(&thread,NULL,&GPIB_AddrId_Check,NULL); //���GPIB��ַID�Ƿ��иı�
	while(1)
	{
		
		do
		{
			usleep(200000);
			Update_INTERFACE_STATUS();   //����״̬��
			//printf("Update_INTERFACE_STATUS\n");
		}
		while(Read_GPIB_Lines()&0x8000); //��ATNδ����ʱ����Update_INTERFACE_STATUS()����CPT_Handler()����CPT_Handler()��֤HS488�Ͷ�����ַ��
                                         //�����ȴ�GPIB��ȡ��Щ�ֽڣ�Ȼ����ܵ���I/O����
	
	 //If SPOLL set
 		if(INTERFACE_STATUS & SPOLL)
		{
			//printf("SPOLL\n");
			Set_4882_Status(STB,spoll_count++);  //����STB״̬�Ĵ���
			Clear_4882_Status(STB,0xff);         //���STB״̬�Ĵ���
		}
	//����״̬
		else if(INTERFACE_STATUS & LACS)
		{
			//printf("LACS\n");
			memset(readbuf,0,150000);       //��ս��ջ�����buff
			Receive(readbuf,150000,EOI);    //��ȡGPIB����
			if(DATA_COUNT > 0)  //GPIB���ݳ���
			{
				readbuf[DATA_COUNT] = '\0';         //������ĩβ����ֹͣ��
				printf("\nGPIB read len = %d\n",DATA_COUNT); //��ӡ���ݳ���
				if(DATA_COUNT < 200)                //�����������������ӡ����
				{
					printf("GPIB read buf = %s\n", readbuf);
				}
				
				GPIBSCPI_Dispose(readbuf,DATA_COUNT); //����GPIB���յ����ַ���
				
				Set_4882_Status(STB,0x10);  //����STB״̬�Ĵ��� Set MAV bit in STB
			}
		}
	//��ȡ״̬
		else if(INTERFACE_STATUS & TACS)
		{
			//printf("TACS\n");
			if(scpi_check_id != 0) //�в�ѯָ��
			{
				GPIB_SCPI_Read();  //��ѯָ�����
				Send(GPIB_ReadBuf,GPIB_ReadCount,EOI);  //����ѯ���������ϴ�����λ�� 				
			}
			else
			{
				DATA_COUNT = 0;  //û�в�ѯ����ʱ�������ֽ�������
			}
			
			if(DATA_COUNT>0)
			{
				Clear_4882_Status(STB,0x10);  //�����STB״̬�Ĵ��� Clear MAV bit
			}
		}
	//����״̬
		if(INTERFACE_STATUS & ERR)
		{
			//printf("ERR\n");
			//If EABO report it 
			//INTERFACE_ERROR = ENOL(1):û������
			//INTERFACE_ERROR = EARG(2):��������������ܽ���ĳ�ֲ�������
			//INTERFACE_ERROR = EABO(3):��ʱ
			usleep(1000);
			//printf("\nGPIB Serror has occured (INTERFACE_ERROR = %d)",INTERFACE_ERROR);
			INTERFACE_STATUS &= ~ERR;
		}
		
	} 
	Interface_Off();  //�رսӿ�
	Gpmc_free();      //���Gpmcͨ�ô洢������
	Mem_free();       //����ڴ�
	return 0;
}
