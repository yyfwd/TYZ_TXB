#include "vxi11.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>  //目录操作的头文件

char *DirPath = "/mnt/Cur_line"; //任意曲线通讯板文件夹路径 

//低字节CRC值表
const char auchCRCLo[256]= 
{
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
0x40
};  
//高字节CRC值表
const char auchCRCHi[256]= 
{
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40
};

/******************************************/
//CRC校验
/******************************************/
int Modbus_CRC16(char *Buff_addr,int len)
{           
	char uchCRCHi = 0xFF; //CRC高字节的初始化
	char uchCRCLo = 0xFF; //CRC低字节的初始化
	int uIndex;           //CRC查找表的指针
	while (len--)
	{                                            //异或校验
		uIndex = uchCRCLo ^ *Buff_addr++;        //计算CRC
		uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex];
		uchCRCHi = auchCRCLo[uIndex];
	}
	return(uchCRCHi << 8 | uchCRCLo);
}

/******************************************/
//发送IP查询命令
/******************************************/
void IP_check(void)
{
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度
	sendbuf[0]=0x00;
	sendbuf[1]=0x03;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x90;
	sendbuf[4]=0x00;
	sendbuf[5]=0x06;
	sendbuf[6]=0x34;
	sendbuf[7]=0xC4;
	sendbuf[8]=0x0d;
	sendbuf[9]=0x0a;	
	sendbuf_len=10;
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
}

/******************************************/
//返回模拟器开关机状态/模式
/******************************************/
void read_onoff_mode(void)
{
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度
	int CRC_num = 0;      	   //CRC校验结果		
	sendbuf[0]=0x00;
	sendbuf[1]=0x03;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x9b;
	sendbuf[4]=0x00;
	sendbuf[5]=0x01;
	CRC_num = Modbus_CRC16(sendbuf,6);  //CRC校验
	sendbuf[6]=CRC_num/256;
	sendbuf[7]=CRC_num%256;
	sendbuf[8]=0x0d;
	sendbuf[9]=0x0a;		
	sendbuf_len=10;
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
}

/******************************************/
//读取模拟器的状态参数
/******************************************/
void read_status_check(void)
{
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度	
	int CRC_num = 0;      	   //CRC校验结果		
	sendbuf[0]=0x00;
	sendbuf[1]=0x03;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x8c;
	sendbuf[4]=0x00;
	sendbuf[5]=0x09;               //修改后为9个字
	CRC_num = Modbus_CRC16(sendbuf,6); //CRC校验
	sendbuf[6]=CRC_num/256;
	sendbuf[7]=CRC_num%256;
	sendbuf[8]=0x0d;
	sendbuf[9]=0x0a;		
	sendbuf_len=10;
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
}

/******************************************/
//查询CC(FIXed)模式下电压
/******************************************/
void measure_voltage_check(void)
{
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度	
	int CRC_num = 0;      //CRC校验结果		
	sendbuf[0]=0x00;
	sendbuf[1]=0x03;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x06;
	sendbuf[4]=0x00;
	sendbuf[5]=0x02;
	CRC_num = Modbus_CRC16(sendbuf,6); //CRC校验
	sendbuf[6]=CRC_num/256;
	sendbuf[7]=CRC_num%256;
	sendbuf[8]=0x0d;
	sendbuf[9]=0x0a;		
	sendbuf_len=10;
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
}

/******************************************/
//查询模拟器GPIB地址
/******************************************/
void GPIB_check(void)
{
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度
	int CRC_num = 0;      	   //CRC校验结果		
	sendbuf[0]=0x00;
	sendbuf[1]=0x03;
	sendbuf[2]=0x00;
	sendbuf[3]=0x91;
	sendbuf[4]=0x00;
	sendbuf[5]=0x01;
	sendbuf[6]=0x36;
	sendbuf[7]=0xd4;
	sendbuf[8]=0x0d;
	sendbuf[9]=0x0a;
	sendbuf_len=10;
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
}

/******************************************/
//返回SAS各个值
/******************************************/
void SAS_check(void)
{
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度
	int CRC_num = 0;           //CRC校验结果		
	sendbuf[0]=0x00;
	sendbuf[1]=0x03;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x94;
	sendbuf[4]=0x00;
	sendbuf[5]=0x08;
	CRC_num = Modbus_CRC16(sendbuf,6); //CRC校验
	sendbuf[6]=CRC_num/256;
	sendbuf[7]=CRC_num%256;
	sendbuf[8]=0x0d;
	sendbuf[9]=0x0a;		
	sendbuf_len=10;
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
}

/******************************************/
//查询CC(FIXed)模式下电流
/******************************************/
void measure_current_check(void)
{
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度
	int CRC_num = 0;      //CRC校验结果		
	sendbuf[0]=0x00;
	sendbuf[1]=0x03;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x05;
	sendbuf[4]=0x00;
	sendbuf[5]=0x03;
	CRC_num = Modbus_CRC16(sendbuf,6); //CRC校验
	sendbuf[6]=CRC_num/256;
	sendbuf[7]=CRC_num%256;
	sendbuf[8]=0x0d;
	sendbuf[9]=0x0a;		
	sendbuf_len=10;
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
}

