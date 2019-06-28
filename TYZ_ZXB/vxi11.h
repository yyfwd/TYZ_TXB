#ifndef _VXI11_H_RPCGEN
#define _VXI11_H_RPCGEN

#include "udpuart.h"
#include <rpc/rpc.h>
#include <ctype.h>

#include "scpi/scpi.h" //SCPI指令解析器头文件 
#include "scpi-def.h"

#define SCPI_CHECK(cmd)  SCPI_Input(&scpi_context, cmd, strlen(cmd)) //定义SCPI指令解析器函数

#ifdef __cplusplus
extern "C" {
#endif

/********************************************指令ID定义********************************************/
//设置指令
#define OUTPUT_ON           44  //开机
#define OUTPUT_OFF          45  //关机
#define MODE_CC             59  //CC模式
#define MODE_FIXed          59  //FIXed模式
#define MODE_SAS            60  //SAS模式
#define MODE_Table          61  //Table模式
#define SET_SAS_IMP         62  //设置曲线峰值功率点处电流
#define SET_SAS_ISC         63  //设置短路电流
#define SET_SAS_VMP         95  //设置曲线峰值功率点处电压
#define SET_SAS_VOC         96  //设置开路电压
#define SET_CC_CURR         146 //设置CC(FIXed)模式下电流
#define SET_CC_VOLT         147 //设置CC(FIXed)模式下电压
#define CALIBRATION_VOLTAGE 149 //设置显示电压校正系数
#define CALIBRATION_CURRENT 153 //设置显示电流分段校正系数
#define CALIBRATION_ISC     151 //设置曲线ISC分段校正系数
#define SERIAL_NUMBER       180 //设置模拟器编码
#define SET_SEL_NUM  		190 //上位机选择曲线号开机

//发任意曲线指令
#define SET_TABL_CURR       501 //设置任意曲线电流值
#define SET_TABL_VOLT       502 //设置任意曲线电压值

#define SET_SAVE_CURR1      511 //保存任意曲线电流数据1
#define SET_SAVE_CURR2      512 //保存任意曲线电流数据2
#define SET_SAVE_CURR3      513 //保存任意曲线电流数据3
#define SET_SAVE_CURR4      514 //保存任意曲线电流数据4
#define SET_SAVE_CURR5      515 //保存任意曲线电流数据5
#define SET_SAVE_CURR6      516 //保存任意曲线电流数据6
#define SET_SAVE_CURR7      517 //保存任意曲线电流数据7
#define SET_SAVE_CURR8      518 //保存任意曲线电流数据8
#define SET_SAVE_CURR9      519 //保存任意曲线电流数据9
#define SET_SAVE_CURR10     520 //保存任意曲线电流数据10
#define SET_SAVE_CURR11     521 //保存任意曲线电流数据11
#define SET_SAVE_CURR12     522 //保存任意曲线电流数据12
#define SET_SAVE_CURR13     523 //保存任意曲线电流数据13
#define SET_SAVE_CURR14     524 //保存任意曲线电流数据14
#define SET_SAVE_CURR15     525 //保存任意曲线电流数据15

#define SET_SAVE_VOLT1      531 //保存任意曲线电压数据1
#define SET_SAVE_VOLT2      532 //保存任意曲线电压数据2
#define SET_SAVE_VOLT3      533 //保存任意曲线电压数据3
#define SET_SAVE_VOLT4      534 //保存任意曲线电压数据4
#define SET_SAVE_VOLT5      535 //保存任意曲线电压数据5
#define SET_SAVE_VOLT6      536 //保存任意曲线电压数据6
#define SET_SAVE_VOLT7      537 //保存任意曲线电压数据7
#define SET_SAVE_VOLT8      538 //保存任意曲线电压数据8
#define SET_SAVE_VOLT9      539 //保存任意曲线电压数据9
#define SET_SAVE_VOLT10     540 //保存任意曲线电压数据10
#define SET_SAVE_VOLT11     541 //保存任意曲线电压数据11
#define SET_SAVE_VOLT12     542 //保存任意曲线电压数据12
#define SET_SAVE_VOLT13     543 //保存任意曲线电压数据13
#define SET_SAVE_VOLT14     544 //保存任意曲线电压数据14
#define SET_SAVE_VOLT15     545 //保存任意曲线电压数据15
//查询指令
#define OUTPUT_STATE        301  //查询开关机状态
#define MODE_STATE          302  //查询模式设置
#define CHECK_SAS_IMP       303  //查询曲线峰值功率点处电流
#define CHECK_SAS_ISC       304  //查询短路电流
#define CHECK_SAS_VMP       305  //查询曲线峰值功率点处电压
#define CHECK_SAS_VOC       306  //查询开路电压
#define CHECK_CC_CURR       307  //查询CC(FIXed)模式下电流
#define CHECK_CC_VOLT       308  //查询CC(FIXed)模式下电压
#define CHECK_CURR          309  //查询实时电流
#define CHECK_VOLT          310  //查询实时电压
#define CHECK_ID            311  //ID查询
#define CHECK_SYSTEM_ERROR  312  //系统错误查询
#define CHECK_MACHINE_ERROR 314  //查询机器故障

#define CHECK_CURVE_COUNT   320  //查询Table模式下曲线的总条数

#define Enable      1
#define Disable     0

typedef struct share_mem  //定义一个结构体，用于进程共享
{
	char vxi11_current_buf[20]; //存储实时电流数据，将发送到上位机
	char vxi11_voltage_buf[20]; //存储实时电压数据，将发送到上位机
	char vxi11_error_buf[20];   //存储故障状态数据，将发送到上位机
	
	char vxi11_onoff_buf[20]; //存储开关机状态字节，将发送到上位机
	char vxi11_mode_buf[20];  //存储模式状态字节，将发送到上位机
	
	char vxi11_IMP_buf[20]; //存储曲线峰值功率点处电流，将发送到上位机
	char vxi11_ISC_buf[20]; //存储设置短路电流，将发送到上位机
	char vxi11_VMP_buf[20]; //存储曲线峰值功率点处电压，将发送到上位机
	char vxi11_VOC_buf[20]; //存储开路电压，将发送到上位机
	char vxi11_cc_current[20]; //存储CC模式下电流，将发送到上位机
	char vxi11_cc_voltage[20]; //存储CC模式下电压，将发送到上位机
	
	double ID_parameter_pack[120];    //ID和参数包，存储数据库解析ID和参数，数组队列的存储格式为 ID + 参数个数（可以为0） + 参数列表（如果个数不为0）120可调整 类型为double，可参数保存小数位
	int vxi_ID_parameter_pack_count;  //指令入队列指针位置
	char save_serial_number[20];      //存储模拟器编码
	
	char volt_data[200000]; //任意曲线电压数据
	char curr_data[200000]; //任意曲线电流数据
	//int anydata_ok; //任意曲线全部数据接收完成 包括电流电压
	
	int anydata_fromPC_flag;    //从上位机下发曲线标志位
	int anydata_savetxb_ok;     //存储上位机发送的任意曲线数据到通讯板标志位
	char curve_Num;             //任意曲线曲线号
	char Table_OnOff_Flag;      //前面板发送Table模式开机标志位
	int Curve_Line_Count;       //任意曲线总条数
	char Curve_Name[20];        //从上位机下载任意曲线到通讯板保存的曲线名称
	int gpib_addr_id;  //前面板设置的GPIB地址
}share_mem;

typedef struct
{
	unsigned char Address[4]; //IP地址
	unsigned char Netmask[4]; //子网掩码
	unsigned char Gateway[4]; //网关
}LAN_SET; //IP地址设置

/********************************************变量定义********************************************/
int fd_uart;  //串口文件
share_mem *share_vxi11;   //父进程共享内存结构体
share_mem *share_uart;    //子进程共享内存结构体

extern scpi_id;                    //SCPI指令经解析后得到的ID
extern double scpi_parameter[100]; //存储带参数的指令的参数值
extern scpi_parameter_count;       //统计此条指令带多少个参数
extern char serial_number[20];     //存储模拟器编码

LAN_SET LAN;    //IP地址设置

//任意曲线相关变量
int anydata_take;        //是否已经从字符串中提取数值
int anydata_file_take;   //是否已经从文件中提取数值
int send_count;     //发送第几帧的数据包
int send_TXB_count; //发送第几帧的数据包

double curr_savedata[10000]; //8185个保存电流值 浮点型
double volt_savedata[10000]; //8185个保存电压值 浮点型
int Iint[10000]; 			//8185个电流值 整数型
int voc_sa;  			//开路电压
int nv_ratio;			//数据点数与开路电压的比值
int Volt_inh;			//封锁信号
int Curr_sc; 			//短路电流
int Curr_Path;          //封锁信号计算参数

/********************************************函数声明********************************************/
int Modbus_CRC16(char *Buff_addr,int len); //CRC校验
void IP_check(void);          //发送IP查询命令
void read_onoff_mode(void);   //返回模拟器开关机状态/模式
void read_status_check(void); //读取模拟器的状态参数
void measure_voltage_check(void); //查询CC(FIXed)模式下电压
void GPIB_check(void);            //查询模拟器GPIB地址
void SAS_check(void);             //返回SAS各个值
void measure_current_check(void); //查询CC(FIXed)模式下电流
void LAN_check(void);             //网络连接状态检查
int read_CurveList(char *basePath);  //读取存储任意曲线目录下任意曲线条数
void TableMode_Curve_CountSet(void); //发送Table模式下曲线总条数
void Read_CurveCount_Check(void);    //查询读取任意曲线的总条数 (暂时不用)

void anydata_send(void); //发送上位机下发的曲线给前面板
void AnyData_FromTXB_Dispose(void); //从通讯板下发曲线给前面板 (有上位机选择曲线号开机、前面板选择曲线号开机两种情况)
void Anydata_SavetoTXB_Dispose(void); //从上位机下载任意曲线到通讯板保存

void vxi11_init(void); //vxi11初始化
void uart_init(void);  //串口初始化
void IP_init(void);    //检测本机IP 用于与前面板设置的IP比较
void device_async_1(struct svc_req *rqstp, register SVCXPRT *transp); //用于VXI11初始化
void device_core_1(struct svc_req *rqstp, register SVCXPRT *transp); //用于VXI11初始化
void device_intr_1(struct svc_req *rqstp, register SVCXPRT *transp); //用于VXI11初始化




typedef long Device_Link;

enum Device_AddrFamily {
	DEVICE_TCP = 0, DEVICE_UDP = 1,
};
typedef enum Device_AddrFamily Device_AddrFamily;

typedef long Device_Flags;

typedef long Device_ErrorCode;

struct Device_Error {
	Device_ErrorCode error;
};
typedef struct Device_Error Device_Error;

struct Create_LinkParms {
	long clientId;
	bool_t lockDevice;
	u_long lock_timeout;
	char *device;
};
typedef struct Create_LinkParms Create_LinkParms;

struct Create_LinkResp {
	Device_ErrorCode error;
	Device_Link lid;
	u_short abortPort;
	u_long maxRecvSize;
};
typedef struct Create_LinkResp Create_LinkResp;

struct Device_WriteParms {
	Device_Link lid;
	u_long io_timeout;
	u_long lock_timeout;
	Device_Flags flags;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct Device_WriteParms Device_WriteParms;

struct Device_WriteResp {
	Device_ErrorCode error;
	u_long size;
};
typedef struct Device_WriteResp Device_WriteResp;

struct Device_ReadParms {
	Device_Link lid;
	u_long requestSize;
	u_long io_timeout;
	u_long lock_timeout;
	Device_Flags flags;
	char termChar;
};
typedef struct Device_ReadParms Device_ReadParms;

struct Device_ReadResp {
	Device_ErrorCode error;
	long reason;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct Device_ReadResp Device_ReadResp;

struct Device_ReadStbResp {
	Device_ErrorCode error;
	u_char stb;
};
typedef struct Device_ReadStbResp Device_ReadStbResp;

struct Device_GenericParms {
	Device_Link lid;
	Device_Flags flags;
	u_long lock_timeout;
	u_long io_timeout;
};
typedef struct Device_GenericParms Device_GenericParms;

struct Device_RemoteFunc {
	u_long hostAddr;
	u_long hostPort;
	u_long progNum;
	u_long progVers;
	Device_AddrFamily progFamily;
};
typedef struct Device_RemoteFunc Device_RemoteFunc;

struct Device_EnableSrqParms {
	Device_Link lid;
	bool_t enable;
	struct {
		u_int handle_len;
		char *handle_val;
	} handle;
};
typedef struct Device_EnableSrqParms Device_EnableSrqParms;

struct Device_LockParms {
	Device_Link lid;
	Device_Flags flags;
	u_long lock_timeout;
};
typedef struct Device_LockParms Device_LockParms;

struct Device_DocmdParms {
	Device_Link lid;
	Device_Flags flags;
	u_long io_timeout;
	u_long lock_timeout;
	long cmd;
	bool_t network_order;
	long datasize;
	struct {
		u_int data_in_len;
		char *data_in_val;
	} data_in;
};
typedef struct Device_DocmdParms Device_DocmdParms;

struct Device_DocmdResp {
	Device_ErrorCode error;
	struct {
		u_int data_out_len;
		char *data_out_val;
	} data_out;
};
typedef struct Device_DocmdResp Device_DocmdResp;

struct Device_SrqParms {
	struct {
		u_int handle_len;
		char *handle_val;
	} handle;
};
typedef struct Device_SrqParms Device_SrqParms;

void Datascpi_Deal(char *datacode);

#define DEVICE_ASYNC 0x0607B0
#define DEVICE_ASYNC_VERSION 1

#if defined(__STDC__) || defined(__cplusplus)
#define device_abort 1
extern Device_Error * device_abort_1(Device_Link *, CLIENT *);
extern Device_Error * device_abort_1_svc(Device_Link *, struct svc_req *);
extern int device_async_1_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define device_abort 1
extern Device_Error * device_abort_1();
extern Device_Error * device_abort_1_svc();
extern int device_async_1_freeresult ();
#endif /* K&R C */

#define DEVICE_CORE 0x0607AF
#define DEVICE_CORE_VERSION 1

#if defined(__STDC__) || defined(__cplusplus)
#define create_link 10
extern Create_LinkResp * create_link_1(Create_LinkParms *, CLIENT *);
extern Create_LinkResp * create_link_1_svc(Create_LinkParms *,
		struct svc_req *);
#define device_write 11
extern Device_WriteResp * device_write_1(Device_WriteParms *, CLIENT *);
extern Device_WriteResp * device_write_1_svc(Device_WriteParms *,
		struct svc_req *);
#define device_read 12
extern Device_ReadResp * device_read_1(Device_ReadParms *, CLIENT *);
extern Device_ReadResp * device_read_1_svc(Device_ReadParms *,
		struct svc_req *);
#define device_readstb 13
extern Device_ReadStbResp * device_readstb_1(Device_GenericParms *, CLIENT *);
extern Device_ReadStbResp * device_readstb_1_svc(Device_GenericParms *,
		struct svc_req *);
#define device_trigger 14
extern Device_Error * device_trigger_1(Device_GenericParms *, CLIENT *);
extern Device_Error * device_trigger_1_svc(Device_GenericParms *,
		struct svc_req *);
#define device_clear 15
extern Device_Error * device_clear_1(Device_GenericParms *, CLIENT *);
extern Device_Error * device_clear_1_svc(Device_GenericParms *,
		struct svc_req *);
#define device_remote 16
extern Device_Error * device_remote_1(Device_GenericParms *, CLIENT *);
extern Device_Error * device_remote_1_svc(Device_GenericParms *,
		struct svc_req *);
#define device_local 17
extern Device_Error * device_local_1(Device_GenericParms *, CLIENT *);
extern Device_Error * device_local_1_svc(Device_GenericParms *,
		struct svc_req *);
#define device_lock 18
extern Device_Error * device_lock_1(Device_LockParms *, CLIENT *);
extern Device_Error * device_lock_1_svc(Device_LockParms *, struct svc_req *);
#define device_unlock 19
extern Device_Error * device_unlock_1(Device_Link *, CLIENT *);
extern Device_Error * device_unlock_1_svc(Device_Link *, struct svc_req *);
#define device_enable_srq 20
extern Device_Error * device_enable_srq_1(Device_EnableSrqParms *, CLIENT *);
extern Device_Error * device_enable_srq_1_svc(Device_EnableSrqParms *,
		struct svc_req *);
#define device_docmd 22
extern Device_DocmdResp * device_docmd_1(Device_DocmdParms *, CLIENT *);
extern Device_DocmdResp * device_docmd_1_svc(Device_DocmdParms *,
		struct svc_req *);
#define destroy_link 23
extern Device_Error * destroy_link_1(Device_Link *, CLIENT *);
extern Device_Error * destroy_link_1_svc(Device_Link *, struct svc_req *);
#define create_intr_chan 25
extern Device_Error * create_intr_chan_1(Device_RemoteFunc *, CLIENT *);
extern Device_Error * create_intr_chan_1_svc(Device_RemoteFunc *,
		struct svc_req *);
#define destroy_intr_chan 26
extern Device_Error * destroy_intr_chan_1(void *, CLIENT *);
extern Device_Error * destroy_intr_chan_1_svc(void *, struct svc_req *);
extern int device_core_1_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define create_link 10
extern Create_LinkResp * create_link_1();
extern Create_LinkResp * create_link_1_svc();
#define device_write 11
extern Device_WriteResp * device_write_1();
extern Device_WriteResp * device_write_1_svc();
#define device_read 12
extern Device_ReadResp * device_read_1();
extern Device_ReadResp * device_read_1_svc();
#define device_readstb 13
extern Device_ReadStbResp * device_readstb_1();
extern Device_ReadStbResp * device_readstb_1_svc();
#define device_trigger 14
extern Device_Error * device_trigger_1();
extern Device_Error * device_trigger_1_svc();
#define device_clear 15
extern Device_Error * device_clear_1();
extern Device_Error * device_clear_1_svc();
#define device_remote 16
extern Device_Error * device_remote_1();
extern Device_Error * device_remote_1_svc();
#define device_local 17
extern Device_Error * device_local_1();
extern Device_Error * device_local_1_svc();
#define device_lock 18
extern Device_Error * device_lock_1();
extern Device_Error * device_lock_1_svc();
#define device_unlock 19
extern Device_Error * device_unlock_1();
extern Device_Error * device_unlock_1_svc();
#define device_enable_srq 20
extern Device_Error * device_enable_srq_1();
extern Device_Error * device_enable_srq_1_svc();
#define device_docmd 22
extern Device_DocmdResp * device_docmd_1();
extern Device_DocmdResp * device_docmd_1_svc();
#define destroy_link 23
extern Device_Error * destroy_link_1();
extern Device_Error * destroy_link_1_svc();
#define create_intr_chan 25
extern Device_Error * create_intr_chan_1();
extern Device_Error * create_intr_chan_1_svc();
#define destroy_intr_chan 26
extern Device_Error * destroy_intr_chan_1();
extern Device_Error * destroy_intr_chan_1_svc();
extern int device_core_1_freeresult ();
#endif /* K&R C */

#define DEVICE_INTR 0x0607B1
#define DEVICE_INTR_VERSION 1

#if defined(__STDC__) || defined(__cplusplus)
#define device_intr_srq 30
extern void * device_intr_srq_1(Device_SrqParms *, CLIENT *);
extern void * device_intr_srq_1_svc(Device_SrqParms *, struct svc_req *);
extern int device_intr_1_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define device_intr_srq 30
extern void * device_intr_srq_1();
extern void * device_intr_srq_1_svc();
extern int device_intr_1_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern bool_t xdr_Device_Link(XDR *, Device_Link*);
extern bool_t xdr_Device_AddrFamily(XDR *, Device_AddrFamily*);
extern bool_t xdr_Device_Flags(XDR *, Device_Flags*);
extern bool_t xdr_Device_ErrorCode(XDR *, Device_ErrorCode*);
extern bool_t xdr_Device_Error(XDR *, Device_Error*);
extern bool_t xdr_Create_LinkParms(XDR *, Create_LinkParms*);
extern bool_t xdr_Create_LinkResp(XDR *, Create_LinkResp*);
extern bool_t xdr_Device_WriteParms(XDR *, Device_WriteParms*);
extern bool_t xdr_Device_WriteResp(XDR *, Device_WriteResp*);
extern bool_t xdr_Device_ReadParms(XDR *, Device_ReadParms*);
extern bool_t xdr_Device_ReadResp(XDR *, Device_ReadResp*);
extern bool_t xdr_Device_ReadStbResp(XDR *, Device_ReadStbResp*);
extern bool_t xdr_Device_GenericParms(XDR *, Device_GenericParms*);
extern bool_t xdr_Device_RemoteFunc(XDR *, Device_RemoteFunc*);
extern bool_t xdr_Device_EnableSrqParms(XDR *, Device_EnableSrqParms*);
extern bool_t xdr_Device_LockParms(XDR *, Device_LockParms*);
extern bool_t xdr_Device_DocmdParms(XDR *, Device_DocmdParms*);
extern bool_t xdr_Device_DocmdResp(XDR *, Device_DocmdResp*);
extern bool_t xdr_Device_SrqParms(XDR *, Device_SrqParms*);

#else /* K&R C */
extern bool_t xdr_Device_Link ();
extern bool_t xdr_Device_AddrFamily ();
extern bool_t xdr_Device_Flags ();
extern bool_t xdr_Device_ErrorCode ();
extern bool_t xdr_Device_Error ();
extern bool_t xdr_Create_LinkParms ();
extern bool_t xdr_Create_LinkResp ();
extern bool_t xdr_Device_WriteParms ();
extern bool_t xdr_Device_WriteResp ();
extern bool_t xdr_Device_ReadParms ();
extern bool_t xdr_Device_ReadResp ();
extern bool_t xdr_Device_ReadStbResp ();
extern bool_t xdr_Device_GenericParms ();
extern bool_t xdr_Device_RemoteFunc ();
extern bool_t xdr_Device_EnableSrqParms ();
extern bool_t xdr_Device_LockParms ();
extern bool_t xdr_Device_DocmdParms ();
extern bool_t xdr_Device_DocmdResp ();
extern bool_t xdr_Device_SrqParms ();


#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_VXI11_H_RPCGEN */
