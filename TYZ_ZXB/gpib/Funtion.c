#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "NGPIB_IO.h"
#include "Funtion.h"
#include "gpib.h"
#include "../vxi11.h"

char device_info[50] = {"HTXY,HTXY-TYZmachine,0001,V1.0"};  //IDN查询返回值 制造商,型号,序列号,固件版本
char SYSTem_ERRor_info[] = {"+0"};                          //系统错误查询值
extern struct share_mem *share_gpib;                        //父进程共享内存结构体

int  scpi_check_id;  //查询指令ID号
char *GPIB_ReadBuf = NULL; //GPIB发送给上位机的数据
int GPIB_ReadCount = 0;    //GPIB发送给上位机的数据长度
int volt_get = 0;  //接收电压数据标志位
int curr_get = 0;  //接收电流数据标志位
int volt_save = 0; //保存曲线电压标志位
int curr_save = 0; //保存曲线电流标志位
int Old_GPIBAddrID = 1; //默认GPIB地址是1
int New_GPIBAddrID = 0; //存储前面板设置的GPIB地址

/******************************************/
//检测GPIB地址ID是否有改变
/******************************************/
void  GPIB_AddrId_Check(void)
{
	while(1)
	{
		New_GPIBAddrID = share_gpib->gpib_addr_id;
		if(New_GPIBAddrID != Old_GPIBAddrID)
		{	
			Old_GPIBAddrID = New_GPIBAddrID;
			Change_Primary_Address(New_GPIBAddrID);	
			printf("GPIB Address set success = %d\n",New_GPIBAddrID);
		}
		else
		{
			usleep(10000);
		}
	}
}

