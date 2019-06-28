#include "vxi11.h"
#include <stdio.h>
#include <stdlib.h>

/******************************************/
//发送上位机下发的曲线给前面板
/******************************************/
void anydata_send(void)
{
	//printf("%s\n",share_uart->volt_data);
	//printf("%s\n",share_uart->curr_data);
	int i = 0,j = 0,y = 0;  //循环计数
	int now_addr = 0;       //当前一个地址
	int before_addr = 0;    //前一个地址
	int date_len = 0;       //数据长度
	char save_temp[20] = {'0'};//临时存储
	int volt_len,curr_len;     //字符串长度
	double temp;               //临时数据
	double curr_getdata[10000]; //8185个电流值 浮点型
	double volt_getdata[10000]; //8185个电压值 浮点型
	char num_5k[10],num_5b[10],num_10k[10],num_10b[10],num_15k[10],num_15b[10],num_20k[10],num_20b[10]; //ISC分段校正系数 字符型
	float ISC_5k,ISC_5b,ISC_10k,ISC_10b,ISC_15k,ISC_15b,ISC_20k,ISC_20b; //ISC分段校正系数 浮点型
	float num_k,num_b;         //最终取到的 K B 值
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度
	int CRC_num = 0;           //CRC校验结果
	FILE *fp = NULL;           //校验系数的文件
	
	if(anydata_take == 0) //还未从字符串中提取数值 开始提取并进行计算
	{	
		anydata_take = 1;
		memset(curr_getdata,0,10000); //清空数据
		memset(volt_getdata,0,10000);
		memset(Iint,0,10000);
		send_count = 0; //第0帧数据包
		volt_len = strlen(share_uart->volt_data); //提取字符长度
		curr_len = strlen(share_uart->curr_data);
		//printf("volt_len = %d\n",volt_len);
		//printf("curr_len = %d\n",curr_len);
		//提取电流数据
		for(i=0;i<=curr_len;i++)
		{
			if(share_uart->curr_data[i] == ',' || i==curr_len)
			{
				now_addr = i+1;                    //当前字符的地址 当前地址表示从字符串第一个地址(从1数起)开始数到的地址
				date_len = now_addr-before_addr-1; //数据长度date_len = 当前地址now_addr - 前一个地址before_addr - 1
				for(j=0;j<date_len;j++)
				{
					save_temp[j] = share_uart->curr_data[j+before_addr]; //开始存储参数
				}
				curr_getdata[y] = atof(save_temp); //字符串转为浮点数
				//printf(" curr_getdata[%d] = %.9f \n",y,curr_getdata[y]);	
				y++;                     //指向下一个存储位置
				before_addr = now_addr;  //保存前一个地址
				for(j=0;j<20;j++)        //清除临时存储
				{
					save_temp[j] = '0';
				}
			}
		}
		now_addr = 0;
		before_addr = 0;
		y = 0;
		//提取电压数据
		for(i=0;i<=volt_len;i++)
		{
			if(share_uart->volt_data[i] == ',' || i==volt_len)
			{
				now_addr = i+1;                    //当前字符的地址 当前地址表示从字符串第一个地址(从1数起)开始数到的地址
				date_len = now_addr-before_addr-1; //数据长度date_len = 当前地址now_addr - 前一个地址before_addr - 1
				for(j=0;j<date_len;j++)
				{
					save_temp[j] = share_uart->volt_data[j+before_addr]; //开始存储参数
				}
				volt_getdata[y] = atof(save_temp); //字符串转为浮点数
				//printf(" volt_getdata[%d] = %.9f \n",y,volt_getdata[y]);	
				y++;                     //指向下一个存储位置
				before_addr = now_addr;  //保存前一个地址
				for(j=0;j<20;j++)        //清除临时存储
				{
					save_temp[j] = '0';
				}
			}
		}
		
		while(fp == NULL) //一直到文件打开成功
		{
			printf("anydata_send ISC fgets open___________________________________\n");
			usleep(100000);  //延时100ms
			fp = fopen("/mnt/ISC.txt","r");  //打开校验系数的文件
			if(fp == NULL)   //打开文件失败 则关闭文件
			{
				fclose(fp);  //关闭校验系数的文件
			}
		}
		//计算电流值 电流数据中的第一个电流数据为短路电流ISC curr_getdata[0] 电压数据中最后一个为开路电压VOC volt_getdata[8184]
		fgets(num_5k,10,fp);fgets(num_5b,10,fp); //读取ISC分段校正系数
		fgets(num_10k,10,fp);fgets(num_10b,10,fp);
		fgets(num_15k,10,fp);fgets(num_15b,10,fp);
		fgets(num_20k,10,fp);fgets(num_20b,10,fp);
		fclose(fp);
		ISC_5k = atof(num_5k); ISC_5b = atof(num_5b);//字符串转为浮点数
		ISC_10k = atof(num_10k); ISC_10b = atof(num_10b);
		ISC_15k = atof(num_15k); ISC_15b = atof(num_15b);
		ISC_20k = atof(num_20k); ISC_20b = atof(num_20b);
		//用短路电流ISC，来判断这次采用哪个控制电流校准系数
		if(curr_getdata[0] >= 0 && curr_getdata[0] <= 5)
		{
			num_k = ISC_5k;
			num_b = ISC_5b;
			Curr_Path = 1;
		}
		else if(curr_getdata[0] > 5 && curr_getdata[0] <= 10)
		{
			num_k = ISC_10k;
			num_b = ISC_10b;
			Curr_Path = 3;
		}
		else if(curr_getdata[0] > 10 && curr_getdata[0] <= 15)
		{
			num_k = ISC_15k;
			num_b = ISC_15b;
			Curr_Path = 7;
		}
		else if(curr_getdata[0] > 15 && curr_getdata[0] <= 20)
		{
			num_k = ISC_20k;
			num_b = ISC_20b;
			Curr_Path = 15;
		}
		else if(curr_getdata[0] > 20)
		{
			Curr_Path = 15;
		}
		for(i=0;i<8185;i++) //将8185个电流数据都乘以对应的控制电流校准系数 并代入公式进行计算
		{
			Iint[i] = (curr_getdata[i]*num_k+num_b)*52428/( ((int)((curr_getdata[0]/5)+1)) * 5);
		}
		
		voc_sa = volt_getdata[8184]/140*1.9*65535/2; //开路电压

		temp = 8185/(volt_getdata[8184]/140*62259);  //数据点数与开路电压的比值
		for(i=16;i>0;i--)
		{
			temp = temp*2;
			if(temp > 1)
			{
				temp = temp - 1;
				nv_ratio = nv_ratio | 1<<(i-1); //移位 从最高位开始赋值1
			}
		}
		
		if(volt_getdata[8184] <= 18.375) //封锁信号
			Volt_inh = 3;
		else if(volt_getdata[8184] > 18.375 && volt_getdata[8184] <= 26.75)
			Volt_inh = 7;
		else if(volt_getdata[8184] > 26.75 && volt_getdata[8184] <= 40.125)
			Volt_inh = 15;
		else if(volt_getdata[8184] > 40.125 && volt_getdata[8184] <= 53.5)
			Volt_inh = 31;
		else if(volt_getdata[8184] > 53.5 && volt_getdata[8184] <= 66.875)
			Volt_inh = 66;
		else if(volt_getdata[8184] > 66.875 && volt_getdata[8184] <= 80.25)
			Volt_inh = 127;
		else if(volt_getdata[8184] > 80.25 && volt_getdata[8184] <= 93.625)
			Volt_inh = 511;
		else if(volt_getdata[8184] > 93.625 && volt_getdata[8184] <= 107)
			Volt_inh = 511;
		else if(volt_getdata[8184] > 107 && volt_getdata[8184] <= 120.375)
			Volt_inh = 511;
		else if(volt_getdata[8184] > 120.375)
			Volt_inh = 511;
		Volt_inh = ((Curr_Path<<12)&0xf000)+Volt_inh;
		
		Curr_sc = curr_getdata[0]*65535/20; //短路电流
	}
	//开始发送计算完后的值
	sendbuf[0]=0x00;
	sendbuf[1]=0x10;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x8b;
	sendbuf[4]=0x00;
	sendbuf[5]=0x41; //字数
	sendbuf[6]=0x82; //字节数
	sendbuf[7]=0x00;
	sendbuf[8]=send_count; //第几帧
	if(send_count < 127)   //前127帧数据
	{
		for(i=0;i<64;i++)  //有效数据
		{
			sendbuf[9+i*2] = Iint[send_count*64+i] >> 8; //高字节
			sendbuf[10+i*2] = Iint[send_count*64+i];  //低字节
		}
	}
	else if(send_count == 127)  //最后1帧
	{
		for(i=0;i<57;i++)  //有效数据
		{
			sendbuf[9+i*2] = Iint[send_count*64+i] >> 8;
			sendbuf[10+i*2] = Iint[send_count*64+i];
		}
		sendbuf[123] = (voc_sa >> 8)&0xff; 	 //开路电压
		sendbuf[124] = voc_sa&0x00ff;
		sendbuf[125] = (nv_ratio >> 8)&0xff; //数据点数与开路电压的比值
		sendbuf[126] = nv_ratio&0x00ff;
		sendbuf[127] = (Volt_inh >> 8)&0xff; //封锁信号
		sendbuf[128] = (Volt_inh)&0x00ff;
		sendbuf[129] = (Curr_sc >> 8)&0xff;  //短路电流
		sendbuf[130] = Curr_sc&0x00ff;
		sendbuf[131] = 0;sendbuf[132] = 0;
		sendbuf[133] = 0;sendbuf[134] = 0;
		sendbuf[135] = 0;sendbuf[136] = 0;
	}
	CRC_num = Modbus_CRC16(sendbuf,137); //CRC校验
	sendbuf[137]=CRC_num/256;
	sendbuf[138]=CRC_num%256; 
	sendbuf[139]=0x0d;
	sendbuf[140]=0x0a;
	sendbuf_len=141;

	/*
	for(i=0;i<141;i++)
		printf("sendbuf[%d] = %X\n",i,sendbuf[i]);
	*/
	printf("anydata_send_count = %d\n",send_count); //打印发送了第几帧数据
	if(send_count == 127)
	{
		printf("\nanydata_send send success\n"); //数据发送完成
	}
	
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
	if(send_count == 0)    //第一帧发慢点
		sleep(1);
}

