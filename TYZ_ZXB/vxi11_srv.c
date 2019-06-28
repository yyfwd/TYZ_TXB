#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>
#include <error.h>
#include "vxi11.h"

int scpi_check_id = 0; //查询指令ID号
char device_info[] = {"HTXY,HTXY-TYZmachine,0001,V1.0"}; //IDN查询返回值 制造商,型号,序列号,固件版本
char SYSTem_ERRor_info[] = {"+0"}; //系统错误查询值
int anydata_get = 0; //是否收到设置任意曲线的指令
int volt_get = 0;  //接收电压数据标志位
int curr_get = 0;  //接收电流数据标志位
int volt_save = 0; //保存曲线电压标志位
int curr_save = 0; //保存曲线电流标志位
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
			sprintf(share_vxi11->volt_data,"%s",data+i+1); //提取数据部分
			//printf("%s\n",share_vxi11->volt_data);
			volt_get = 1; //接收电压数据标志位
			result = 1;
		}
		else if(ID == SET_TABL_CURR)
		{
			sprintf(share_vxi11->curr_data,"%s",data+i+1); //提取数据部分
			curr_get = 1; //接收电流数据标志位
			result = 1;
			//printf("%s\n",share_vxi11->curr_data);
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
		memset(share_vxi11->Curve_Name,0,20);
		for(k=0;k<len;k++)
		{
			share_vxi11->Curve_Name[k] = data[j+k+1];
		}
		printf("CURVE NAME:%s\n",share_vxi11->Curve_Name);
		if((IDSave >=531)&&(IDSave <=545))
		{
			sprintf(share_vxi11->volt_data,"%s",data+i+1); //提取数据部分
			//printf("%s\n",share_vxi11->volt_data);
			volt_save = 1; //保存曲线电压标志位
			result = 1;
		}
		else if((IDSave >=511)&&(IDSave <=525))
		{
			sprintf(share_vxi11->curr_data,"%s",data+i+1); //提取数据部分
			//printf("%s\n",share_vxi11->curr_data);
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
//vxi11读取函数
/******************************************/
Device_ReadResp *
device_read_1_svc(Device_ReadParms *argp, struct svc_req *rqstp)
{
	static Device_ReadResp result;
	int i;   //for循环计数
	if( argp->lid == 1 )
	{
		switch(scpi_check_id)  //查询指令ID号
		{
			case OUTPUT_STATE: //查询开关机状态
			{
				result.data.data_len = strlen(share_vxi11->vxi11_onoff_buf); //开关机状态长度
				result.data.data_val = share_vxi11->vxi11_onoff_buf; //存储开关机状态，将发送到上位机
				//printf("__vxi11_onoff  = %02x\n",result.data.data_val[0]);
			}break;
			case MODE_STATE: //查询模式设置
			{
				result.data.data_len = strlen(share_vxi11->vxi11_mode_buf); //查询模式设置长度
				result.data.data_val = share_vxi11->vxi11_mode_buf; //存储查询模式设置，将发送到上位机
			}break;
			case CHECK_SAS_IMP: //查询曲线峰值功率点处电流
			{
				//printf("  check IMP\n");
				result.data.data_len = strlen(share_vxi11->vxi11_IMP_buf); //曲线峰值功率点处电流长度
				result.data.data_val = share_vxi11->vxi11_IMP_buf; //存储曲线峰值功率点处电流，将发送到上位机
			}break;
			case CHECK_SAS_ISC: //查询短路电流
			{
				//printf("  check ISC\n");
				result.data.data_len = strlen(share_vxi11->vxi11_ISC_buf); //设置短路电流长度
				result.data.data_val = share_vxi11->vxi11_ISC_buf; 		//存储设置短路电流，将发送到上位机
			}break;
			case CHECK_SAS_VMP: //查询曲线峰值功率点处电压
			{
				//printf("  check VMP\n");
				result.data.data_len = strlen(share_vxi11->vxi11_VMP_buf); //曲线峰值功率点处电压长度
				result.data.data_val = share_vxi11->vxi11_VMP_buf; //存储曲线峰值功率点处电压，将发送到上位机
			}break;
			case CHECK_SAS_VOC: //查询开路电压
			{
				//printf("  check VOC\n");
				result.data.data_len = strlen(share_vxi11->vxi11_VOC_buf); //开路电压长度
				result.data.data_val = share_vxi11->vxi11_VOC_buf; //存储开路电压，将发送到上位机
			}break;
			case CHECK_CC_CURR: //查询CC(FIXed)模式下电流
			{
				//printf("  check CC_CURR\n");
				result.data.data_len = strlen(share_vxi11->vxi11_cc_current); //CC模式下电流长度
				result.data.data_val = share_vxi11->vxi11_cc_current; //存储CC模式下电流，将发送到上位机
			}break;
			case CHECK_CC_VOLT: //查询CC(FIXed)模式下电压
			{
				//printf("  check CC_VOLT\n");
				result.data.data_len = strlen(share_vxi11->vxi11_cc_voltage); //CC模式下电压长度
				result.data.data_val = share_vxi11->vxi11_cc_voltage;  //存储CC模式下电压，将发送到上位机//存储CC模式下电压，将发送到上位机
			}break;
			case CHECK_CURR: //查询实时电流命令
			{
				result.data.data_len = strlen(share_vxi11->vxi11_current_buf); //返回平均输出电流长度
				result.data.data_val = share_vxi11->vxi11_current_buf; //存储串口读取到的返回平均输出电流数据，将发送到上位机
			}break;
			case CHECK_VOLT: //查询实时电压命令
			{
				result.data.data_len = strlen(share_vxi11->vxi11_voltage_buf); //返回平均输出电压长度
				result.data.data_val = share_vxi11->vxi11_voltage_buf; //存储串口读取到的返回平均输出电压数据，将发送到上位机
			}break;
			case CHECK_ID: //IDN查询命令
			{
				result.data.data_len = strlen(device_info);  //IDN查询值长度
				result.data.data_val = device_info;          //IDN查询返回值 制造商,型号,序列号,固件版本
			}break;
			case CHECK_SYSTEM_ERROR: //系统错误查询命令 故障状态数据
			{
				result.data.data_len = strlen(SYSTem_ERRor_info);  //系统错误查询命令值长度
				result.data.data_val = SYSTem_ERRor_info;          //系统错误查询命令值
			}break;
			case CHECK_MACHINE_ERROR: //查询机器故障 返回故障字给上位机
			{
				result.data.data_len = strlen(share_vxi11->vxi11_error_buf); //故障状态数据长度
				result.data.data_val = share_vxi11->vxi11_error_buf; //存储故障状态数据，将发送到上位机
			}break;
			case 313: //读取模拟器的状态参数命令 暂时不起作用
			{
				
			}break;
			default://无内容转发到上位机
				result.data.data_len = 0;break;
		}
		scpi_check_id = 0;  //查询指令ID号清零
		result.error = 0;
	   	result.reason = 0x04;
	}
	return &result;
}

/******************************************/
//vxi11写函数 分析接收到的字符串
/******************************************/
Device_WriteResp *
device_write_1_svc(Device_WriteParms *argp, struct svc_req *rqstp)
{
	static Device_WriteResp  result;
	int lennum = 0;   //SCPI字符串长度
	int back;         //发送任意曲线标志
	int saveback;     //保存任意曲线标志
	int i;                    //for循环计数
	int id_num = 0;           //SCPI指令经解析后得到的ID
	char scpi_str[210] = {};  //存储有效的scpi指令字符
	char add_str[4] = "\r\n"; //字符末尾补充\r\n
	memset(scpi_str,0,210);   //清零
	printf("\nVxi11 read len = %d\n",argp->data.data_len);
	if(argp->data.data_len < 200)  //非任意曲线数据则打印
	{
		printf("Vxi11 read buf = %s\n" , argp->data.data_val);//打印接收到的SCPI字符串
	}
	lennum = argp->data.data_len;   //SCPI字符串长度 
	if(argp->lid == 1)
	{
		if((argp->data.data_len > 200 && argp->data.data_len < 10000)||(anydata_get == 1))  //任意曲线8000多个点被截包的情况,每包8128个数据
		{
			if(anydata_get == 0)
			{
				strncpy(scpi_str,argp->data.data_val,100);  //获取前100个字符用指令判断
				strncat(scpi_str,add_str,4);       //字符末尾补充\r\n
				SCPI_CHECK(scpi_str);         	   //scpi指令解析
				printf("scpi_id = %d\n",scpi_id);  //打印指令经解析后得到的ID
				id_num = scpi_id;  		  		   //存储ID
				scpi_id = 0;       		   		   //清零
				back = AnyData_Analy(argp->data.data_val,id_num);  //判断是否为发送任意曲线时发来的数据包
				saveback = AnyData_SaveMode(argp->data.data_val,id_num); //判断是否为保存任意曲线时发来的数据包
				if(back == 1 || saveback == 1) //为设置、保存任意曲线时发来的数据包
				{
					anydata_get = 1;			
					printf("AnyData_Analy OK____\n");
				}
			}
			else
			{
				if(volt_get == 1 || volt_save == 1) //接收电压数据标志位 保存曲线电压标志位 
				{
					strcat(share_vxi11->volt_data,argp->data.data_val); //拼接电压数据字符串
				}
				if(curr_get == 1 || curr_save == 1) //接收电流数据标志位 保存曲线电流标志位
				{
					strcat(share_vxi11->curr_data,argp->data.data_val); //拼接电流数据字符串
				}
				if(lennum < 8128) //接收到了最后一包 根据字符长度判断
				{
					anydata_get =0;
					if(volt_get == 1)  //发送电压数据接收完成
					{
						volt_get = 0;
						volt_curr_get++;
						//printf("%s\n",share_vxi11->volt_data);
					}
					if(curr_get ==1)   //发送电流数据接收完成
					{
						curr_get = 0;
						volt_curr_get++;
						//printf("%s\n",share_vxi11->curr_data);
					}
					
					if(volt_save == 1)  //保存电压数据接收完成
					{
						volt_save = 0;
						volt_curr_save++;
					}

					if(curr_save == 1)   //保存电流数据接收完成
					{
						curr_save = 0;
						volt_curr_save++;
					}
					
					if(volt_curr_get == 2) //电压电流数据都接收完成
					{
						volt_curr_get = 0;
						share_vxi11->anydata_fromPC_flag = Enable;  //从上位机下发曲线标志位
						printf("\nanydata line download success\n");
					}
					if(volt_curr_save == 2)  //保存的曲线数据接收完毕
					{
						volt_curr_save = 0;
						share_vxi11->anydata_savetxb_ok = Enable; //存储上位机发送的任意曲线数据到通讯板标志位
						printf("\nanydata save download success\n");
					}
				}
			}
		}
		else if(argp->data.data_len > 10000) //根据字符串长度判断是否为任意曲线包  任意曲线8000多个点整包接受的情况
		{
			strncpy(scpi_str,argp->data.data_val,100);  //获取前100个字符用指令判断
			strncat(scpi_str,add_str,4);       //字符末尾补充\r\n
			SCPI_CHECK(scpi_str);         	   //scpi指令解析
			printf("scpi_id = %d\n",scpi_id);  //打印指令经解析后得到的ID
			id_num = scpi_id;  		   //存储ID
			scpi_id = 0;       		   //清零
			back = AnyData_Analy(argp->data.data_val,id_num);  //判断是否为设置任意曲线时发来的数据包
			saveback = AnyData_SaveMode(argp->data.data_val,id_num); //判断是否为保存任意曲线时发来的数据包
			if(back == 1 || saveback == 1) //为设置、保存任意曲线时发来的数据包
			{					
				if(volt_get ==1)  //发送电压数据接收完成
				{
					volt_get = 0;
					volt_curr_get++;
					//printf("%s\n",share_vxi11->volt_data);
				}
				if(curr_get ==1)  //发送电电流数据接收完成
				{
					curr_get = 0;
					volt_curr_get++;
					//printf("%s\n",share_vxi11->curr_data);
				}
				
				if(volt_save == 1)  //保存电压数据接收完成
				{
					volt_save = 0;
					volt_curr_save++;
				}
				if(curr_save == 1)  //保存电流数据接收完成
				{
					curr_save = 0;
					volt_curr_save++;
				}
				
				if(volt_curr_get == 2) //电压电流数据都接收完成
				{
					volt_curr_get = 0;
					share_vxi11->anydata_fromPC_flag = Enable; //从上位机下发曲线标志位
					printf("\nanydata line download success\n");
				}
				if(volt_curr_save == 2)  //保存的曲线数据接收完毕
				{
					volt_curr_save = 0;
					share_vxi11->anydata_savetxb_ok = Enable; //存储上位机发送的任意曲线数据到通讯板标志位
					printf("\nanydata save download success\n");
				}
			}
		}
		else  //为普通的SCPI指令 非任意曲线包
		{
			strncpy(scpi_str,argp->data.data_val,argp->data.data_len);  //获取有效的scpi指令字符	
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
					strcpy(share_vxi11->save_serial_number,serial_number);  //存储模拟器编码
				}
				printf("___vxi _ now __pack_count = %d\n",share_vxi11->vxi_ID_parameter_pack_count);
				
				switch(id_num)  //判断ID是否是SAS、CC的参数点设置，并存储参数 
				{
					case SET_SAS_IMP:
						SAS_IMP_Flag = 1; IMP_DATA = scpi_parameter[0];break; //IMP参数是否接收完成标志位 0默认 1接收完成
					case SET_SAS_ISC:
						SAS_ISC_Flag = 1; ISC_DATA = scpi_parameter[0];break;//ISC参数是否接收完成标志位 0默认 1接收完成
					case SET_SAS_VMP:
						SAS_VMP_Flag = 1; VMP_DATA = scpi_parameter[0];break; //VMP参数是否接收完成标志位 0默认 1接收完成
					case SET_SAS_VOC:
						SAS_VOC_Flag = 1; VOC_DATA = scpi_parameter[0];break; //VOC参数是否接收完成标志位 0默认 1接收完成
					case SET_CC_CURR:
						CC_CURR_DATA = scpi_parameter[0];break;
					case SET_CC_VOLT:
						CC_VOLT_DATA = scpi_parameter[0];break;
					default:break;
				}

				//设置指令 存储ID和参数，用于uart进程分析发送串口数据
				if((id_num!=SET_SAS_IMP) && (id_num != SET_SAS_ISC) && (id_num != SET_SAS_VMP) && (id_num != SET_SAS_VOC)) //如果设置指令不是SAS四个参数的设置指令
				{
					if((id_num == OUTPUT_ON) && (strncmp(share_vxi11->vxi11_mode_buf,"TAB",3)==0)) //如果在TAB模式下执行开机指令则不响应
					{
						;
					}
					else if(((strncmp(share_vxi11->vxi11_mode_buf,"TAB",3) != 0) || (share_vxi11->anydata_fromPC_flag == Enable)) && (id_num == SET_SEL_NUM)) //如果当前模式为非TAB模式 或者 状态为"从上位机下发曲线"，则"上位机选择曲线号开机"指令无效
					{
						;
					}
					else if(id_num == SET_CC_CURR) //判断CC电流参数 20>=CC电流>=0
					{
						if((CC_CURR_DATA >= 0) && (CC_CURR_DATA <= 20)) //满足大小范围则入队列
						{
							share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = SET_CC_CURR;
							share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1] = 1;
							share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 ] = CC_CURR_DATA;
							share_vxi11->vxi_ID_parameter_pack_count = share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_vxi11->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
							{
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
								share_vxi11->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
							}
							printf("___vxi _ next __pack_count = %d\n",share_vxi11->vxi_ID_parameter_pack_count);
						}
					}
					else if(id_num == SET_CC_VOLT) //判断CC电压参数 120>=CC电压>=0
					{
						if((CC_VOLT_DATA >= 0) && (CC_VOLT_DATA <= 120)) //满足大小范围则入队列
						{
							share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = SET_CC_VOLT;
							share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1] = 1;
							share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 ] = CC_VOLT_DATA;
							share_vxi11->vxi_ID_parameter_pack_count = share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 + 1;
							if(share_vxi11->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
							{
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
								share_vxi11->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
							}
							printf("___vxi _ next __pack_count = %d\n",share_vxi11->vxi_ID_parameter_pack_count);
						}						
					}
					else  //为SAS/CC参数设置、在TAB模式下执行开机指令、判断模式为非"TAB"/状态为"从上位机下发曲线" 之外的其他情况
					{
						share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = id_num; //将ID存入共享内存，用于串口进程识别后发送指令到前面板
						//printf("vxi share_vxi11->vxi_ID_parameter_pack_count = %d\n",share_vxi11->vxi_ID_parameter_pack_count);
						//printf("vxi ID_parameter_pack = %f\n",share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count]);
						if(scpi_parameter_count != 0) //收到了带参数的指令 如设置VOC ISC
						{
							share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1] = scpi_parameter_count; //记录带了几个参数
							for(i=0;i<scpi_parameter_count;i++) //存储参数
							{
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 + i] = scpi_parameter[i]; //将指令的参数存入共享内存
							}
						}
						else //没收到了带参数的指令 如开关机 设置模式
						{
							share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1] = 0; //记录带了几个参数 0个
						}
						share_vxi11->vxi_ID_parameter_pack_count = share_vxi11->vxi_ID_parameter_pack_count + 1 + scpi_parameter_count + 1; //指向数组的下一个存储位置
						if(share_vxi11->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
						{
							share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
							share_vxi11->vxi_ID_parameter_pack_count = 0; 					//重置指向数组的起始存储位置
						}
						printf("___vxi _ next __pack_count = %d\n",share_vxi11->vxi_ID_parameter_pack_count);
					}
				}
				else  //如果是SAS设置参数指令
				{
					if((SAS_IMP_Flag == 1)&&(SAS_ISC_Flag == 1)&&(SAS_VMP_Flag == 1)&&(SAS_VOC_Flag == 1)) //只有四个参数点都接收完成才进行斜率运算
					{
						SAS_IMP_Flag = 0; //清除标志位
						SAS_ISC_Flag = 0;
						SAS_VMP_Flag = 0;
						SAS_VOC_Flag = 0;
						//将取出的SAS设置参数进行判断 VOC>VMP ISC>IMP 130>=VOC>=0 20>=ISC>=0 VMP>=0 IMP>=0
						if((VOC_DATA>VMP_DATA)&&(ISC_DATA>IMP_DATA)&&(VOC_DATA>=0)&&(VOC_DATA<=130)&&(ISC_DATA>=0)&&(ISC_DATA<=20)&&(VMP_DATA>=0)&&(IMP_DATA>=0))  //判断大小关系
						{
							printf("SAS IV success\n");
							SAS_Kn = (IMP_DATA/(VOC_DATA-VMP_DATA))+(ISC_DATA/VOC_DATA);  //计算SAS的I-V斜率
							printf("SAS IV K:%.9f\n",SAS_Kn);
							if(SAS_Kn < 1.6) //斜率满足要求则将SAS四个参数入队列
							{
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = SET_SAS_VMP;
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1] = 1;
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 ] = VMP_DATA;
								share_vxi11->vxi_ID_parameter_pack_count = share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 + 1;
								if(share_vxi11->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
								{
									share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
									share_vxi11->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
								}
								printf("___vxi _ next __pack_count = %d\n",share_vxi11->vxi_ID_parameter_pack_count);
								
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = SET_SAS_VOC;
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1] = 1;
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 ] = VOC_DATA;
								share_vxi11->vxi_ID_parameter_pack_count = share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 + 1;
								if(share_vxi11->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
								{
									share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
									share_vxi11->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
								}
								printf("___vxi _ next __pack_count = %d\n",share_vxi11->vxi_ID_parameter_pack_count);
								
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = SET_SAS_ISC;
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1] = 1;
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 ] = ISC_DATA;
								share_vxi11->vxi_ID_parameter_pack_count = share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 + 1;
								if(share_vxi11->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
								{
									share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
									share_vxi11->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
								}
								printf("___vxi _ next __pack_count = %d\n",share_vxi11->vxi_ID_parameter_pack_count);
								
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = SET_SAS_IMP;
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1] = 1;
								share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 ] = IMP_DATA;
								share_vxi11->vxi_ID_parameter_pack_count = share_vxi11->vxi_ID_parameter_pack_count + 1 + 1 + 1;
								if(share_vxi11->vxi_ID_parameter_pack_count >= 100) //指向下一个ID
								{
									share_vxi11->ID_parameter_pack[share_vxi11->vxi_ID_parameter_pack_count] = -1; //存储特殊数字-1 表示数组一个循环完成
									share_vxi11->vxi_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
								}
								printf("___vxi _ next __pack_count = %d\n",share_vxi11->vxi_ID_parameter_pack_count);
							}
						}
							
					}
				}
			}			
		}
		result.error = 0; 
		result.size = lennum;
	}
	free(argp->data.data_val);
	argp->data.data_val = NULL;	
	return &result;
}

