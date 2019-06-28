#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "gpmc.h"

int fd;

char * gpmc_reg_virtual_base;
char * gpmc_cs1_virtual_base;

#define GPMC_REG_MAP_SIZE 16*1024*1024
#define GPMC_REG_MAP_BASE 0x50000000

#define GPMC_CS1_MAP_SIZE 16*1024*1024
#define GPMC_CS1_MAP_BASE 0x1000000

void gpmc_write_reg(unsigned int address, unsigned int data)
{
	*(unsigned int *) (gpmc_reg_virtual_base + address) = data;
}

unsigned int gpmc_read_reg(unsigned int address)
{
	return *(unsigned int *) (gpmc_reg_virtual_base + address);
}

int Mem_Init()
{
	//打开内存管理器
	if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
		printf("ERROR: could not open \"/dev/mem\"...\n");
		return (1);
	}
	// GPMC寄存器地址映射到虚拟地址
	gpmc_reg_virtual_base = mmap(0, GPMC_REG_MAP_SIZE, (PROT_READ | PROT_WRITE),
			MAP_SHARED, fd, GPMC_REG_MAP_BASE);
	if (gpmc_reg_virtual_base == MAP_FAILED ) {
		printf("gpmc_reg_virtual_base ERROR: mmap() failed...\n");
		close(fd);
		return (1);
	}
	return 0;
}

int Gpmc_Init()
{
	unsigned int val;
	gpmc_write_reg(GPMC_CONFIG7_1, STNOR_GPMC_CONFIG7);
	gpmc_write_reg(GPMC_CONFIG1_1, STNOR_GPMC_CONFIG1);
	gpmc_write_reg(GPMC_CONFIG2_1, STNOR_GPMC_CONFIG2);
	gpmc_write_reg(GPMC_CONFIG3_1, STNOR_GPMC_CONFIG3);
	gpmc_write_reg(GPMC_CONFIG4_1, STNOR_GPMC_CONFIG4);
	gpmc_write_reg(GPMC_CONFIG5_1, STNOR_GPMC_CONFIG5);
	gpmc_write_reg(GPMC_CONFIG6_1, STNOR_GPMC_CONFIG6);

	val = gpmc_read_reg(GPMC_REVISION);
	printf("[GPMC_Test] GPMC revision %d.%d\n", (val >> 4) & 0x0f, val & 0x0f);

	val = gpmc_read_reg(GPMC_CONFIG);
	gpmc_write_reg(GPMC_CONFIG, (val & 0xFFFFFFFD));	//GPMC控制所有的地址
	// GPMC CS1地址映射到虚拟地址
	gpmc_cs1_virtual_base = mmap(0, GPMC_CS1_MAP_SIZE, (PROT_READ | PROT_WRITE),
			MAP_SHARED, fd, GPMC_CS1_MAP_BASE);
	if (gpmc_cs1_virtual_base == MAP_FAILED ) {
		printf("gpmc_cs1_virtual_base ERROR: mmap() failed...\n");
		close(fd);
		return (1);
	}
	return 0;
}

int Mem_free()
{
	if ((munmap(gpmc_reg_virtual_base, GPMC_REG_MAP_SIZE) != 0)
			&& (munmap(gpmc_cs1_virtual_base, GPMC_CS1_MAP_SIZE) != 0)) {
		printf("ERROR: munmap() failed...\n");
		close(fd);
		return (1);
	}
	close(fd);
	return 0;
}

void Gpmc_free()
{
	unsigned int l;

	l = gpmc_read_reg(GPMC_CONFIG7_1);
	l &= ~(1 << 6);
	gpmc_write_reg(GPMC_CONFIG7_1, l);
}

void TNT_Out(unsigned int address, char data)
{
	unsigned int add = 0, addh = 0, addl = 0;
	addl = (address & 0x07) << 1;
	addh = (address & 0x38) << 3;
	add = addh | addl;
	//printf("[GPMC_Test] write 0x%x = 0x%x \n", address, data);

	*(char *) (gpmc_cs1_virtual_base + add) = data;
}

char TNT_In(unsigned int address)
{
	char data = 0x00;
	unsigned int add = 0, addh = 0, addl = 0;
	addl = (address & 0x07) << 1;
	addh = (address & 0x38) << 3;
	add = addh | addl;

	data = *(char *) (gpmc_cs1_virtual_base + add);

	//printf("[GPMC_Test] read 0x%x = 0x%x \n", address, data);

	return data;
}

void TNT_Out_Word(unsigned int address, short data)
{
	unsigned int add = 0, addh = 0, addl = 0;
	addl = (address & 0x07) << 1;
	addh = (address & 0x38) << 3;
	add = addh | addl;

	//printf("[GPMC_Test] write 0x%x = 0x%x \n", address, data);
	*(short *) (gpmc_cs1_virtual_base + add) = data;
}

short TNT_In_Word(unsigned int address)
{
	short data = 0;
	unsigned int add = 0, addh = 0, addl = 0;
	addl = (address & 0x07) << 1;
	addh = (address & 0x38) << 3;
	add = addh | addl;

	data = *(short *)(gpmc_cs1_virtual_base + add);

	//printf("[GPMC_Test] data 0x%x = 0x%x \n", address, data);

	return data;
}
