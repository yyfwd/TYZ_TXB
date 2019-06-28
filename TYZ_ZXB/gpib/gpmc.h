
#ifndef __GPMC_H
#define __GPMC_H

int Mem_Init();
int Gpmc_Init();
void Gpmc_free();
int Mem_free();

void TNT_Out(unsigned int address, char data);
char TNT_In(unsigned int address);
void TNT_Out_Word(unsigned int address, short data);
short TNT_In_Word(unsigned int address);

/* GPMC register offsets */
#define GPMC_REVISION 0x00
#define GPMC_SYSCONFIG 0x10
#define GPMC_SYSSTATUS 0x14
#define GPMC_IRQSTATUS 0x18
#define GPMC_IRQENABLE 0x1c
#define GPMC_TIMEOUT_CONTROL 0x40
#define GPMC_ERR_ADDRESS 0x44
#define GPMC_ERR_TYPE 0x48
#define GPMC_CONFIG 0x50
#define GPMC_STATUS 0x54
#define GPMC_PREFETCH_CONFIG1 0x1e0
#define GPMC_PREFETCH_CONFIG2 0x1e4
#define GPMC_PREFETCH_CONTROL 0x1ec
#define GPMC_PREFETCH_STATUS 0x1f0
#define GPMC_ECC_CONFIG 0x1f4
#define GPMC_ECC_CONTROL 0x1f8
#define GPMC_ECC_SIZE_CONFIG 0x1fc
#define GPMC_ECC1_RESULT 0x200
#define GPMC_ECC_BCH_RESULT_0 0x240

#define GPMC_CONFIG1_1 0x90
#define GPMC_CONFIG2_1 0x94
#define GPMC_CONFIG3_1 0x98
#define GPMC_CONFIG4_1 0x9c
#define GPMC_CONFIG5_1 0xA0
#define GPMC_CONFIG6_1 0xA4
#define GPMC_CONFIG7_1 0xA8

#define STNOR_GPMC_CONFIG1 0x00000001
#define STNOR_GPMC_CONFIG2 0x001B1B01 //0：CS的开始 1：读访问时CS的结束 2：写访问时CS的结束
#define STNOR_GPMC_CONFIG3 0x00020201
#define STNOR_GPMC_CONFIG4 0x10031003 //0：写的开始 1：写的结束 2：读的开始 3：读的结束
#define STNOR_GPMC_CONFIG5 0x000F1D1D //0：总共的写入周期时间 1：总共的读取周期时间 2：在第一个开始周期时间和第一个有效数据之间的延时
#define STNOR_GPMC_CONFIG6 0x02000F80
#define STNOR_GPMC_CONFIG7 0x00000f41

#endif