/******************************************/
//网络连接状态检查
/******************************************/
void LAN_check(void)
{
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度
	char read_buffer[10]; //fread函数用于接收数据的内存地址
	FILE *read_fp;        //存储网口查询命令的返回值
	int read_count;       //存储fread函数的返回值
	int CRC_num = 0;      //CRC校验结果
	memset(read_buffer,0,10);
	//read_fp = popen("ifconfig eth0 | grep RUNNING", "r"); //网口0 执行shell命令，返回值是个标准I/O流 grep查找带RUNNING字符的行
	read_fp = popen("ifconfig eth1 | grep RUNNING", "r"); //网口1
	if(read_fp != NULL)  
	{
		read_count = fread(read_buffer,sizeof(char),10,read_fp); //从文件流中读数据，调用成功返回实际读取到的项个数，不成功或读到文件末尾返回0
		if(read_count > 0)   //有读取到数据，即找到带RUNNING字符的行，即有插入网线
		{
			sendbuf[8]=0x01; //有插入网线
		}  
		else  
		{
			sendbuf[8]=0x00; //无插入网线
		}  
		pclose(read_fp);     //关闭由 popen() 打开的管道
	}
	//发送LAN状态标志给前面板
	sendbuf[0]=0x00;
	sendbuf[1]=0x10;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x98;
	sendbuf[4]=0x00;
	sendbuf[5]=0x01;
	sendbuf[6]=0x02;
	sendbuf[7]=0x00;
	CRC_num = Modbus_CRC16(sendbuf,9); //CRC校验
	sendbuf[9]=CRC_num/256;
	sendbuf[10]=CRC_num%256;
	sendbuf[11]=0x0d;
	sendbuf[12]=0x0a;				
	sendbuf_len=13;
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
}

/******************************************/
//读取存储任意曲线目录下任意曲线条数
/******************************************/
int read_CurveList(char *basePath)
{
    DIR *dir;           //目录句柄
    struct dirent *ptr; //目录结构体
    char base[1000];    //存储目录
    if ((dir=opendir(basePath)) == NULL) //打开目录
    {
         perror("Open dir error...");  //打开目录失败
         exit(1);
    }
    while ((ptr=readdir(dir)) != NULL) //读取目录 返回值为目录流的下个目录进入点
    {
         if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    //当前目录 或 上一个目录
             continue;
         else if(ptr->d_type == 4)  //为目录类型
         {
			 share_uart->Curve_Line_Count++; //任意曲线总条数加一
             memset(base,'\0',sizeof(base));
             strcpy(base,basePath);
             strcat(base,"/");
             strcat(base,ptr->d_name); //子目录
             read_CurveList(base);     //进入子目录
         }
    }
    closedir(dir);
    return share_uart->Curve_Line_Count; //返回任意曲线总条数
}

/******************************************/
//发送Table模式下曲线总条数
/******************************************/
void TableMode_Curve_CountSet(void)
{
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度
	int CRC_num = 0;      	   //CRC校验结果
	int Cur_Count=0;           //任意曲线总条数
	share_uart->Curve_Line_Count = 0;    //每次发送前将共享内存曲线条数清0后再根据实际情况采集条数
	Cur_Count = read_CurveList(DirPath); //读取存储任意曲线目录下任意曲线条数
	sendbuf[0]=0x00;
	sendbuf[1]=0x10;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x9D;
	sendbuf[4]=0x00;
	sendbuf[5]=0x01;
	sendbuf[6]=0x02;
	sendbuf[7]=Cur_Count/256;
	sendbuf[8]=Cur_Count%256;
	CRC_num = Modbus_CRC16(sendbuf,9); //CRC校验
	sendbuf[9]=CRC_num/256;
	sendbuf[10]=CRC_num%256;
	sendbuf[11]=0x0d;
	sendbuf[12]=0x0a;	
	sendbuf_len=13;
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
	memset(sendbuf,0,sendbuf_len);      //清空要发送给串口的数据
	sendbuf_len = 0;                    //清零要发送给串口的数据长度		
}

/******************************************/
//查询读取任意曲线的总条数 (暂时不用)
/******************************************/
void Read_CurveCount_Check(void)
{
	char sendbuf[200] = {'0'}; //存储要发送给串口的数据
	int sendbuf_len = 0;       //要发送给串口的数据长度
	int CRC_num = 0;      	   //CRC校验结果		
	sendbuf[0]=0x00;
	sendbuf[1]=0x03;	
	sendbuf[2]=0x00;
	sendbuf[3]=0x9F;
	sendbuf[4]=0x00;
	sendbuf[5]=0x01;
	CRC_num = Modbus_CRC16(sendbuf,6);  //CRC校验
	sendbuf[6]=CRC_num/256;
	sendbuf[7]=CRC_num%256;
	sendbuf[8]=0x0d;
	sendbuf[9]=0x0a;		
	sendbuf_len=10;
	tcflush(fd_uart, TCIOFLUSH);        //向串口发送数据之前先清空输入输出队列
	write(fd_uart,sendbuf,sendbuf_len); //串口发送数据
	memset(sendbuf,0,sendbuf_len);      //清空要发送给串口的数据
	sendbuf_len = 0;                    //清零要发送给串口的数据长度	
}
