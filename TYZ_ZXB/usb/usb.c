/********************************************************************
Copyright (C), 2018, Tech. Co., Ltd.
File name:  usb.c
Author: 	Yangyongfeng      
Version: 	V1.0       
Date: 	    2018.6.10
Description: USB通讯主函数功能
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

char readbuf[150000];   //USB接收到的数据
share_mem *share_usb;   //父进程共享内存结构体
int share_id;           //共享内存的ID
int usbread = 0;		//usb读取字符长度计数
int usbwrite = 0;		//usb写字符长度计数
int volt_curr_get = 0;  //电压电流数据都接收完成标志位
int volt_curr_save = 0; //电压电流数据都保存完成标志位

int SAS_IMP_Flag = 0;  //IMP参数是否接收完成标志位 0默认 1接收完成
int SAS_ISC_Flag = 0;  //ISC参数是否接收完成标志位 0默认 1接收完成
int SAS_VMP_Flag = 0;  //VMP参数是否接收完成标志位 0默认 1接收完成
int SAS_VOC_Flag = 0;  //VOC参数是否接收完成标志位 0默认 1接收完成

double IMP_DATA = 0;  //存储IMP参数
double ISC_DATA = 0;  //存储ISC参数
double VMP_DATA = 0;  //存储VMP参数
double VOC_DATA = 0;  //存储VOC参数
double CC_CURR_DATA = 0;  //存储CC电流参数
double CC_VOLT_DATA = 0;  //存储CC电压参数
double SAS_Kn = 0;    //SAS的I-V斜率

/******************************************/
//分析USB接收到的字符串
/******************************************/
void USBSCPI_Dispose(char *scpibuf,int scpilen)
{
	int i;    //for循环计数
	int back;         //发送任意曲线标志
	int saveback;     //保存任意曲线标志
	int id_num = 0;           //SCPI指令经解析后得到的ID
	char scpi_str[210] = {};  //存储有效的scpi指令字符
	char add_str[4] = "\r\n"; //字符末尾补充\r\n
	memset(scpi_str,0,210);
	if(scpilen > 200) //根据字符串长度判断是否为任意曲线包
	{
		strncpy(scpi_str,scpibuf,100);  //获取前100个字符用指令判断	
		strncat(scpi_str,add_str,4);  //字符末尾补充\r\n
		SCPI_CHECK(scpi_str);         //scpi指令解析
		printf("scpi_id = %d\n",scpi_id);  //打印指令经解析后得到的ID
		id_num = scpi_id;  //存储ID
		scpi_id = 0;       //清零
		back = AhyData_Analy(scpibuf,id_num);
		saveback = AnyData_SaveMode(scpibuf,id_num);
		if(back == 1 || saveback == 1) //为设置任意曲线时发来的数据包
		{					
			if(volt_get ==1) //发送电压数据接收完成
			{
				volt_get = 0;
				volt_curr_get++;
				//printf("%s\n",share_vxi11->volt_data);
			}
			if(curr_get ==1) //发送电流数据接收完成
			{
				curr_get = 0;
				volt_curr_get++;
				//printf("%s\n",share_vxi11->curr_data);
			}
			
			if(volt_save == 1) //保存电压数据接收完成
			{
				volt_save = 0;
				volt_curr_save++;
			}
			if(curr_save == 1) //保存电流数据接收完成
			{
				curr_save = 0;
				volt_curr_save++;
			}
			
			if(volt_curr_get == 2) //电压电流数据都接收完成
			{
				volt_curr_get = 0;
				share_usb->anydata_fromPC_flag = Enable;          //选择从上位机下发曲线模式
				printf("\nanydata line download success\n");
			}
			if(volt_curr_save == 2)  //保存的曲线数据接收完毕
			{
				volt_curr_save = 0;
				share_usb->anydata_savetxb_ok = Enable;
				printf("\nanydata save download success\n");
			}
		}
	}
	else  //为普通的SCPI指令 非任意曲线包
	{
		strncpy(scpi_str,scpibuf,scpilen);  //获取有效的scpi指令字符	
		strncat(scpi_str,add_str,4);  //字符末尾补充\r\n
		SCPI_CHECK(scpi_str);         //scpi指令解析
		printf("scpi_id = %d\n",scpi_id);  //打印指令经解析后得到的ID
		id_num = scpi_id;  //存储ID
		scpi_id = 0;       //清零
		if((id_num>300)&&(id_num<400))  //查询ID的范围为301~400
		{
			//为查询指令的ID号 查询指令不需要发送数据给前面板 直接在vxi11读取函数中从共享内存读取数据作为返回值
			scpi_check_id = id_num; //查询指令ID号
		}
		else if((id_num>0)&&(id_num<300)) //设置ID的范围为0~299
		{
			if(id_num == SERIAL_NUMBER) //设置模拟器编码
			{
				strcpy(share_usb->save_serial_number,serial_number);  //存储模拟器编码
			}
			printf("___vxi _ now __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
			
			switch(id_num)              //判断ID是否是SAS的四个参数点设置
			{
				case SET_SAS_IMP:
					SAS_IMP_Flag = 1;IMP_DATA = scpi_parameter[0];break; //IMP参数是否接收完成标志位 0默认 
				case SET_SAS_ISC:
					SAS_ISC_Flag = 1;ISC_DATA = scpi_parameter[0];break; //ISC参数是否接收完成标志位 0默认 
				case SET_SAS_VMP:
					SAS_VMP_Flag = 1;VMP_DATA = scpi_parameter[0];break; //VMP参数是否接收完成标志位 0默认 
				case SET_SAS_VOC:
					SAS_VOC_Flag = 1;VOC_DATA = scpi_parameter[0];break; //VOC参数是否接收完成标志位 0默认 
				case SET_CC_CURR:
					CC_CURR_DATA = scpi_parameter[0];break;
				case SET_CC_VOLT:
					CC_VOLT_DATA = scpi_parameter[0];break;
				default:break;
			}
			
			//设置指令 存储ID和参数，用于uart进程分析发送串口数据
			if((id_num!=SET_SAS_IMP) && (id_num != SET_SAS_ISC) && (id_num != SET_SAS_VMP) && (id_num != SET_SAS_VOC)) 
			{
				if((id_num == OUTPUT_ON)&&(strncmp(share_usb->vxi11_mode_buf,"TAB",3)==0))
				{
					;                 //如果在TAB模式下执行开机指令则不响应
				}
				else if(((strncmp(share_usb->vxi11_mode_buf,"TAB",3) != 0) || (share_usb->anydata_fromPC_flag == Enable)) && (id_num == SET_SEL_NUM)) //如果当前模式为非TAB模式 或者 状态为"从上位机下发曲线"，则"上位机选择曲线号开机"指令无效
				{
					;
				}
				else if(id_num == SET_CC_CURR) //判断CC电流参数 20>=CC电流>=0
				{
					if((CC_CURR_DATA >= 0) && (CC_CURR_DATA <= 20)) //满足大小范围则入队列
					{
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_CC_CURR;
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = CC_CURR_DATA;
						share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
						if(share_usb->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
						{
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
							share_usb->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
						}
						printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
					}
				}
				else if(id_num == SET_CC_VOLT) //判断CC电压参数 120>=CC电压>=0
				{
					if((CC_VOLT_DATA >= 0) && (CC_VOLT_DATA <= 120)) //满足大小范围则入队列
					{
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_CC_VOLT;
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = CC_VOLT_DATA;
						share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
						if(share_usb->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
						{
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
							share_usb->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
						}
						printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
					}						
				}
				else  //为SAS/CC参数设置、在TAB模式下执行开机指令、判断模式为非"TAB"/状态为"从上位机下发曲线" 之外的其他情况
				{
					share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = id_num; //将ID存入共享内存，用于串口进程识别后发送指令到前面板
					//printf("vxi share_usb->vxi_ID_parameter_pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
					//printf("vxi ID_parameter_pack = %f\n",share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count]);
					if(scpi_parameter_count != 0) //收到了带参数的指令 如设置VOC ISC
					{
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = scpi_parameter_count; //记录带了几个参数
						for(i=0;i<scpi_parameter_count;i++) //存储参数
						{
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 + i] = scpi_parameter[i]; //将指令的参数存入共享内存
						}
					}
					else //没收到了带参数的指令 如开关机 设置模式
					{
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 0; //记录带了几个参数 0个
					}
					share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + scpi_parameter_count + 1; //指向数组的下一个存储位置		
					if(share_usb->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
					{
						share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
						share_usb->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
					}
					printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
				}
			}
			else  //如果是SAS设置参数指令
			{
				if((SAS_IMP_Flag == 1)&&(SAS_ISC_Flag == 1)&&(SAS_VMP_Flag == 1)&&(SAS_VOC_Flag == 1)) //只有四个才是点都满足了才进行斜率运算
				{
					SAS_IMP_Flag = 0; //清除标志位
					SAS_ISC_Flag = 0;
					SAS_VMP_Flag = 0;
					SAS_VOC_Flag = 0;
					//将取出的SAS设置参数进行判断 VOC>VMP ISC>IMP 130>=VOC>=0 20>=ISC>=0 VMP>=0 IMP>=0
					if((VOC_DATA>VMP_DATA)&&(ISC_DATA>IMP_DATA)&&(VOC_DATA>=0)&&(VOC_DATA<=130)&&(ISC_DATA>=0)&&(ISC_DATA<=20)&&(VMP_DATA>=0)&&(IMP_DATA>=0))  //判断大小关系
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
							if(share_usb->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
							{
								share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
								share_usb->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
							}
							
							printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_SAS_VOC;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = VOC_DATA;
							share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_usb->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
							{
								share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
								share_usb->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
							}
							printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
							
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_SAS_ISC;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = ISC_DATA;
							share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_usb->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
							{
								share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
								share_usb->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
							}
							
							printf("___vxi _ next __pack_count = %d\n",share_usb->vxi_ID_parameter_pack_count);
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = SET_SAS_IMP;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1] = 1;
							share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count + 1 + 1 ] = IMP_DATA;
							share_usb->vxi_ID_parameter_pack_count = share_usb->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_usb->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
							{
								share_usb->ID_parameter_pack[share_usb->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
								share_usb->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
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
//主函数
/******************************************/
int main()
{
	int fd; //文件描述符
	//fd = open(USB_SER,O_RDWR); //如果使用USB串口驱动，上位机发送的数据最后应该带 0d 0a
	fd = open(TMC_DEV,O_RDWR);  //打开USB设备号 USBTMC驱动
	if(fd != -1)  //打开文件成功
	{
		printf("open USB_TMC success\n");
	}
	else  //打开文件失败
	{
		printf("open USB_TMC failed\n");
	}
	
	share_id = shmget((key_t)KEY, SIZE, 0666|IPC_CREAT); //创建共享内存，用于进程之间的通讯 用于共享串口读取到的值及其长度	
	share_usb = (share_mem *) shmat(share_id,0,0);       //映射共享内存
	//printf("USB share_id:%d\n",share_id);
	
	SCPI_Init(&scpi_context,scpi_commands,&scpi_interface,scpi_units_def,0, 0, 0, 0,
			scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE); //初始化SCPI指令解析器            
	while(1)
	{
		memset(readbuf,0,150000);         //清空接收缓冲区buff
		usbread=read(fd,readbuf,150000);  //读取USB数据
		
		if(usbread > 0)  //USB数据长度
		{
			printf("\nUSB read len = %d\n",usbread); //打印数据长度
			if(usbread < 200)  //非任意曲线数据则打印
			{
				printf("USB read buf = %s\n",readbuf);
			}
			USBSCPI_Dispose(readbuf,usbread);  //分析USB接收到的字符串
			tcflush(fd, TCIFLUSH);             //清空输入输出队列
		}
		
		if(scpi_check_id != 0)  //有查询指令
		{
			USB_SCPI_Read();        //查询指令解析
			tcflush(fd, TCIFLUSH);  //清空写BUFF
			usbwrite = write(fd,USB_ReadBuf,USB_ReadCount);  //将查询到的数据上传给上位机
		}
		else  //加else避免占用cpu使用率过高
		{
			usleep(10000);
		}
	}
	close(fd);  //关闭USB设备
	return 0;
}

