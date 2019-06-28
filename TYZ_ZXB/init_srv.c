#include "vxi11.h"
#include <stdio.h>
#include <stdlib.h>

/******************************************/
//vxi11初始化
/******************************************/
void vxi11_init(void)
{
	register SVCXPRT *transp;
	pmap_unset (DEVICE_ASYNC, DEVICE_ASYNC_VERSION);
	pmap_unset (DEVICE_CORE, DEVICE_CORE_VERSION);
	pmap_unset (DEVICE_INTR, DEVICE_INTR_VERSION);
	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL)
	{
		fprintf (stderr, "%s", "cannot create tcp service.");
		exit(1);
	}
	if (!svc_register(transp, DEVICE_ASYNC, DEVICE_ASYNC_VERSION, device_async_1, IPPROTO_TCP))
	{
		fprintf (stderr, "%s", "unable to register (DEVICE_ASYNC, DEVICE_ASYNC_VERSION, tcp).");
		exit(1);
	}
	if (!svc_register(transp, DEVICE_CORE, DEVICE_CORE_VERSION,device_core_1, IPPROTO_TCP))
	{
		fprintf (stderr, "%s", "unable to register (DEVICE_CORE, DEVICE_CORE_VERSION, tcp).");
		exit(1);
	}
	if (!svc_register(transp, DEVICE_INTR, DEVICE_INTR_VERSION, device_intr_1, IPPROTO_TCP))
	{
		fprintf (stderr, "%s", "unable to register (DEVICE_INTR, DEVICE_INTR_VERSION, tcp).");
		exit(1);
	}
	printf("svc_register finish\n");
}

/******************************************/
//串口初始化
/******************************************/
void uart_init(void)
{
	char *dev  = "/dev/ttyO1";
	fd_uart = OpenDev(dev);  //打开串口设备
	//printf("opendev id:%d ",fd_uart);                 
	set_speed(fd_uart,115200);  //设置波特率         
	if (set_Parity(fd_uart,8,1,'N') == -1)
	{
		printf("set serial failed\n");
		exit(1);
	}
}