Create_LinkResp *create_link_1_svc(Create_LinkParms *argp, struct svc_req *rqstp)
{
	static Create_LinkResp  result;
	//printf("run create_link_1_svc()\n");
	//printf("clientId = %ld\n", argp->clientId);
	//printf("argp->device = %s\n", argp->device);
	if(strcmp(argp->device, "inst0") == 0)
	{
		result.error = 0;
		result.lid = 1;
		result.abortPort =111;            //返回值为中断端口号通过端口映射可以查询
		//result.maxRecvSize = 1250;        //该此值可最大接收1186，更改此值可以增大接收BUF字符
		result.maxRecvSize = 150000;       //该此值可最大接收8128
		return &result;
	}
	else
	{
		result.error = 9;
		result.lid = 522133279;
		result.abortPort = 0;
		result.maxRecvSize = 0;
		return &result;
	}
}

Device_Error *device_abort_1_svc(Device_Link *argp, struct svc_req *rqstp)
{
	static Device_Error  result;
	//printf("run device_abort_1_svc()\n");
	return &result;
}

Device_ReadStbResp *
device_readstb_1_svc(Device_GenericParms *argp, struct svc_req *rqstp)
{
	static Device_ReadStbResp  result;	
	return &result;
}

Device_Error *
device_trigger_1_svc(Device_GenericParms *argp, struct svc_req *rqstp)
{
	static Device_Error  result;
	//printf("run device_trigger_1_svc()\n");
	/*
	 * insert server code here
	 */
	return &result;
}

