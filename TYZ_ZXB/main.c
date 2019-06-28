#include "vxi11.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>  //使用定时器
#include <sys/ipc.h>
#include <sys/shm.h>

#define KEY   1234    //一个用来标识共享内存块的键值大小
#define SIZE  600000  //指定了所申请的内存块的大小 接收任意曲线8千多个数据时注意该值大小

extern char device_info[]; //IDN查询返回值 制造商,型号,序列号,固件版本

int share_id;             //共享内存的ID
char modbuf[200] = {'0'}; //存储要发送给串口的数据
int  modbuf_len = 0;      //要发送给串口的数据长度

int uart_send_flag = 0;    //串口发送标志位 排除web发送串口数据时返回的信息
int send_set_first = 0;    //设置指令发送后，若无新的设置指令，间隔时间再发送查询指令
int send_check_first = 0;  //查询指令发送后，若有新的设置指令，间隔时间再发送设置指令

struct timeval time_count; //保存时间
long long msec_last = 0;   //保存上一次的毫秒
long long msec_now = 0;    //保存当前的毫秒
int time_one = 0;          //设置指令计时
int time_two = 0,time_there = 0; //查询指令计时
int time_second_task = 0,time_third_task = 0; //循环任务
char web_str[1000];        //web字符串
int uart_ID_parameter_pack_count = 0; //ID和参数包个数统计 用于uart进程
int first_IP_check = 0;    //上电后先发送一次IP查询命令
int time_sendanydata = 0;  //发送任意曲线数据包计时

char TableMode_Curve_SendFlag = Enable; //发送Table模式下曲线总条数标志位 收到前面板回复则停止发送
char PC_UseCurveOnOff_Flag = Disable;   //上位机选择曲线号开机标志位
int anydata_sendfromtxb_ok = Disable;   //前面板选择曲线号，从通讯板下发曲线标志位
int chose_line_donwload = Disable;      //上位机下发曲线开机志位
int chose_num_donwload = Disable;       //上位机选择曲线号开机志位
char TableMode_Flag = Disable;          //Table模式标志位 
char GPIB_Flag = Disable;               //GPIB地址查询标志位

