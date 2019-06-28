/********************************************************************
Copyright (C), 2018, Tech. Co., Ltd.
File name:  Funtion.c
Author: 	Yangyongfeng      
Version: 	V1.0       
Date: 	    2018.6.15
Description: USBͨѶ����SCPI����API
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "Funtion.h"
#include "../vxi11.h"
#include "usb.h"

char device_info[50] = {"HTXY,HTXY-TYZmachine,0001,V1.0"};  //IDN��ѯ����ֵ ������,�ͺ�,���к�,�̼��汾
char SYSTem_ERRor_info[] = {"+0"};                          //ϵͳ�����ѯֵ
extern struct share_mem *share_usb;                         //�����̹����ڴ�ṹ��

int scpi_check_id = 0; //Ϊ��ѯָ���ID��
char *USB_ReadBuf = NULL; //USB���͸���λ��������
int USB_ReadCount = 0;    //USB���͸���λ�������ݳ���
int volt_get = 0;  //���յ�ѹ���ݱ�־λ
int curr_get = 0;  //���յ������ݱ�־λ
int volt_save = 0; //�������ߵ�ѹ��־λ
int curr_save = 0; //�������ߵ�����־λ

/******************************************/
//�ж��Ƿ�Ϊ������������ʱ���������ݰ�
/******************************************/
int AhyData_Analy(char *data,int ID)
{
	int result = 0;
	int i=0;
	if((ID == SET_TABL_CURR)||(ID == SET_TABL_VOLT))
	{
		do 
			i++;
		while(data[i] != ' '); //��λ���ո�
		if(ID == SET_TABL_VOLT)
		{
			sprintf(share_usb->volt_data,"%s",data+i+1); //��ȡ���ݲ���
			//printf("%s\n",share_usb->volt_data);
			volt_get = 1;  //���յ�ѹ���ݱ�־λ
			result = 1;
		}
		else if(ID == SET_TABL_CURR)
		{
			sprintf(share_usb->curr_data,"%s",data+i+1); //��ȡ���ݲ���
			curr_get = 1;  //���յ������ݱ�־λ
			result = 1;
			//printf("%s\n",share_usb->curr_data);
		}
		else
		{
			result = 0;
		}
	}
	else
	{
		result = 0;
	}
	return result;
}

/******************************************/
//�ж��Ƿ�Ϊ������������ʱ���������ݰ�
/******************************************/
int AnyData_SaveMode(char *data,int IDSave)
{
	int result = 0;
	int i=0,j=0,k=0,len=0;
	if((IDSave >= 511)&&(IDSave <= 545))   //ȥ��ָ���� ��ȡ���ݲ��� MEM:TABL:CURR ����,����,..  MEM:TABL:VOLT ����,����,...
	{
		do 
			i++;
		while(data[i] != ' '); 	//��λ���ո�
		j = i;
		do 
			j--;
		while(data[j] != ':'); 	//��λ���ո�ǰһ��ð��
		len = i-j-1;
		memset(share_usb->Curve_Name,0,20);
		for(k=0;k<len;k++)
		{
			share_usb->Curve_Name[k] = data[j+k+1];
		}
		printf("CURVE NAME:%s\n",share_usb->Curve_Name);
		if((IDSave >=531)&&(IDSave <=545))
		{
			sprintf(share_usb->volt_data,"%s",data+i+1); //��ȡ���ݲ���
			//printf("%s\n",share_usb->volt_data);
			volt_save = 1;  //�������ߵ�ѹ��־λ
			result = 1;
		}
		else if((IDSave >=511)&&(IDSave <=525))
		{
			sprintf(share_usb->curr_data,"%s",data+i+1); //��ȡ���ݲ���
			//printf("%s\n",share_usb->curr_data);
			curr_save = 1;  //�������ߵ�����־λ
			result = 1;
		}	
		else
		{
			result = 0;
		}
	}
	else
	{
		result = 0;
	}
	return result;	
}