Device_Error *
device_clear_1_svc(Device_GenericParms *argp, struct svc_req *rqstp)
{
	static Device_Error  result;
	//printf("run device_clear_1_svc()\n");
	/*
	 * insert server code here
	 */
	return &result;
}

Device_Error *
device_remote_1_svc(Device_GenericParms *argp, struct svc_req *rqstp)
{
	static Device_Error  result;
	//printf("run device_remote_1_svc()\n");
	/*
	 * insert server code here
	 */
	return &result;
}

Device_Error *
device_local_1_svc(Device_GenericParms *argp, struct svc_req *rqstp)
{
	static Device_Error  result;
	//printf("run device_local_1_svc()\n");
	/*
	 * insert server code here
	 */
	return &result;
}

Device_Error *
device_lock_1_svc(Device_LockParms *argp, struct svc_req *rqstp)
{
	static Device_Error  result;
	//printf("run device_lock_1_svc()\n");
	/*
	 * insert server code here
	 */
	return &result;
}

Device_Error *
device_unlock_1_svc(Device_Link *argp, struct svc_req *rqstp)
{
	static Device_Error  result;
	//printf("run device_unlock_1_svc()\n");
	/*
	 * insert server code here
	 */
	return &result;
}

Device_Error *
device_enable_srq_1_svc(Device_EnableSrqParms *argp, struct svc_req *rqstp)
{
	static Device_Error  result;

	//printf("run device_enable_srq_1_svc()\n");
	/*
	 * insert server code here
	 */
	return &result;
}