/******************************************/
//从通讯板下发曲线给前面板 (有上位机选择曲线号开机、前面板选择曲线号开机两种情况)
/******************************************/
void AnyData_FromTXB_Dispose(void)
{	
	FILE *fp = NULL,*fp1 = NULL,*fp2 = NULL;  //电压数据文件 电流数据文件 校验系数的文件
	int i = 0;        //循环计数
	int vol_count = 0;//电压数据个数统计
	int cur_count = 0;//电流数据个数统计
	double temp;      //临时数据
	char buff[100];   //临时存储文件读取数据
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度
	int CRC_num = 0;           //CRC校验结果
	double current[10000];  //8185个电流值 浮点型
	double voltage[10000];  //8185个电压值 浮点型
	char num_5k[10],num_5b[10],num_10k[10],num_10b[10],num_15k[10],num_15b[10],num_20k[10],num_20b[10]; //ISC分段校正系数 字符型
	float ISC_5k,ISC_5b,ISC_10k,ISC_10b,ISC_15k,ISC_15b,ISC_20k,ISC_20b; //ISC分段校正系数 浮点型
	float num_k,num_b;   //最终取到的 K B 值
	
	if(anydata_file_take == 0) //还未从文件中提取数值 开始提取并进行计算
	{
		anydata_file_take = 1;
		memset(current,0,10000); //清空数据
		memset(voltage,0,10000);
		memset(Iint,0,10000);
		memset(buff,0,100);
		send_TXB_count = 0; //第0帧数据包
		
		while((fp == NULL) && (fp1 == NULL)) //一直到文件打开成功
		{
			printf("AnyData_FromTXB_Dispose file fgets open___________________________________\n");
			usleep(100000);  //延时100ms
			switch(share_uart->curve_Num)  //根据曲线号加载相应的曲线
			{
				case 1:
				{
					fp = fopen("/mnt/Cur_line/Line1/voltage.txt","r");   //电压数据文件
					fp1 = fopen("/mnt/Cur_line/Line1/current.txt","r");  //电流数据文件
				}break;
				case 2:
				{
					fp = fopen("/mnt/Cur_line/Line2/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line2/current.txt","r");
				}break;
				case 3:
				{
					fp = fopen("/mnt/Cur_line/Line3/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line3/current.txt","r");
				}break;
				case 4:
				{
					fp = fopen("/mnt/Cur_line/Line4/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line4/current.txt","r");
				}break;
				case 5:
				{
					fp = fopen("/mnt/Cur_line/Line5/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line5/current.txt","r");
				}break;
				case 6:
				{
					fp = fopen("/mnt/Cur_line/Line6/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line6/current.txt","r");
				}break;
				case 7:
				{
					fp = fopen("/mnt/Cur_line/Line7/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line7/current.txt","r");
				}break;
				case 8:
				{
					fp = fopen("/mnt/Cur_line/Line8/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line8/current.txt","r");
				}break;
				case 9:
				{
					fp = fopen("/mnt/Cur_line/Line9/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line9/current.txt","r");
				}break;
				case 10:
				{
					fp = fopen("/mnt/Cur_line/Line10/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line10/current.txt","r");
				}break;
				case 11:
				{
					fp = fopen("/mnt/Cur_line/Line11/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line11/current.txt","r");
				}break;
				case 12:
				{
					fp = fopen("/mnt/Cur_line/Line12/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line12/current.txt","r");
				}break;
				case 13:
				{
					fp = fopen("/mnt/Cur_line/Line13/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line13/current.txt","r");
				}break;
				case 14:
				{
					fp = fopen("/mnt/Cur_line/Line14/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line14/current.txt","r");
				}break;
				case 15:
				{
					fp = fopen("/mnt/Cur_line/Line15/voltage.txt","r");
					fp1 = fopen("/mnt/Cur_line/Line15/current.txt","r");
				}break;
				default:break;
			}
			if((fp == NULL) && (fp1 == NULL)) //打开文件失败 则关闭文件
			{
				fclose(fp);  //关闭电压数据文件
				fclose(fp1); //关闭电流数据文件
			}
		}
		while(fgets(buff,100,fp))  //读取电压数据
		{	
			voltage[vol_count] = atof(buff);  //字符串转为浮点数
			memset(buff,0,100);  //清空数组
			vol_count++;  //电压数据个数统计
		}
		while(fgets(buff,100,fp1)) //读取电流数据
		{	
			current[cur_count] = atof(buff);  //字符串转为浮点数
			memset(buff,0,100);  //字符串转为浮点数
			cur_count++;  //电流数据个数统计
		}
		fclose(fp);  //关闭电压数据文件
		fclose(fp1); //关闭电流数据文件
		
		while(fp2 == NULL) //一直到文件打开成功
		{
			printf("AnyData_FromTXB_Dispose ISC fgets open___________________________________\n");
			usleep(100000);  //延时100ms
			fp2 = fopen("/mnt/ISC.txt","r");  //打开校验系数的文件
			if(fp2 == NULL)   //打开文件失败 则关闭文件
			{
				fclose(fp2);  //关闭校验系数的文件
			}
		}
		//计算电流值 电流数据中的第一个电流数据为短路电流ISC current[0] 电压数据中最后一个为开路电压VOC voltage[8184]
		fgets(num_5k,10,fp2);fgets(num_5b,10,fp2); //读取ISC分段校正系数
		fgets(num_10k,10,fp2);fgets(num_10b,10,fp2);
		fgets(num_15k,10,fp2);fgets(num_15b,10,fp2);
		fgets(num_20k,10,fp2);fgets(num_20b,10,fp2);
		fclose(fp2);
		ISC_5k = atof(num_5k); ISC_5b = atof(num_5b);//字符串转为浮点数
		ISC_10k = atof(num_10k); ISC_10b = atof(num_10b);
		ISC_15k = atof(num_15k); ISC_15b = atof(num_15b);
		ISC_20k = atof(num_20k); ISC_20b = atof(num_20b);
		//用短路电流ISC，来判断这次采用哪个控制电流校准系数
		if(current[0] >= 0 && current[0] <= 5)
		{
			num_k = ISC_5k;
			num_b = ISC_5b;
			Curr_Path = 1;
		}
		else if(current[0] > 5 && current[0] <= 10)
		{
			num_k = ISC_10k;
			num_b = ISC_10b;
			Curr_Path = 3;
		}
		else if(current[0] > 10 && current[0] <= 15)
		{
			num_k = ISC_15k;
			num_b = ISC_15b;
			Curr_Path = 7;
		}
		else if(current[0] > 15 && current[0] <= 20)
		{
			num_k = ISC_20k;
			num_b = ISC_20b;
			Curr_Path = 15;
		}
		else if(current[0] > 20)
		{	
			Curr_Path = 15;
		}
		for(i=0;i<cur_count;i++) //将8185个电流数据都乘以对应的控制电流校准系数 并代入公式进行计算
		{
			Iint[i] = (current[i]*num_k+num_b)*52428/( ((int)((current[0]/5)+1)) * 5); 	
		}
		
		voc_sa = voltage[8184]/140*1.9*65535/2; //开路电压
		
		temp = 8185/(voltage[8184]/140*62259);  //数据点数与开路电压的比值
		for(i=16;i>0;i--)
		{
			temp = temp*2;
			if(temp > 1)
			{
				temp = temp - 1;
				nv_ratio = nv_ratio | 1<<(i-1); //移位 从最高位开始赋值1
			}
		}
		
		if(voltage[8184] <= 18.375) //封锁信号
			Volt_inh = 3;
		else if((voltage[8184] > 18.375) && (voltage[8184] <= 26.75))
			Volt_inh = 7;
		else if((voltage[8184] > 26.75) && (voltage[8184] <= 40.125))
			Volt_inh = 15;
		else if((voltage[8184] > 40.125) && (voltage[8184] <= 53.5))
			Volt_inh = 31;
		else if((voltage[8184] > 53.5) && (voltage[8184] <= 66.875))
			Volt_inh = 66;
		else if((voltage[8184] > 66.875) && (voltage[8184] <= 80.25))
			Volt_inh = 127;
		else if((voltage[8184] > 80.25) && (voltage[8184] <= 93.625))
			Volt_inh = 511;
		else if((voltage[8184] > 93.625) && (voltage[8184] <= 107))
			Volt_inh = 511;
		else if((voltage[8184] > 107) && (voltage[8184] <= 120.375))
			Volt_inh = 511;
		else if(voltage[8184] > 120.375)
			Volt_inh = 511;
		Volt_inh = ((Curr_Path<<12)&0xf000)+Volt_inh;
		
		Curr_sc = current[0]*65535/20; //短路电流
	}
	//开始发送计算完后的值
	sendbuf[0]=0x00;
	sendbuf[1]=0x10;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x8b;
	sendbuf[4]=0x00;
	sendbuf[5]=0x41; //字数
	sendbuf[6]=0x82; //字节数
	sendbuf[7]=0x00;
	sendbuf[8]=send_TXB_count; //第几帧
	if(send_TXB_count < 127)   //前127帧数据
	{
		for(i=0;i<64;i++) //有效数据
		{
			sendbuf[9+i*2] = Iint[send_TXB_count*64+i] >> 8; //高字节
			sendbuf[10+i*2] = Iint[send_TXB_count*64+i];  //低字节
		}
	}
	else if(send_TXB_count == 127)  //最后1帧
	{
		for(i=0;i<57;i++)  //有效数据
		{
			sendbuf[9+i*2] = Iint[send_TXB_count*64+i] >> 8;
			sendbuf[10+i*2] = Iint[send_TXB_count*64+i];
		}
		sendbuf[123] = (voc_sa >> 8)&0xff; 	 //开路电压
		sendbuf[124] = voc_sa&0x00ff;
		sendbuf[125] = (nv_ratio >> 8)&0xff; //数据点数与开路电压的比值
		sendbuf[126] = nv_ratio&0x00ff;
		sendbuf[127] = (Volt_inh >> 8)&0xff; //封锁信号
		sendbuf[128] = (Volt_inh)&0x00ff;
		sendbuf[129] = (Curr_sc >> 8)&0xff;  //短路电流
		sendbuf[130] = Curr_sc&0x00ff;
		sendbuf[131] = 0;sendbuf[132] = 0;
		sendbuf[133] = 0;sendbuf[134] = 0;
		sendbuf[135] = 0;sendbuf[136] = 0;
	}
	CRC_num = Modbus_CRC16(sendbuf,137); //CRC校验
	sendbuf[137]=CRC_num/256;
	sendbuf[138]=CRC_num%256; 
	sendbuf[139]=0x0d;
	sendbuf[140]=0x0a;
	sendbuf_len=141;

	/*
	for(i=0;i<141;i++)
		printf("sendbuf[%d] = %X\n",i,sendbuf[i]);
	*/
	printf("anydata_send_TXB_count = %d\n",send_TXB_count); //打印发送了第几帧数据
	if(send_TXB_count == 127)
	{
		printf("\nAnyData_FromTXB_Dispose send success\n"); //数据发送完成
	}
	
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
	if(send_TXB_count == 0) //第一帧发慢点
		sleep(1);
}