/******************************************/
//��ѯָ�����
/******************************************/
void USB_SCPI_Read(void)
{
	switch(scpi_check_id)  //��ѯָ��ID��
	{
		case OUTPUT_STATE: //��ѯ���ػ�״̬
		{
			USB_ReadCount = strlen(share_usb->vxi11_onoff_buf); //���ػ�״̬����
			USB_ReadBuf = share_usb->vxi11_onoff_buf; //�洢���ػ�״̬�������͵���λ��
			//printf("__vxi11_onoff  = %02x\n",USB_ReadBuf[0]);
		}break;
		case MODE_STATE: //��ѯģʽ����
		{
			USB_ReadCount = strlen(share_usb->vxi11_mode_buf); //��ѯģʽ���ó���
			USB_ReadBuf = share_usb->vxi11_mode_buf; //�洢��ѯģʽ���ã������͵���λ��
		}break;
		case CHECK_SAS_IMP: //��ѯ���߷�ֵ���ʵ㴦����
		{
			//printf("  check IMP\n");
			USB_ReadCount = strlen(share_usb->vxi11_IMP_buf); //���߷�ֵ���ʵ㴦��������
			USB_ReadBuf = share_usb->vxi11_IMP_buf; //�洢���߷�ֵ���ʵ㴦�����������͵���λ��
		}break;
		case CHECK_SAS_ISC: //��ѯ��·����
		{
			//printf("  check ISC\n");
			USB_ReadCount = strlen(share_usb->vxi11_ISC_buf); //���ö�·��������
			USB_ReadBuf = share_usb->vxi11_ISC_buf; 		//�洢���ö�·�����������͵���λ��
		}break;
		case CHECK_SAS_VMP: //��ѯ���߷�ֵ���ʵ㴦��ѹ
		{
			//printf("  check VMP\n");
			USB_ReadCount = strlen(share_usb->vxi11_VMP_buf); //���߷�ֵ���ʵ㴦��ѹ����
			USB_ReadBuf = share_usb->vxi11_VMP_buf; //�洢���߷�ֵ���ʵ㴦��ѹ�������͵���λ��
		}break;
		case CHECK_SAS_VOC: //��ѯ��·��ѹ
		{
			//printf("  check VOC\n");
			USB_ReadCount = strlen(share_usb->vxi11_VOC_buf); //��·��ѹ����
			USB_ReadBuf = share_usb->vxi11_VOC_buf; //�洢��·��ѹ�������͵���λ��
		}break;
		case CHECK_CC_CURR: //��ѯCC(FIXed)ģʽ�µ���
		{
			//printf("  check CC_CURR\n");
			USB_ReadCount = strlen(share_usb->vxi11_cc_current); //CCģʽ�µ�������
			USB_ReadBuf = share_usb->vxi11_cc_current; //�洢CCģʽ�µ����������͵���λ��
		}break;
		case CHECK_CC_VOLT: //��ѯCC(FIXed)ģʽ�µ�ѹ
		{
			//printf("  check CC_VOLT\n");
			USB_ReadCount = strlen(share_usb->vxi11_cc_voltage); //CCģʽ�µ�ѹ����
			USB_ReadBuf = share_usb->vxi11_cc_voltage;  //�洢CCģʽ�µ�ѹ�������͵���λ��//�洢CCģʽ�µ�ѹ�������͵���λ��
		}break;
		case CHECK_CURR: //��ѯʵʱ��������
		{
			USB_ReadCount = strlen(share_usb->vxi11_current_buf); //����ƽ�������������
			USB_ReadBuf = share_usb->vxi11_current_buf; //�洢���ڶ�ȡ���ķ���ƽ������������ݣ������͵���λ��
		}break;
		case CHECK_VOLT: //��ѯʵʱ��ѹ����
		{
			USB_ReadCount = strlen(share_usb->vxi11_voltage_buf); //����ƽ�������ѹ����
			USB_ReadBuf = share_usb->vxi11_voltage_buf; //�洢���ڶ�ȡ���ķ���ƽ�������ѹ���ݣ������͵���λ��
		}break;
		case CHECK_ID: //IDN��ѯ����
		{
			USB_ReadCount = strlen(device_info);    //IDN��ѯֵ����
			USB_ReadBuf  = device_info;          //IDN��ѯ����ֵ ������,�ͺ�,���к�,�̼��汾
		}break;
		case CHECK_SYSTEM_ERROR: //ϵͳ�����ѯ���� ����״̬����
		{
			USB_ReadCount = strlen(SYSTem_ERRor_info);   //ϵͳ�����ѯ����ֵ����
			USB_ReadBuf = SYSTem_ERRor_info;          //ϵͳ�����ѯ����ֵ
		}break;
		case CHECK_MACHINE_ERROR: //��ѯ�������� ���ع����ָ���λ��
		{
			USB_ReadCount = strlen(share_usb->vxi11_error_buf); //����״̬���ݳ���
			USB_ReadBuf = share_usb->vxi11_error_buf; //�洢����״̬���ݣ������͵���λ��
		}break;
		case 313: //��ȡģ������״̬�������� ��ʱ��������
		{
			
		}break;
		default://������ת������λ��
			USB_ReadBuf = 0;break;
	}
	scpi_check_id = 0; //��ѯָ��ID������
}