Device_DocmdResp *
device_docmd_1_svc(Device_DocmdParms *argp, struct svc_req *rqstp)
{
	static Device_DocmdResp  result;

	//printf("run device_docmd_1_svc()\n");
	/*
	 * insert server code here
	 */

	return &result;
}

Device_Error *
destroy_link_1_svc(Device_Link *argp, struct svc_req *rqstp)
{
	static Device_Error  result;
	//printf("run destroy_link_1_svc() argp = %ld \n",(long)argp);
//	if(*argp == 1)
//	{
//		result.error = 0;
//	}
	return &result;
}

Device_Error *
create_intr_chan_1_svc(Device_RemoteFunc *argp, struct svc_req *rqstp)
{
	static Device_Error  result;

	//printf("run create_intr_chan_1_svc()\n");
	/*
	 * insert server code here
	 */

	return &result;
}

Device_Error *
destroy_intr_chan_1_svc(void *argp, struct svc_req *rqstp)
{
	static Device_Error  result;

	//printf("run destroy_intr_chan_1_svc()\n");
	/*
	 * insert server code here
	 */

	return &result;
}

void *
device_intr_srq_1_svc(Device_SrqParms *argp, struct svc_req *rqstp)
{
	static char * result;

	//printf("run device_intr_srq_1_svc()\n");
	/*
	 * insert server code here
	 */

	return (void *) &result;
}