/******************************************/
//从上位机下载任意曲线到通讯板保存
/******************************************/
void Anydata_SavetoTXB_Dispose(void)
{
	FILE *fp,*fp1;        //电压数据文件 电流数据文件
	int i=0,j=0,y=0;      //循环计数
	int now_addr = 0;     //当前一个地址
	int before_addr = 0;  //前一个地址
	int date_len = 0;            //数据长度
	char save_temp[20] = {'0'};//临时存储
	int volt_len,curr_len;     //字符串长度
	
	volt_len = strlen(share_uart->volt_data); //提取字符长度
	curr_len = strlen(share_uart->curr_data);
	//printf("volt_len = %d\n",volt_len);
	//printf("curr_len = %d\n",curr_len);
	
	//曲线名做匹配
	//printf("curr_name = %s\n",share_uart->Curve_Name);
	if(strncmp("CURve1",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line1/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line1/current.txt","w");
		printf("save1 success\n");
	}
	else if(strncmp("CURve2",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line2/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line2/current.txt","w");
		printf("save2 success\n");
	}
	else if(strncmp("CURve3",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line3/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line3/current.txt","w");
		printf("save3 success\n");
	}
	else if(strncmp("CURve4",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line4/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line4/current.txt","w");
		printf("save4 success\n");
	}
	else if(strncmp("CURve5",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line5/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line5/current.txt","w");
		printf("save5 success\n");
	}
	else if(strncmp("CURve6",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line6/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line6/current.txt","w");
		printf("save6 success\n");
	}
	else if(strncmp("CURve7",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line7/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line7/current.txt","w");
		printf("save7 success\n");
	}
	else if(strncmp("CURve8",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line8/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line8/current.txt","w");
		printf("save8 success\n");
	}else if(strncmp("CURve9",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line9/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line9/current.txt","w");
		printf("save9 success\n");
	}else if(strncmp("CURve10",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line10/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line10/current.txt","w");
		printf("save10 success\n");
	}else if(strncmp("CURve11",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line11/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line11/current.txt","w");
		printf("save11 success\n");
	}
	else if(strncmp("CURve12",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line12/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line12/current.txt","w");
		printf("save12 success\n");
	}
	else if(strncmp("CURve13",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line13/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line13/current.txt","w");
		printf("save13 success\n");
	}
	else if(strncmp("CURve14",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line14/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line14/current.txt","w");
		printf("save14 success\n");
	}
	else if(strncmp("CURve15",share_uart->Curve_Name,7)==0)
	{
		fp = fopen("/mnt/Cur_line/Line15/voltage.txt","w");
		fp1 = fopen("/mnt/Cur_line/Line15/current.txt","w");
		printf("save15 success\n");
	}
	
	//提取、存储电流数据
	for(i=0;i<=curr_len;i++)
	{
		if(share_uart->curr_data[i] == ',' || i==curr_len)
		{
			now_addr = i+1;                    //当前字符的地址 当前地址表示从字符串第一个地址(从1数起)开始数到的地址
			date_len = now_addr-before_addr-1; //数据长度date_len = 当前地址now_addr - 前一个地址before_addr - 1
			for(j=0;j<date_len;j++)
			{
				save_temp[j] = share_uart->curr_data[j+before_addr]; //开始存储参数
			}
			curr_savedata[y] = atof(save_temp);        //字符串转为浮点数
			fprintf(fp1, "%.9lf\n",curr_savedata[y]);  //将数据写入文件中
			y++;                     //指向下一个存储位置
			before_addr = now_addr;  //保存前一个地址
			for(j=0;j<20;j++)        //清除临时存储
			{
				save_temp[j] = '0';
			}
		}
	}
	now_addr = 0;
	before_addr = 0;
	y = 0;
	//提取、存储电压数据
	for(i=0;i<=volt_len;i++)
	{
		if(share_uart->volt_data[i] == ',' || i==volt_len)
		{
			now_addr = i+1;                    //当前字符的地址 当前地址表示从字符串第一个地址(从1数起)开始数到的地址
			date_len = now_addr-before_addr-1; //数据长度date_len = 当前地址now_addr - 前一个地址before_addr - 1
			for(j=0;j<date_len;j++)
			{
				save_temp[j] = share_uart->volt_data[j+before_addr]; //开始存储参数
			}
			volt_savedata[y] = atof(save_temp);       //字符串转为浮点数
			fprintf(fp, "%.9lf\n",volt_savedata[y]);  //将数据写入文件中
			y++;                     //指向下一个存储位置
			before_addr = now_addr;  //保存前一个地址
			for(j=0;j<20;j++)        //清除临时存储
			{
				save_temp[j] = '0';
			}
		}
	}
	fclose(fp); //关闭电压数据文件
	fclose(fp1);//关闭电流数据文件
}