/******************************************/
//对收到的串口数据进行解析
/******************************************/
void Rx_uart(unsigned char *buf,int uart_len)
{
	char str[1000];     //存放拼接起来的IP地址设置字符串
	char adr[20];       //存放整数型转换为字符型的结果
	char error[6];      //存放故障字
	char error_add[1];  //存放追加的故障字
	memset(str,0,1000); //清空字符串
	memset(error,0,6);  //清空字符串
	int len = 0;        //参数个数
	int i;              //循环计数
	switch(buf[1])  //判断功能码
	{
		case 0x10:
		{
			if(buf[2] == 0x00 && buf[3] != 0x98 && buf[3] != 0x8b && uart_send_flag == 1) //各种设置指令(排除设置LAN标志指令、转发上位机设置的任意曲线数据包、web发送串口数据时返回的信息) 如果指令设置成功，指向存储设置指令的数组的下一位，否则继续发送上一个设置指令
			{
				uart_send_flag = 0; //串口发送标志位 排除web发送串口数据时返回的信息
				time_one = 0;       //收到设置指令返回值后，设置指令计时清零
			
				len = share_uart->ID_parameter_pack[uart_ID_parameter_pack_count+1]; //参数个数
				share_uart->ID_parameter_pack[uart_ID_parameter_pack_count] = 0;     //清除已经处理完成的ID
				for(i=0;i<len;i++) //清除参数
				{
					share_uart->ID_parameter_pack[uart_ID_parameter_pack_count + 1 + 1 + i] = 0;
				}
				share_uart->ID_parameter_pack[uart_ID_parameter_pack_count + 1] = 0; //清除参数个数
				
				uart_ID_parameter_pack_count = uart_ID_parameter_pack_count + 1 + len + 1;  //指向数组的下一个存储位置
				
				if(share_uart->ID_parameter_pack[uart_ID_parameter_pack_count] == -1) //特殊数字-1 表示数组一个循环完成
				{
					share_uart->ID_parameter_pack[uart_ID_parameter_pack_count] = 0; //清除特殊数字
					uart_ID_parameter_pack_count = 0; //重置指向数组的起始存储位置
				}
			}			

			//发送Table模式下曲线总条数给前面板
			if(buf[2] == 0x00 && buf[3] == 0x9d && buf[4] == 0x00 && buf[5] == 0x01)
			{
				TableMode_Curve_SendFlag = Disable;  //发送Table模式下曲线总条数标志位 收到前面板回复则停止发送
			}
			
			//发送曲线号给前面板
			if(buf[2] == 0x00 && buf[3] == 0x9E && buf[4] == 0x00 && buf[5] == 0x01)
			{
				PC_UseCurveOnOff_Flag = Enable; //上位机选择曲线号开机标志位 发送曲线号给前面板并收到回复后 开始发送曲线给前面板
			}

			//转发上位机设置的任意曲线数据包
			if(buf[2] == 0x00 && buf[3] == 0x8b && buf[4] == 0x00 && buf[5] == 0x41)
			{
				//从上位机下发曲线开机
				if(share_uart->anydata_fromPC_flag == Enable)   //从上位机下发曲线标志位
				{
					send_count++; //发送的数据包帧数目加1 一共128帧
					if(send_count == 128) //全部发送完成
					{
						send_count = 0;   //发送第几帧的数据包
						anydata_take = 0; //是否已经从字符串中提取数值
						share_uart->anydata_fromPC_flag = Disable; //从上位机下发曲线标志位 清零
						chose_line_donwload = Disable;      //上位机下发曲线开机志位
					}
				}
				
				//上位机选择曲线号开机 发送曲线号给前面板并收到回复后 开始发送曲线给前面板
				if( PC_UseCurveOnOff_Flag == Enable) //上位机选择曲线号开机标志位
				{
					send_TXB_count++; //发送的数据包帧数目加1 一共128帧
					if(send_TXB_count == 128) //全部发送完成
					{
						send_TXB_count = 0;     //发送第几帧的数据包
						anydata_file_take = 0;  //是否已经从文件中提取数值
						PC_UseCurveOnOff_Flag = Disable; //上位机选择曲线号开机标志位 清零
						chose_num_donwload = Disable;    //上位机选择曲线号开机志位
					}	
				}
				
				//前面板选择曲线号开机 从通讯板下发曲线
				if(anydata_sendfromtxb_ok == Enable) //前面板选择曲线号，从通讯板下发曲线标志位
				{
					send_TXB_count++; //发送的数据包帧数目加1 一共128帧
					if(send_TXB_count == 128) //全部发送完成
					{
						send_TXB_count = 0;     //发送第几帧的数据包
						anydata_file_take = 0;  //是否已经从文件中提取数值
						anydata_sendfromtxb_ok = Disable; //前面板选择曲线号，从通讯板下发曲线标志位 清零
					}
				}
			}			
		}break;
		case 0x03:
		{
			if(buf[2] == 0x0C) //设置IP
			{
				//判断串口发送过来的IP是否改了，如果改了就写入文件并重启网卡
				if(buf[3] != LAN.Address[0] || buf[4] != LAN.Address[1] || buf[5] != LAN.Address[2] || buf[6] != LAN.Address[3]
				|| buf[7] != LAN.Netmask[0]|| buf[8] != LAN.Netmask[1] || buf[9] != LAN.Netmask[2] || buf[10] != LAN.Netmask[3]
				|| buf[11] != LAN.Gateway[0] || buf[12] != LAN.Gateway[1] || buf[13] != LAN.Gateway[2] || buf[14] != LAN.Gateway[3])
				{
				//设置IP地址
					//sprintf 会将整数型转换为字符型，同时组合成字符串 注意接收到的buf[3]~buf[14]为十六进制 %d会转换为十进制
					//sed 用来编辑文件，可指定要编辑的行数 -i表示对原文件进行修改
					//sprintf(str,"sed -i '25c\\address %d.%d.%d.%d' /etc/network/interfaces",buf[3],buf[4],buf[5],buf[6]); //网口0
					sprintf(str,"sed -i '32c\\address %d.%d.%d.%d' /etc/network/interfaces",buf[3],buf[4],buf[5],buf[6]);//网口1
					printf("%s\n", str);  //打印命令行语句 如 sed -i '25c\address 192.168.137.185' /etc/network/interfaces
					system(str);          //system函数 执行命令行语句
				//设置子网掩码
					//sprintf(str,"sed -i '27c\\netmask %d.%d.%d.%d' /etc/network/interfaces",buf[7],buf[8],buf[9],buf[10]); //网口0
					sprintf(str,"sed -i '34c\\netmask %d.%d.%d.%d' /etc/network/interfaces",buf[7],buf[8],buf[9],buf[10]);//网口1
					printf("%s\n", str);  //打印命令行语句 如 sed -i '27c\netmask 255.255.255.0' /etc/network/interfaces
					system(str);          //system函数 执行命令行语句
				//设置网关
					//sprintf(str,"sed -i '26c\\gateway %d.%d.%d.%d' /etc/network/interfaces",buf[11],buf[12],buf[13],buf[14]); //网口0
					sprintf(str,"sed -i '33c\\gateway %d.%d.%d.%d' /etc/network/interfaces",buf[11],buf[12],buf[13],buf[14]);//网口1
					printf("%s\n", str);  //打印命令行语句 如 sed -i '26c\gateway 192.168.137.1' /etc/network/interfaces
					system(str);          //system函数 执行命令行语句

					system("sync"); //用来强制将内存缓冲区中的数据立即写入磁盘中，即保存修改后的数据到原文件中
					system("/etc/init.d/networking restart"); //重启网卡

					LAN.Address[0] = buf[3];     //存储设置过的IP 用于当前面板设置的IP发送变化时比较
					LAN.Address[1] = buf[4];
					LAN.Address[2] = buf[5];
					LAN.Address[3] = buf[6];
					LAN.Netmask[0] = buf[7];
					LAN.Netmask[1] = buf[8];
					LAN.Netmask[2] = buf[9];
					LAN.Netmask[3] = buf[10];
					LAN.Gateway[0] = buf[11];
					LAN.Gateway[1] = buf[12];
					LAN.Gateway[2] = buf[13];
					LAN.Gateway[3] = buf[14];
				}
			}
			else if(buf[2] == 0x12 && buf[7] == 0x00 && buf[8] == 0x00) //读取模拟器的状态参数 与返回SAS各个值指令分开
			{
				//将串口接收到的数据存入共享内存，用于VXI11发送到上位机
				sprintf(share_uart->vxi11_current_buf, "%f", (float)(((buf[6] + (buf[5] << 8)) + ((buf[4] + (buf[3] << 8)) << 16))/10000.0));    //实时电流
				sprintf(share_uart->vxi11_voltage_buf, "%f", (float)(((buf[12] + (buf[11] << 8)) + ((buf[10] + (buf[9] << 8)) << 16))/10000.0)); //实时电压
				
				share_uart->curve_Num = buf[19];             //存储曲线号
				share_uart->Table_OnOff_Flag = buf[20];      //前面板发送Table模式开机标志位
				if(share_uart->anydata_fromPC_flag == Disable && PC_UseCurveOnOff_Flag == Disable)   //如果没有从上位机发任意曲线开机、上位机选择曲线号开机,则监测是否有从前面板选择曲线号开机
				{
					if(share_uart->Table_OnOff_Flag == 0x01)     //前面板发送Table模式开机标志位
					{
						if((share_uart->curve_Num >= 1)&&(share_uart->curve_Num <= 15))  //暂时最多存15条后期可修改
						{
							anydata_sendfromtxb_ok = Enable;  //前面板选择曲线号，从通讯板下发曲线标志位
						}
						else
						{
							anydata_sendfromtxb_ok = Disable;
						}
					}
				}
				
				//判断故障字 可能发生一个或多个故障 或者没有故障
				if((buf[15] & 0x20) == 0x00)    //0表示发生风扇堵转故障
				{
					strcpy(error_add,"F");      //故障字为F
					strncat(error,error_add,1); //追加故障字
				}
				if((buf[15] & 0x10) == 0x00)    //0表示过流故障
				{
					strcpy(error_add,"C");      //故障字为C
					strncat(error,error_add,1); //追加故障字
				}
				if((buf[15] & 0x08) == 0x00)    //0表示发生过压故障
				{
					strcpy(error_add,"V");      //故障字为V
					strncat(error,error_add,1); //追加故障字
				}
				if((buf[15] & 0x04) == 0x00)    //0表示发生过温故障
				{
					strcpy(error_add,"T");      //故障字为T
					strncat(error,error_add,1); //追加故障字
				}
				if((buf[15] & 0x02) == 0x00)    //0表示发生风扇辅助源欠压故障
				{
					strcpy(error_add,"A");      //故障字为A
					strncat(error,error_add,1); //追加故障字
				}
				if((buf[15] & 0x40) == 0x40)    //1表示前面板与FPGA的通讯断开
				{
					strcpy(error_add,"D");      //故障字为D
					strncat(error,error_add,1); //追加故障字
				}
				if((buf[15] & 0x7E) == 0x3E)    //无故障发生 多位一起判断
				{
					strcpy(error_add,"0");      //故障字为0
					strncat(error,error_add,1);
				}
				sprintf(share_uart->vxi11_error_buf, "%s", error); //返回故障状态字 存入共享内存
			}
			else if(buf[2] == 0x06) //CC模式下的电流
			{
				//将串口接收到的数据存入共享内存，用于VXI11发送到上位机
				sprintf(share_uart->vxi11_cc_current, "%f", (float)(((buf[8] + (buf[7] << 8)) + ((buf[6] + (buf[5] << 8)) << 16))/10000.0)); //将浮点数转为字符串
			}
			else if(buf[2] == 0x04) //CC模式下的电压
			{
				//将串口接收到的数据存入共享内存，用于VXI11发送到上位机
				sprintf(share_uart->vxi11_cc_voltage, "%f", (float)(((buf[6] + (buf[5] << 8)) + ((buf[4] + (buf[3] << 8)) << 16))/10000.0)); //将浮点数转为字符串
			}
			else if(buf[2] == 0x10) //返回SAS各个值，需除以10000再发送给上位机 与读取模拟器的状态参数指令分开
			{
				//将串口接收到的数据存入共享内存，用于VXI11发送到上位机
				sprintf(share_uart->vxi11_IMP_buf, "%f", (float)(((buf[6] + (buf[5] << 8)) + ((buf[4] + (buf[3] << 8)) << 16))/10000.0)); //曲线峰值功率点处电流
				sprintf(share_uart->vxi11_VMP_buf, "%f", (float)(((buf[10] + (buf[9] << 8)) + ((buf[8] + (buf[7] << 8)) << 16))/10000.0)); //曲线峰值功率点处电压
				sprintf(share_uart->vxi11_ISC_buf, "%f", (float)(((buf[14] + (buf[13] << 8)) + ((buf[12] + (buf[11] << 8)) << 16))/10000.0)); //短路电流
				sprintf(share_uart->vxi11_VOC_buf, "%f", (float)(((buf[18] + (buf[17] << 8)) + ((buf[16] + (buf[15] << 8)) << 16))/10000.0)); //开路电压
			}
			else if(buf[2] == 0x02)     //返回模拟器开关机状态/模式   或者是GPIB_ADDRID
			{
				if(GPIB_Flag == Enable) //GPIB地址查询标志位 避免指令解析出错
				{
					share_uart->gpib_addr_id = (int)((int)(buf[3]<<8)+buf[4]); //存储前面板设置的GPIB地址
					GPIB_Flag = Disable; //GPIB地址查询标志位
					//printf("GPIB_ID = %d\n",share_uart->gpib_addr_id);
				}
				else
				{
					sprintf(share_uart->vxi11_onoff_buf, "%d", buf[3]); //开关机状态字节
					//printf("__uart_onoff  = %d\n",buf[3]);
					if(buf[4] == 0x00) //CC(Fixed)模式
					{
						TableMode_Flag = Disable;  //Table模式标志位 失能
						share_uart->anydata_fromPC_flag = Disable;  //从上位机下发曲线标志位 在非Table模式下不起作用
						PC_UseCurveOnOff_Flag = Disable;            //上位机选择曲线号开机标志位
						share_uart->anydata_savetxb_ok = Disable;   //存储上位机发送的任意曲线数据到通讯板标志位
						sprintf(share_uart->vxi11_mode_buf, "FIX"); //模式状态字节
					}
					else if(buf[4] == 0x01) //SAS模式
					{
						TableMode_Flag = Disable;  //Table模式标志位 失能
						share_uart->anydata_fromPC_flag = Disable;  //从上位机下发曲线标志位 在非Table模式下不起作用
						PC_UseCurveOnOff_Flag = Disable;            //上位机选择曲线号开机标志位
						share_uart->anydata_savetxb_ok = Disable;   //存储上位机发送的任意曲线数据到通讯板标志位
						sprintf(share_uart->vxi11_mode_buf, "SAS"); //模式状态字节
					}
					else if(buf[4] == 0x02) //Table模式
					{
						TableMode_Flag = Enable;  //Table模式标志位 使能
						sprintf(share_uart->vxi11_mode_buf, "TAB"); //模式状态字节
					}
				}
			}
			
		}break;
		case 0x83:case 0x90: //CRC校验出错返回数据
		{
			printf("CRC back error\n");
		}break;
		default:break;
	}
}