/******************************************/
//判断是否为设置任意曲线时发来的数据包
/******************************************/
int AnyData_Analy(char *data,int ID)
{
	int result = 0;
	int i=0;
	if((ID == SET_TABL_CURR)||(ID == SET_TABL_VOLT))
	{
		do 
			i++;
		while(data[i] != ' '); //定位到空格处
		if(ID == SET_TABL_VOLT)
		{
			sprintf(share_gpib->volt_data,"%s",data+i+1); //提取数据部分
			//printf("%s\n",share_gpib->volt_data);
			volt_get = 1;  //接收电压数据标志位
			result = 1;
		}
		else if(ID == SET_TABL_CURR)
		{
			sprintf(share_gpib->curr_data,"%s",data+i+1); //提取数据部分
			curr_get = 1;  //接收电流数据标志位
			result = 1;
			//printf("%s\n",share_gpib->curr_data);
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
//判断是否为保存任意曲线时发来的数据包
/******************************************/
int AnyData_SaveMode(char *data,int IDSave)
{
	int result = 0;
	int i=0,j=0,k=0,len=0;
	if((IDSave >= 511)&&(IDSave <= 545))   //去除指令字 提取数据部分 MEM:TABL:CURR 参数,参数,..  MEM:TABL:VOLT 参数,参数,...
	{
		do 
			i++;
		while(data[i] != ' '); 	//定位到空格处
		j = i;
		do 
			j--;
		while(data[j] != ':'); 	//定位到空格前一个冒号
		len = i-j-1;
		memset(share_gpib->Curve_Name,0,20);
		for(k=0;k<len;k++)
		{
			share_gpib->Curve_Name[k] = data[j+k+1];
		}
		printf("CURVE NAME:%s\n",share_gpib->Curve_Name);
		if((IDSave >=531)&&(IDSave <=545))
		{
			sprintf(share_gpib->volt_data,"%s",data+i+1); //提取数据部分
			//printf("%s\n",share_gpib->volt_data);
			volt_save = 1;  //保存曲线电压标志位
			result = 1;
		}
		else if((IDSave >=511)&&(IDSave <=525))
		{
			sprintf(share_gpib->curr_data,"%s",data+i+1); //提取数据部分
			//printf("%s\n",share_gpib->curr_data);
			curr_save = 1;  //保存曲线电流标志位
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
//查询指令解析
/******************************************/
void GPIB_SCPI_Read(void)
{
	switch(scpi_check_id)  //查询指令ID号
	{
		case OUTPUT_STATE: //查询开关机状态
		{
			GPIB_ReadCount = strlen(share_gpib->vxi11_onoff_buf); //开关机状态长度
			GPIB_ReadBuf = share_gpib->vxi11_onoff_buf; //存储开关机状态，将发送到上位机
			//printf("__vxi11_onoff  = %02x\n",GPIB_ReadBuf[0]);
		}break;
		case MODE_STATE: //查询模式设置
		{
			GPIB_ReadCount = strlen(share_gpib->vxi11_mode_buf); //查询模式设置长度
			GPIB_ReadBuf = share_gpib->vxi11_mode_buf; //存储查询模式设置，将发送到上位机
		}break;
		case CHECK_SAS_IMP: //查询曲线峰值功率点处电流
		{
			//printf("  check IMP\n");
			GPIB_ReadCount = strlen(share_gpib->vxi11_IMP_buf); //曲线峰值功率点处电流长度
			GPIB_ReadBuf = share_gpib->vxi11_IMP_buf; //存储曲线峰值功率点处电流，将发送到上位机
		}break;
		case CHECK_SAS_ISC: //查询短路电流
		{
			//printf("  check ISC\n");
			GPIB_ReadCount = strlen(share_gpib->vxi11_ISC_buf); //设置短路电流长度
			GPIB_ReadBuf = share_gpib->vxi11_ISC_buf; 		//存储设置短路电流，将发送到上位机
		}break;
		case CHECK_SAS_VMP: //查询曲线峰值功率点处电压
		{
			//printf("  check VMP\n");
			GPIB_ReadCount = strlen(share_gpib->vxi11_VMP_buf); //曲线峰值功率点处电压长度
			GPIB_ReadBuf = share_gpib->vxi11_VMP_buf; //存储曲线峰值功率点处电压，将发送到上位机
		}break;
		case CHECK_SAS_VOC: //查询开路电压
		{
			//printf("  check VOC\n");
			GPIB_ReadCount = strlen(share_gpib->vxi11_VOC_buf); //开路电压长度
			GPIB_ReadBuf = share_gpib->vxi11_VOC_buf; //存储开路电压，将发送到上位机
		}break;
		case CHECK_CC_CURR: //查询CC(FIXed)模式下电流
		{
			//printf("  check CC_CURR\n");
			GPIB_ReadCount = strlen(share_gpib->vxi11_cc_current); //CC模式下电流长度
			GPIB_ReadBuf = share_gpib->vxi11_cc_current; //存储CC模式下电流，将发送到上位机
		}break;
		case CHECK_CC_VOLT: //查询CC(FIXed)模式下电压
		{
			//printf("  check CC_VOLT\n");
			GPIB_ReadCount = strlen(share_gpib->vxi11_cc_voltage); //CC模式下电压长度
			GPIB_ReadBuf = share_gpib->vxi11_cc_voltage;  //存储CC模式下电压，将发送到上位机//存储CC模式下电压，将发送到上位机
		}break;
		case CHECK_CURR: //查询实时电流命令
		{
			GPIB_ReadCount = strlen(share_gpib->vxi11_current_buf); //返回平均输出电流长度
			GPIB_ReadBuf = share_gpib->vxi11_current_buf; //存储串口读取到的返回平均输出电流数据，将发送到上位机
		}break;
		case CHECK_VOLT: //查询实时电压命令
		{
			GPIB_ReadCount = strlen(share_gpib->vxi11_voltage_buf); //返回平均输出电压长度
			GPIB_ReadBuf = share_gpib->vxi11_voltage_buf; //存储串口读取到的返回平均输出电压数据，将发送到上位机
		}break;
		case CHECK_ID: //IDN查询命令
		{
			GPIB_ReadCount = strlen(device_info);    //IDN查询值长度
			GPIB_ReadBuf  = device_info;          //IDN查询返回值 制造商,型号,序列号,固件版本
		}break;
		case CHECK_SYSTEM_ERROR: //系统错误查询命令 故障状态数据
		{
			GPIB_ReadCount = strlen(SYSTem_ERRor_info);   //系统错误查询命令值长度
			GPIB_ReadBuf = SYSTem_ERRor_info;          //系统错误查询命令值
		}break;
		case CHECK_MACHINE_ERROR: //查询机器故障 返回故障字给上位机
		{
			GPIB_ReadCount = strlen(share_gpib->vxi11_error_buf); //故障状态数据长度
			GPIB_ReadBuf = share_gpib->vxi11_error_buf; //存储故障状态数据，将发送到上位机
		}break;
		case 313: //读取模拟器的状态参数命令 暂时不起作用
		{
			
		}break;
		default://无内容转发到上位机
			GPIB_ReadCount = 0;break;
	}
	scpi_check_id = 0; //查询指令ID号清零
}