//用于主函数vxi11的初始化
void device_async_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	//printf("run device_async_1()\n");
	union {
		Device_Link device_abort_1_arg;
	} argument;
	char *result;
	xdrproc_t _xdr_argument, _xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;

	case device_abort:
		_xdr_argument = (xdrproc_t) xdr_Device_Link;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) device_abort_1_svc;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	memset ((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		exit (1);
	}
	return;
}

//用于主函数vxi11的初始化
void device_core_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	//printf("run device_core_1() %ld\n" ,(unsigned long)rqstp->rq_proc);
	union {
		Create_LinkParms create_link_1_arg;
		Device_WriteParms device_write_1_arg;
		Device_ReadParms device_read_1_arg;
		Device_GenericParms device_readstb_1_arg;
		Device_GenericParms device_trigger_1_arg;
		Device_GenericParms device_clear_1_arg;
		Device_GenericParms device_remote_1_arg;
		Device_GenericParms device_local_1_arg;
		Device_LockParms device_lock_1_arg;
		Device_Link device_unlock_1_arg;
		Device_EnableSrqParms device_enable_srq_1_arg;
		Device_DocmdParms device_docmd_1_arg;
		Device_Link destroy_link_1_arg;
		Device_RemoteFunc create_intr_chan_1_arg;
	} argument;
	char *result;
	xdrproc_t _xdr_argument, _xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;

	case create_link:
		_xdr_argument = (xdrproc_t) xdr_Create_LinkParms;
		_xdr_result = (xdrproc_t) xdr_Create_LinkResp;
		local = (char *(*)(char *, struct svc_req *)) create_link_1_svc;
		break;

	case device_write:
		_xdr_argument = (xdrproc_t) xdr_Device_WriteParms;
		_xdr_result = (xdrproc_t) xdr_Device_WriteResp;
		local = (char *(*)(char *, struct svc_req *)) device_write_1_svc;
		break;

	case device_read:
		_xdr_argument = (xdrproc_t) xdr_Device_ReadParms;
		_xdr_result = (xdrproc_t) xdr_Device_ReadResp;
		local = (char *(*)(char *, struct svc_req *)) device_read_1_svc;
		break;

	case device_readstb:
		_xdr_argument = (xdrproc_t) xdr_Device_GenericParms;
		_xdr_result = (xdrproc_t) xdr_Device_ReadStbResp;
		local = (char *(*)(char *, struct svc_req *)) device_readstb_1_svc;
		break;

	case device_trigger:
		_xdr_argument = (xdrproc_t) xdr_Device_GenericParms;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) device_trigger_1_svc;
		break;

	case device_clear:
		_xdr_argument = (xdrproc_t) xdr_Device_GenericParms;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) device_clear_1_svc;
		break;


	case device_remote:
		_xdr_argument = (xdrproc_t) xdr_Device_GenericParms;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) device_remote_1_svc;
		break;

	case device_local:
		_xdr_argument = (xdrproc_t) xdr_Device_GenericParms;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) device_local_1_svc;
		break;

	case device_lock:
		_xdr_argument = (xdrproc_t) xdr_Device_LockParms;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) device_lock_1_svc;
		break;

	case device_unlock:
		_xdr_argument = (xdrproc_t) xdr_Device_Link;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) device_unlock_1_svc;
		break;

	case device_enable_srq:
		_xdr_argument = (xdrproc_t) xdr_Device_EnableSrqParms;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) device_enable_srq_1_svc;
		break;

	case device_docmd:
		_xdr_argument = (xdrproc_t) xdr_Device_DocmdParms;
		_xdr_result = (xdrproc_t) xdr_Device_DocmdResp;
		local = (char *(*)(char *, struct svc_req *)) device_docmd_1_svc;
		break;

	case destroy_link:
		_xdr_argument = (xdrproc_t) xdr_Device_Link;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) destroy_link_1_svc;
		break;

	case create_intr_chan:
		_xdr_argument = (xdrproc_t) xdr_Device_RemoteFunc;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) create_intr_chan_1_svc;
		break;

	case destroy_intr_chan:
		_xdr_argument = (xdrproc_t) xdr_void;
		_xdr_result = (xdrproc_t) xdr_Device_Error;
		local = (char *(*)(char *, struct svc_req *)) destroy_intr_chan_1_svc;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	memset ((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		exit (1);
	}
	return;
}

//用于主函数vxi11的初始化
void device_intr_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	//printf("run device_intr_1()\n");
	union {
		Device_SrqParms device_intr_srq_1_arg;
	} argument;
	char *result;
	xdrproc_t _xdr_argument, _xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;

	case device_intr_srq:
		_xdr_argument = (xdrproc_t) xdr_Device_SrqParms;
		_xdr_result = (xdrproc_t) xdr_void;
		local = (char *(*)(char *, struct svc_req *)) device_intr_srq_1_svc;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	memset ((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		exit (1);
	}
	return;
}