/******************************************/
//根据查询ID结果，进行modbuf协议组帧
/******************************************/
void Modbus_rtu(int id)
{
	long int senddata,Kdata,Bdata,ISC_high,ISC_low,Curve_Num; //存储参数计算结果
	double scpi_parameter[100]; //存储带参数的指令的参数值
	char ISC_str[100];  		//ISC字符串
	int i;           			//for循环计数
	int CRC_num = 0; 			//CRC校验结果
	modbuf[0] = 0x00;			//设备节点号 固定0
	switch(id)
	{
	//启用指定的输出通道（开机） 设置三种模式：CC(FIXed)/SAS/TABLE
		case OUTPUT_ON:case MODE_CC:case MODE_SAS:case MODE_Table:
		{
			switch(id)
			{
				case OUTPUT_ON:
				{
					printf("\nuart send command : set OUTPUT_ON\n");
				}break;
				case MODE_CC:
				{
					printf("\nuart send command : set MODE_CC\n");
				}break;
				case MODE_SAS:
				{
					printf("\nuart send command : set MODE_SAS\n");
				}break;
				case MODE_Table:
				{
					printf("\nuart send command : set MODE_Table\n");
				}break;
				default:break;
			}
			
			modbuf[1]=0x10;	
			modbuf[2]=id/256;
			modbuf[3]=id%256;
			modbuf[4]=0x00;
			modbuf[5]=0x01;
			modbuf[6]=0x02;
			modbuf[7]=0x00;
			modbuf[8]=0x00;
			CRC_num = Modbus_CRC16(modbuf,9); //CRC校验
			modbuf[9]=CRC_num/256;
			modbuf[10]=CRC_num%256;
			modbuf[11]=0x0d;
			modbuf[12]=0x0a;
			modbuf_len=13;
		}break;
	//禁止指定的输出通道（关机）
		case OUTPUT_OFF:
		{
			printf("\nuart send command : set OUTPUT_OFF\n");
			modbuf[1]=0x10;
			modbuf[2]=0x00;
			modbuf[3]=0x9C;
			modbuf[4]=0x00;
			modbuf[5]=0x01;
			modbuf[6]=0x02;
			modbuf[7]=0x00;
			modbuf[8]=0x01;
			CRC_num = Modbus_CRC16(modbuf,9); //CRC校验模块
			modbuf[9]=CRC_num/256;
			modbuf[10]=CRC_num%256;
			modbuf[11]=0x0d;
			modbuf[12]=0x0a;
			modbuf_len=13;
		}break;
	//设置曲线峰值功率点处电流 设置短路电流 设置曲线峰值功率点处电压 设置开路电压 设置CC(FIXed)模式下电流 设置CC(FIXed)模式下电压
		case SET_SAS_IMP:case SET_SAS_ISC:case SET_SAS_VMP:case SET_SAS_VOC:case SET_CC_CURR:case SET_CC_VOLT:
		{
			scpi_parameter[0] = share_uart->ID_parameter_pack[uart_ID_parameter_pack_count + 2]; //从共享内存中读取参数 此指令只需1个参数
			
			switch(id)
			{
				case SET_SAS_IMP:
				{
					sprintf(share_uart->vxi11_IMP_buf, "%f", scpi_parameter[0]); //存入共享内存 可在上位机查询参数时，快速查到设置的值
					printf("\nuart send command : set SET_SAS_IMP\n");
				}break;
				case SET_SAS_ISC:
				{
					sprintf(share_uart->vxi11_ISC_buf, "%f", scpi_parameter[0]); //存入共享内存 可在上位机查询参数时，快速查到设置的值
					printf("\nuart send command : set SET_SAS_ISC\n");
				}break;
				case SET_SAS_VMP:
				{
					sprintf(share_uart->vxi11_VMP_buf, "%f", scpi_parameter[0]); //存入共享内存 可在上位机查询参数时，快速查到设置的值
					printf("\nuart send command : set SET_SAS_VMP\n");
				}break;
				case SET_SAS_VOC:
				{
					sprintf(share_uart->vxi11_VOC_buf, "%f", scpi_parameter[0]); //存入共享内存 可在上位机查询参数时，快速查到设置的值
					printf("\nuart send command : set SET_SAS_VOC\n");
				}break;
				case SET_CC_CURR:
				{
					sprintf(share_uart->vxi11_cc_current, "%f", scpi_parameter[0]); //存入共享内存 可在上位机查询参数时，快速查到设置的值
					printf("\nuart send command : set SET_CC_CURR\n");
				}break;
				case SET_CC_VOLT:
				{
					sprintf(share_uart->vxi11_cc_voltage, "%f", scpi_parameter[0]); //存入共享内存 可在上位机查询参数时，快速查到设置的值
					printf("\nuart send command : set SET_CC_VOLT\n");
				}break;
				default:break;
			}
			printf("uart parameter = %f\n",scpi_parameter[0]);
			
			senddata=(long int)(scpi_parameter[0]*10000);
			modbuf[1]=0x10;
			modbuf[2]=id/256;
			modbuf[3]=id%256;
			modbuf[4]=0x00;
			modbuf[5]=0x02;
			modbuf[6]=0x04;
			modbuf[7]=senddata/0x1000000;
			modbuf[8]=(senddata/0x10000)%0x100;
			modbuf[9]=(senddata%0x10000)/0x100;
			modbuf[10]=(senddata%256);
			CRC_num = Modbus_CRC16(modbuf,11); //CRC校验
			modbuf[11]=CRC_num/256;
			modbuf[12]=CRC_num%256;
			modbuf[13]=0x0d;
			modbuf[14]=0x0a;
			modbuf_len=15;
		}break;
	//上位机选择曲线号开机 发送完成并收到前面板回复，就开始发送曲线给前面板
		case SET_SEL_NUM:
		{
			printf("\nuart send command : set SET_SEL_NUM\n");
			scpi_parameter[0] = share_uart->ID_parameter_pack[uart_ID_parameter_pack_count + 2]; //从共享内存中读取参数 此指令只需1个参数
			printf("uart parameter = %f\n",scpi_parameter[0]);
			Curve_Num = (long int)scpi_parameter[0];	//曲线号
			share_uart->curve_Num = Curve_Num;
			modbuf[1]=0x10;
			modbuf[2]=0;
			modbuf[3]=0x9E;
			modbuf[4]=0x00;
			modbuf[5]=0x01;
			modbuf[6]=0x02;
			modbuf[7]=Curve_Num/256;
			modbuf[8]=Curve_Num%256;
			CRC_num = Modbus_CRC16(modbuf,9); //CRC校验
			modbuf[9]=CRC_num/256;
			modbuf[10]=CRC_num%256;
			modbuf[11]=0x0d;
			modbuf[12]=0x0a;
			modbuf_len = 13;
		}break;
	//计算曲线
		case 141:
		{
			modbuf[1]=0x10;
			modbuf[2]=id/256;
			modbuf[3]=id%256;
			modbuf[4]=0x00;
			modbuf[5]=0x01;
			modbuf[6]=0x02;
			modbuf[7]=0x00;
			modbuf[8]=0x01;
			CRC_num = Modbus_CRC16(modbuf,9); //CRC校验
			modbuf[9]=CRC_num/256;
			modbuf[10]=CRC_num%256;
			modbuf[11]=0x0d;
			modbuf[12]=0x0a;
			modbuf_len=13;
		}break;
	//故障清零
		case 142:
		{
			modbuf[0]=0x00;
			modbuf[1]=0x10;	
			modbuf[2]=id/256;
			modbuf[3]=id%256;
			modbuf[4]=0x00;
			modbuf[5]=0x01;
			modbuf[6]=0x02;
			modbuf[7]=0x00;
			modbuf[8]=0x01;
			CRC_num = Modbus_CRC16(modbuf,9); //CRC校验
			modbuf[9]=CRC_num/256;
			modbuf[10]=CRC_num%256;
			modbuf[11]=0x0d;
			modbuf[12]=0x0a;
			modbuf_len=13;
		}break;
	//设置显示电压校正系数
		case CALIBRATION_VOLTAGE:
		{
			printf("\nuart send command : set CALIBRATION_VOLTAGE\n");
			for(i=0;i<5;i++) //从共享内存中读取参数 此指令需5个参数
			{
				scpi_parameter[i] = share_uart->ID_parameter_pack[uart_ID_parameter_pack_count + 1 + 1 + i];
				printf("uart parameter = %.10f\n",scpi_parameter[i]);
			}	
			modbuf[1]=0x10;	
			modbuf[2]=id/256;
			modbuf[3]=id%256;
			modbuf[4]=0x00;
			modbuf[5]=0x08;
			modbuf[6]=0x10;
			for(i=0;i<5;i++) //一共5个参数
			{
				if(scpi_parameter[i] >= 0) //判断符号位 为正数
				{
					modbuf[7+i] = 0x00;    //0x00 表示系数为正数
					if(i == 0 || i == 1)
					{
						senddata = (long int)(scpi_parameter[i] * 10000000000); //前两个参数 做乘以10000000000处理
					}
					else
					{
						senddata = (long int)(scpi_parameter[i] * 10000); //做乘以10000处理
					}
				}
				if(scpi_parameter[i] < 0)  //判断符号位 为负数
				{
					modbuf[7+i] = 0x01;    //0x01 表示系数为负数
					scpi_parameter[i] = scpi_parameter[i]* (-1);   //负数转为正数，方便计数
					if(i == 0 || i == 1)
					{
						senddata = (long int)(scpi_parameter[i] * 10000000000); //前两个参数 做乘以10000000000处理
					}
					else
					{
						senddata = (long int)(scpi_parameter[i] * 10000); //做乘以10000处理
					}					
				}			
				modbuf[13+i*2] = senddata/256; //系数高字节
				modbuf[14+i*2] = senddata%256; //系数低字节
			}
			CRC_num = Modbus_CRC16(modbuf,23); //CRC校验
			modbuf[23]=CRC_num/256;
			modbuf[24]=CRC_num%256;
			modbuf[25]=0x0d;
			modbuf[26]=0x0a;
			modbuf_len=27;
		}break;
	//设置显示电流分段校正系数
		case CALIBRATION_CURRENT:
		{
			printf("\nuart send command : set CALIBRATION_CURRENT\n");
			for(i=0;i<8;i++) //从共享内存中读取参数 此指令需8个参数
			{
				scpi_parameter[i] = share_uart->ID_parameter_pack[uart_ID_parameter_pack_count + 1 + 1 + i];
				printf("uart parameter = %f\n",scpi_parameter[i]);
			}	
			modbuf[1]=0x10;	
			modbuf[2]=id/256;
			modbuf[3]=id%256;
			modbuf[4]=0x00;
			modbuf[5]=0x0c;
			modbuf[6]=0x18;
			for(i=0;i<8;i++) //一共8个参数
			{
				if(scpi_parameter[i] >= 0) //判断符号位 为正数
				{
					modbuf[7+i] = 0x00;    //0x00 表示系数为正数
					senddata = (long int)(scpi_parameter[i] * 10000); //做乘以10000处理
				}
				if(scpi_parameter[i] < 0)  //判断符号位 为负数
				{
					modbuf[7+i] = 0x01;    //0x01 表示系数为负数
					scpi_parameter[i] = scpi_parameter[i]* (-1);   //负数转为正数，方便计数
					senddata = (long int)(scpi_parameter[i] * 10000); //做乘以10000处理
				}
				if(i%2 == 0) //为系数K 上位机系数K和系数B交替发送，存储时得判断
				{
					modbuf[15+i] = senddata/256; //系数K高字节
					modbuf[16+i] = senddata%256; //系数K低字节
				}
				else         //为系数B
				{
					modbuf[22+i] = senddata/256; //系数B高字节
					modbuf[23+i] = senddata%256; //系数B低字节						
				}
			}
			CRC_num = Modbus_CRC16(modbuf,31); //CRC校验
			modbuf[31]=CRC_num/256;
			modbuf[32]=CRC_num%256;
			modbuf[33]=0x0d;
			modbuf[34]=0x0a;
			modbuf_len=35;
		}break;
	//设置曲线ISC分段校正系数
		case CALIBRATION_ISC:
		{
			printf("\nuart send command : set CALIBRATION_ISC\n");
			for(i=0;i<8;i++) 	//从共享内存中读取参数 此指令需8个参数
			{
				scpi_parameter[i] = share_uart->ID_parameter_pack[uart_ID_parameter_pack_count + 1 + 1 + i];
				printf("uart parameter = %f\n",scpi_parameter[i]);
			}
			sprintf(ISC_str,"echo -e '%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f' > /mnt/ISC.txt",scpi_parameter[0],scpi_parameter[1],scpi_parameter[2],scpi_parameter[3],scpi_parameter[4],scpi_parameter[5],scpi_parameter[6],scpi_parameter[7]); //存储ISC到txt文件中 用于任意曲线的电流计算
			system(ISC_str);     //system函数 执行命令行语句
			system("sync");      //用来强制将内存缓冲区中的数据立即写入磁盘中，即保存修改后的数据到原文件中
			
			modbuf[1]=0x10;
			modbuf[2]=id/256;
			modbuf[3]=id%256;
			modbuf[4]=0x00;
			modbuf[5]=0x08;
			modbuf[6]=0x10;
			for(i=0;i<8;i++) //一共8个参数
			{
				if(scpi_parameter[i] >= 0) //判断符号位 为正数
				{
				  //ISC_high = 0x00 + ((int)(scpi_parameter[i]/1) << 5) + (int)((((int)(scpi_parameter[i]*8192) >> 8))&0x1F);
					ISC_high = (int)(((int)(scpi_parameter[i]*8192) >> 8) & 0x7F);
					ISC_low = (scpi_parameter[i]*8192);
				}
				if(scpi_parameter[i] < 0) //判断符号位 为负数
				{
					scpi_parameter[i] = scpi_parameter[i]* (-1); //负数转为正数，方便计数
					ISC_high = scpi_parameter[i]*8192;           //小数转为整数
					ISC_high = (~ISC_high)+1;                    //取反加一
					ISC_low = ISC_high & 0xFF;                   //低八位
					ISC_high = ((ISC_high>>8) & 0x7F) + 0x80;    //高八位
				}
				if(i%2 == 0) //为系数K 上位机系数K和系数B交替发送，存储时得判断
				{
					modbuf[7+i] = ISC_high; //系数K高字节
					modbuf[8+i] = ISC_low;  //系数K低字节
				}
				else         //为系数B
				{
					modbuf[14+i] = ISC_high; //系数B高字节
					modbuf[15+i] = ISC_low;  //系数B低字节						
				}
			}
			CRC_num = Modbus_CRC16(modbuf,23); //CRC校验
			modbuf[23]=CRC_num/256;
			modbuf[24]=CRC_num%256;
			modbuf[25]=0x0d;
			modbuf[26]=0x0a;
			modbuf_len=27;
		}break;
	//设置模拟器编码
		case SERIAL_NUMBER:
		{
			printf("\nuart send command : set SERIAL_NUMBER\n");
			modbuf[1]=0x10;	
			modbuf[2]=0x00;
			modbuf[3]=0x9a;
			modbuf[4]=0x00;
			modbuf[5]=0x05;
			modbuf[6]=0x0a;
			for(i=0;i<10;i++)
			{
				modbuf[7+i] = share_uart->save_serial_number[i];   //存储模拟器编码
			}
			CRC_num = Modbus_CRC16(modbuf,17);     //CRC校验
			modbuf[17]=CRC_num/256;
			modbuf[18]=CRC_num%256;
			modbuf[19]=0x0d;
			modbuf[20]=0x0a;
			modbuf_len=21;
		}break;
		default:break;
	}
}

