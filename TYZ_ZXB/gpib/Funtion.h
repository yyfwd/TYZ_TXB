
#ifndef __FUNTION_H
#define __FUNTION_H


typedef struct  Scpi_code02{ //定义一个结构体
	char code1[8][20];
}SCPI2;
/*typedef struct  GPIB_Read_Data{ //定义一个结构体
	int Read_Count;
	char *Read_BufVal;
}GPIB_Read_Data;*/
extern  int Old_GPIBAddrID;
extern  int New_GPIBAddrID;
extern  int scpi_check_id;
extern  int volt_get;
extern  int curr_get;
extern  int volt_save;
extern  int curr_save;
extern int GPIB_ReadCount;
extern char *GPIB_ReadBuf;

int AhyData_Analy(char *data,int ID);
int AnyData_SaveMode(char *data,int IDSave);
void  GPIB_AddrId_Check(void);
#endif