/******************************************/
//检测本机IP 用于与前面板设置的IP比较
/******************************************/
void IP_init(void)
{
	char data[100],Address[100],Netmask[100],Gateway[100]; //存储读取到的字符数据
	int i = 0;           //循环计数
	int count = 0;       //记录位数
	int len = 0;         //记录字符串长度
	int addr = 0;        //记录当前位置
	int point_count = 0; //区分出第几个.
	FILE *fp = fopen("/etc/network/interfaces","r");
	while(fgets(data,200,fp)) //读入每行数据
	{
		i++;
		if(i == 32)
			stpcpy(Address,data); //IP地址
		if(i == 33)
			stpcpy(Gateway,data); //网关
		if(i == 34)
			stpcpy(Netmask,data); //子网掩码
	}
	fclose(fp);
	//address 192.168.137.182
	//address 1.1.1.1
	//存储本机IP
	len = strlen(Address); //记录字符串长度
	for(i=8;i<len;i++)
	{
		if(Address[i] == '.')
		{
			point_count++;
			switch(point_count) //区分出第几个.
			{
				case 1:
				{
					count = i - 8; //记录位数
					addr = i;      //记录当前位置
					switch(count)  //根据位数存储数据
					{
						case 1:
						{
							sprintf(data, "%c", Address[i-1]);
						}break;
						case 2:
						{
							sprintf(data, "%c%c", Address[i-2],Address[i-1]);
						}break;
						case 3:
						{
							sprintf(data, "%c%c%c", Address[i-3],Address[i-2],Address[i-1]);
						}break;
					}
					LAN.Address[0] = atoi(data); //字符串转换成整型数
				}break;
				case 2:
				{
					count = i - addr - 1; //记录位数
					addr = i;       //记录当前位置
					switch(count)   //根据位数存储数据
					{
						case 1:
						{
							sprintf(data, "%c", Address[i-1]);
						}break;
						case 2:
						{
							sprintf(data, "%c%c", Address[i-2],Address[i-1]);
						}break;
						case 3:
						{
							sprintf(data, "%c%c%c", Address[i-3],Address[i-2],Address[i-1]);
						}break;
					}
					LAN.Address[1] = atoi(data); //字符串转换成整型数
				}break;
				case 3:
				{
					count = i - addr - 1; //记录位数
					addr = i;      //记录当前位置
					switch(count)  //根据位数存储数据
					{
						case 1:
						{
							sprintf(data, "%c", Address[i-1]);
						}break;
						case 2:
						{
							sprintf(data, "%c%c", Address[i-2],Address[i-1]);
						}break;
						case 3:
						{
							sprintf(data, "%c%c%c", Address[i-3],Address[i-2],Address[i-1]);
						}break;
					}
					LAN.Address[2] = atoi(data); //字符串转换成整型数
				}break;
			}
		}
		if(i == (len-1)) //为最后一位数据
		{
			count = i - addr - 1; //记录位数
			switch(count)   //根据位数存储数据
			{
				case 1:
				{
					sprintf(data, "%c", Address[i-1]);
				}break;
				case 2:
				{
					sprintf(data, "%c%c", Address[i-2],Address[i-1]);
				}break;
				case 3:
				{
					sprintf(data, "%c%c%c", Address[i-3],Address[i-2],Address[i-1]);
				}break;
			}
			LAN.Address[3] = atoi(data); //字符串转换成整型数
		}
	}
//存储本机网关	
	point_count = 0;       //清零
	len = strlen(Gateway); //记录字符串长度
	for(i=8;i<len;i++)
	{
		if(Gateway[i] == '.')
		{
			point_count++;
			switch(point_count) //区分出第几个.
			{
				case 1:
				{
					count = i - 8; //记录位数
					addr = i;      //记录当前位置
					switch(count)  //根据位数存储数据
					{
						case 1:
						{
							sprintf(data, "%c", Gateway[i-1]);
						}break;
						case 2:
						{
							sprintf(data, "%c%c", Gateway[i-2],Gateway[i-1]);
						}break;
						case 3:
						{
							sprintf(data, "%c%c%c", Gateway[i-3],Gateway[i-2],Gateway[i-1]);
						}break;
					}
					LAN.Gateway[0] = atoi(data); //字符串转换成整型数
				}break;
				case 2:
				{
					count = i - addr - 1; //记录位数
					addr = i;       //记录当前位置
					switch(count)   //根据位数存储数据
					{
						case 1:
						{
							sprintf(data, "%c", Gateway[i-1]);
						}break;
						case 2:
						{
							sprintf(data, "%c%c", Gateway[i-2],Gateway[i-1]);
						}break;
						case 3:
						{
							sprintf(data, "%c%c%c", Gateway[i-3],Gateway[i-2],Gateway[i-1]);
						}break;
					}
					LAN.Gateway[1] = atoi(data); //字符串转换成整型数
				}break;
				case 3:
				{
					count = i - addr - 1; //记录位数
					addr = i;      //记录当前位置
					switch(count)  //根据位数存储数据
					{
						case 1:
						{
							sprintf(data, "%c", Gateway[i-1]);
						}break;
						case 2:
						{
							sprintf(data, "%c%c", Gateway[i-2],Gateway[i-1]);
						}break;
						case 3:
						{
							sprintf(data, "%c%c%c", Gateway[i-3],Gateway[i-2],Gateway[i-1]);
						}break;
					}
					LAN.Gateway[2] = atoi(data); //字符串转换成整型数
				}break;
			}
		}
		if(i == (len-1)) //为最后一位数据
		{
			count = i - addr - 1; //记录位数
			switch(count)   //根据位数存储数据
			{
				case 1:
				{
					sprintf(data, "%c", Gateway[i-1]);
				}break;
				case 2:
				{
					sprintf(data, "%c%c", Gateway[i-2],Gateway[i-1]);
				}break;
				case 3:
				{
					sprintf(data, "%c%c%c", Gateway[i-3],Gateway[i-2],Gateway[i-1]);
				}break;
			}
			LAN.Gateway[3] = atoi(data); //字符串转换成整型数
		}
	}
//存储本机子网掩码	
	point_count = 0;       //清零
	len = strlen(Netmask); //记录字符串长度
	for(i=8;i<len;i++)
	{
		if(Netmask[i] == '.')
		{
			point_count++;
			switch(point_count) //区分出第几个.
			{
				case 1:
				{
					count = i - 8; //记录位数
					addr = i;      //记录当前位置
					switch(count)  //根据位数存储数据
					{
						case 1:
						{
							sprintf(data, "%c", Netmask[i-1]);
						}break;
						case 2:
						{
							sprintf(data, "%c%c", Netmask[i-2],Netmask[i-1]);
						}break;
						case 3:
						{
							sprintf(data, "%c%c%c", Netmask[i-3],Netmask[i-2],Netmask[i-1]);
						}break;
					}
					LAN.Netmask[0] = atoi(data); //字符串转换成整型数
				}break;
				case 2:
				{
					count = i - addr - 1; //记录位数
					addr = i;       //记录当前位置
					switch(count)   //根据位数存储数据
					{
						case 1:
						{
							sprintf(data, "%c", Netmask[i-1]);
						}break;
						case 2:
						{
							sprintf(data, "%c%c", Netmask[i-2],Netmask[i-1]);
						}break;
						case 3:
						{
							sprintf(data, "%c%c%c", Netmask[i-3],Netmask[i-2],Netmask[i-1]);
						}break;
					}
					LAN.Netmask[1] = atoi(data); //字符串转换成整型数
				}break;
				case 3:
				{
					count = i - addr - 1; //记录位数
					addr = i;      //记录当前位置
					switch(count)  //根据位数存储数据
					{
						case 1:
						{
							sprintf(data, "%c", Netmask[i-1]);
						}break;
						case 2:
						{
							sprintf(data, "%c%c", Netmask[i-2],Netmask[i-1]);
						}break;
						case 3:
						{
							sprintf(data, "%c%c%c", Netmask[i-3],Netmask[i-2],Netmask[i-1]);
						}break;
					}
					LAN.Netmask[2] = atoi(data); //字符串转换成整型数
				}break;
			}
		}
		if(i == (len-1)) //为最后一位数据
		{
			count = i - addr - 1; //记录位数
			switch(count)   //根据位数存储数据
			{
				case 1:
				{
					sprintf(data, "%c", Netmask[i-1]);
				}break;
				case 2:
				{
					sprintf(data, "%c%c", Netmask[i-2],Netmask[i-1]);
				}break;
				case 3:
				{
					sprintf(data, "%c%c%c", Netmask[i-3],Netmask[i-2],Netmask[i-1]);
				}break;
			}
			LAN.Netmask[3] = atoi(data); //字符串转换成整型数
		}
	}	
}