/******************************************/
//根据收到SCPI指令后，数据库的解析ID，发送相应的命令给前面板
/******************************************/
void uart_send_set(void)
{
	int i;                //循环计数
	int len = 0;          //参数个数

	//printf("uart_send __ uart_ID_parameter_pack_count = %d\n",uart_ID_parameter_pack_count);  //打印队列号
	//printf("uart_send __ ID_parameter_pack = %f\n",share_uart->ID_parameter_pack[uart_ID_parameter_pack_count]); //打印ID号
	Modbus_rtu(share_uart->ID_parameter_pack[uart_ID_parameter_pack_count]);  //根据查询ID结果，进行modbuf协议组帧
	tcflush(fd_uart, TCIOFLUSH);      //向串口发送数据之前先清空输入输出队列
	write(fd_uart,modbuf,modbuf_len); //串口发送数据
	memset(modbuf,0,modbuf_len); //清空要发送给串口的数据
	modbuf_len = 0;              //清零要发送给串口的数据长度
}

/******************************************/
//串口发送
/******************************************/
void uart_send(void)
{
	gettimeofday(&time_count,NULL); //获取当前时间
	msec_now = (long long)(time_count.tv_sec * 1000 + time_count.tv_usec / 1000); //当前毫秒
	if(msec_now - msec_last >= 10) //与上一次的时间比较 超过10ms 简单延时，使串口数据能完全发送出去
	{
		msec_last = msec_now; //更新时间
		time_one ++;          //设置指令计时
		time_two ++;          //查询指令计时
		time_there ++;        //查询指令计时
		time_sendanydata++;   //发送任意曲线数据包计时
	}
	
	//从上位机下发曲线开机
	if(TableMode_Flag == Enable && share_uart->anydata_fromPC_flag == Enable && chose_num_donwload == Disable)  //发送时曲线前面板必须处于Table模式 从上位机下发曲线标志位 上位机选择曲线号开机志位
	{
		chose_line_donwload = Enable;  //上位机下发曲线开机志位
		if(send_count == 0) //发送第一包
		{
			if(time_sendanydata >= 8)   //第0包延时80ms
			{
				time_sendanydata = 0;
				anydata_send();  //发送上位机下发的曲线给前面板
			}
		}
		else //发送剩下的包
		{
			if(time_sendanydata >= 3)   //第1包后全部延时30ms
			{
				time_sendanydata = 0;
				anydata_send();   //发送上位机下发的曲线给前面板
			}
		}
	}
	
	//上位机选择曲线号开机 发送曲线号给前面板并收到回复后 开始发送曲线给前面板
	if(TableMode_Flag == Enable && PC_UseCurveOnOff_Flag == Enable)  //发送时曲线前面板必须处于Table模式 上位机选择曲线号开机标志位
	{
		chose_num_donwload = Enable;  //上位机选择曲线号开机志位 
		share_uart->anydata_fromPC_flag = Disable; //从上位机下发曲线标志位 清零
		if(send_TXB_count == 0) //发送第一包
		{
			if(time_sendanydata >= 8)  //第0包延时80ms
			{
				time_sendanydata = 0;
				AnyData_FromTXB_Dispose(); //从通讯板下发曲线给前面板 (有上位机选择曲线号开机、前面板选择曲线号开机两种情况)
			}
		}
		else //发送剩下的包
		{
			if(time_sendanydata>=3)   //第1包后全部延时30ms
			{
				time_sendanydata = 0;
				AnyData_FromTXB_Dispose(); //从通讯板下发曲线给前面板 (有上位机选择曲线号开机、前面板选择曲线号开机两种情况)
			}
		}
	}
	
	//前面板选择曲线号开机 从通讯板下发曲线
	if(anydata_sendfromtxb_ok == Enable)  //前面板选择曲线号，从通讯板下发曲线标志位
	{
		if(send_TXB_count == 0) //发送第一包
		{
			if(time_sendanydata >= 8)   //第0包延时80ms
			{
				time_sendanydata = 0;
				AnyData_FromTXB_Dispose(); //从通讯板下发曲线给前面板 (有上位机选择曲线号开机、前面板选择曲线号开机两种情况)
			}
		}
		else //发送剩下的包
		{
			if(time_sendanydata>=3)   //第1包后全部延时30ms
			{
				time_sendanydata = 0;
				AnyData_FromTXB_Dispose(); //从通讯板下发曲线给前面板 (有上位机选择曲线号开机、前面板选择曲线号开机两种情况)
			}
		}
	}
	
	//从上位机下载任意曲线到通讯板保存，必须在TABLE模式下保存
	if(TableMode_Flag == Enable && share_uart->anydata_savetxb_ok == Enable)	//为Table模式 存储上位机发送的任意曲线数据到通讯板标志位
	{
		Anydata_SavetoTXB_Dispose();  //从上位机下载任意曲线到通讯板保存
		share_uart->anydata_savetxb_ok = Disable;
	}
	
	//有设置指令 而且无需发送数据包 需发送设置指令
	if((share_uart->ID_parameter_pack[uart_ID_parameter_pack_count] != 0) && (chose_line_donwload == Disable) && (chose_num_donwload == Disable) && (anydata_sendfromtxb_ok == Disable))
	{
		send_set_first = 0;	//设置指令发送后，若无新的设置指令，间隔时间再发送查询指令
		uart_send_flag = 1; //串口发送标志位 排除web发送串口数据时返回的信息
		if(send_check_first == 0) //查询指令发送后，若有新的设置指令，间隔时间再发送设置指令
		{
			send_check_first = 1;
			time_one = 0;         //重新计数
		} 
		if(send_check_first == 1)
		{
			if(time_one >= 20) //20*10ms
			{
				time_one = 0;
				uart_send_set(); //根据收到SCPI指令后，数据库的解析ID，发送相应的命令给前面板	
			}
		}
	}
	
	//当前无设置指令需要发送 而且无需发送数据包 循环发送查询指令
	if((share_uart->ID_parameter_pack[uart_ID_parameter_pack_count] == 0) && (chose_line_donwload == Disable) && (chose_num_donwload == Disable) && (anydata_sendfromtxb_ok == Disable))
	{ 
		send_check_first = 0;
		if(send_set_first == 0) //设置指令发送后，若无新的设置指令，间隔时间再发送查询指令
		{
			send_set_first = 1;
			time_two = 0;       //重新计数
			time_there = 0;
			time_second_task = 0;  //重新从第一个查询任务开始
		}
		if(send_set_first == 1)
		{
			if(time_two >= 10) //10*10ms
			{
				time_two = 0;
				switch(time_second_task)
				{
					case 0:
					{
						if(first_IP_check == 0) //上电后先发送一次IP查询命令
						{
							first_IP_check = 1; //清标志位
							IP_check();         //发送IP查询命令
						}
						else
						{
							GPIB_Flag = Disable; //GPIB地址查询标志位
							read_onoff_mode();   //返回模拟器开关机状态/模式
							time_second_task = 1;//下一个定时任务
						}
					}break;
					case 1:
					{
						read_status_check(); //读取模拟器的状态参数
						time_second_task = 2;//下一个定时任务
					}break;
					case 2:
					{
						measure_voltage_check(); //查询CC(FIXed)模式下电压
						time_second_task = 3;    //下一个定时任务
					}break;
					case 3:
					{
						GPIB_check();            //返回GPIB地址信息
						GPIB_Flag = Enable;      //GPIB地址查询标志位
						time_second_task = 4;    //下一个定时任务
					}break;
					case 4:
					{
						SAS_check();          //返回SAS各个值
						time_second_task = 5; //下一个定时任务
					}break;
					case 5:
					{
						measure_current_check(); //查询CC(FIXed)模式下电流
						if(TableMode_Curve_SendFlag == Enable) //发送Table模式下曲线总条数标志位 收到前面板回复则停止发送
						{
							time_second_task = 6; 	  //下一个定时任务
						}
						else
						{
							time_second_task = 0; 	  //下一个定时任务
						}
					}break;
					case 6:
					{
						TableMode_Curve_CountSet();//发送Table模式下曲线总条数
						time_second_task = 0; 	  //下一个定时任务
					}
					default:break;
				}
			}
			if(time_there >= 300) //300*10ms
			{
				usleep(10000); //延时，保证串口能发送数据，不会被上面的发送任务干扰到 10ms
				time_there = 0;
				switch(time_third_task)
				{
					case 0:
					{
						IP_check();          //发送IP查询命令
						time_third_task = 1; //下一个定时任务
					}break;
					case 1:
					{
						LAN_check();         //网络连接状态检查							
						time_third_task = 0; //下一个定时任务
					}break;
					default:break;
				}
			}
		}
	}
}

/******************************************/
//串口接收
/******************************************/
void uart_receive(void)
{
	unsigned char uart_buff[200]={0}; //串口接收到的值
	int uart_len = 0;  //串口接收到的数据的长度
	int CRC_check = 0; //CRC校验结果
	int i;             //for循环计数
	memset(uart_buff,0,200);
	uart_len = read(fd_uart,uart_buff,200); //读取串口接收到的数据 没读取到数据则返回-1
	if(uart_len != -1 && uart_len>2)        //如果串口接收到数据，则解析数据 至少3个数据（CRC占2个）
	{
		/*printf("uart receive len  = %d\n", uart_len);//打印串口接收到的数据的长度
		printf("uart receive buff = ");
		for(i =0;i<uart_len;i++)           //打印串口接收到的数据
		{
			printf("%X ",uart_buff[i]);
		}
		printf("\n");*/
		
		CRC_check = Modbus_CRC16(uart_buff,uart_len-2); //对串口接收到的数据进行CRC校验
		//printf("CRC:%X\n",CRC_check);
		if(uart_buff[uart_len-2] == CRC_check/256 && uart_buff[uart_len-1] == CRC_check%256) //CRC校验通过，再对数据进行解析
		{		
			Rx_uart(uart_buff,uart_len);  //对收到的串口数据进行解析
			tcflush(fd_uart, TCIOFLUSH);  //清空输入输出队列
		}
		else //CRC校验出错
		{
			printf("CRC check error \n");  //显示CRC校验出错
			printf("uart receive len  = %d\n", uart_len);//打印串口接收到的数据的长度
			printf("uart receive buff = ");
			for(i =0;i<uart_len;i++)       //打印串口接收到的数据
			{
				printf("%X ",uart_buff[i]);
			}
			printf("\n");
		}	
	}
	else if(uart_len != -1 && uart_len<=2 && uart_len>0)   //如果串口接收到数据 少于3个数据
	{
		printf("uart get data too short error\n");
		printf("uart receive len  = %d\n", uart_len);//打印串口接收到的数据的长度
		printf("uart receive buff = ");
		for(i =0;i<uart_len;i++)           //打印串口接收到的数据
		{
			printf("%X ",uart_buff[i]);
		}
		printf("\n");
	}	
}

/******************************************/
//子进程 串口发送与接收
/******************************************/
void child_process(void)
{
	share_uart->ID_parameter_pack[0] = 0;  //存储数组开头赋值
	gettimeofday(&time_count,NULL);        //获取当前时间
	msec_last = (long long)(time_count.tv_sec * 1000 + time_count.tv_usec / 1000); //保存上一次的毫秒
	while(1)
	{
		uart_send();   //串口发送
		usleep(10000); //延时，保证串口能读取到完整的串口数据 20ms
		uart_receive();//串口接收
	}
}

/******************************************/
//主函数
/******************************************/
int main(int argc, char **argv)
{
	share_id = shmget(KEY, SIZE, 0666 | IPC_CREAT ); //创建共享内存，用于进程之间的通讯 用于共享串口读取到的值及其长度
	vxi11_init(); //vxi11初始化
	uart_init();  //串口初始化
	IP_init();    //检测本机IP
	pid_t pid;    //创建进程ID
	pid = fork(); //创建两个进程

	if(pid > 0)  //父进程 pid为子进程ID
	{
		SCPI_Init(&scpi_context,scpi_commands,&scpi_interface,scpi_units_def,0, 0, 0, 0,
			scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE); //初始化SCPI指令解析器
		share_vxi11 = (share_mem *) shmat(share_id, 0, 0);   //映射共享内存
		svc_run ();                    //vxi11通讯 一旦进入函数就不会退出
		fprintf(stderr, "%s", "svc_run returned");
		exit (1);
	}
	else if(pid == 0) //子进程 pid为0
	{
		share_uart = (share_mem *) shmat(share_id, 0, 0); //映射共享内存
		child_process(); //子进程 串口发送与接收
	}
	else //退出程序
		exit (1);
}