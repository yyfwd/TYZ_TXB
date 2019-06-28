#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "scpi/scpi.h"
//#include "scpi-def.h"
#include "vxi11.h"

int scpi_id = 0; //SCPI指令经解析后得到的ID
double scpi_parameter[100] = {0}; //存储带参数的指令的参数值
int scpi_parameter_count = 0;//统计此条指令带多少个参数
char serial_number[20]; //存储模拟器编码

struct _scpi_channel_value_t {
    int32_t row;
    int32_t col;
};
typedef struct _scpi_channel_value_t scpi_channel_value_t;

//判断通道
static scpi_result_t CHECK_Chanlst(scpi_t *context) {
    scpi_parameter_t channel_list_param;
#define MAXROW 2    /* maximum number of rows */
#define MAXCOL 6    /* maximum number of columns */
#define MAXDIM 2    /* maximum number of dimensions */
    scpi_channel_value_t array[MAXROW * MAXCOL]; /* array which holds values in order (2D) */
    size_t chanlst_idx; /* index for channel list */
    size_t arr_idx = 0; /* index for array */
    size_t n, m = 1; /* counters for row (n) and columns (m) */

    /* get channel list */
    if (SCPI_Parameter(context, &channel_list_param, TRUE)) {
        scpi_expr_result_t res;
        scpi_bool_t is_range;
        int32_t values_from[MAXDIM];
        int32_t values_to[MAXDIM];
        size_t dimensions;

        bool for_stop_row = FALSE; /* true if iteration for rows has to stop */
        bool for_stop_col = FALSE; /* true if iteration for columns has to stop */
        int32_t dir_row = 1; /* direction of counter for rows, +/-1 */
        int32_t dir_col = 1; /* direction of counter for columns, +/-1 */

        /* the next statement is valid usage and it gets only real number of dimensions for the first item (index 0) */
        if (!SCPI_ExprChannelListEntry(context, &channel_list_param, 0, &is_range, NULL, NULL, 0, &dimensions)) {
            chanlst_idx = 0; /* call first index */
            arr_idx = 0; /* set arr_idx to 0 */
            do { /* if valid, iterate over channel_list_param index while res == valid (do-while cause we have to do it once) */
                res = SCPI_ExprChannelListEntry(context, &channel_list_param, chanlst_idx, &is_range, values_from, values_to, 4, &dimensions);
                if (is_range == 0) { /* still can have multiple dimensions */
                    if (dimensions == 1) {
                        /* here we have our values
                         * row == values_from[0]
                         * col == 0 (fixed number)
                         * call a function or something */
                        array[arr_idx].row = values_from[0];
                        array[arr_idx].col = 0;
                    } else if (dimensions == 2) {
                        /* here we have our values
                         * row == values_fom[0]
                         * col == values_from[1]
                         * call a function or something */
                        array[arr_idx].row = values_from[0];
                        array[arr_idx].col = values_from[1];
                    } else {
                        return SCPI_RES_ERR;
                    }
                    arr_idx++; /* inkrement array where we want to save our values to, not neccessary otherwise */
                    if (arr_idx >= MAXROW * MAXCOL) {
                        return SCPI_RES_ERR;
                    }
                } else if (is_range == 1) {
                    if (values_from[0] > values_to[0]) {
                        dir_row = -1; /* we have to decrement from values_from */
                    } else { /* if (values_from[0] < values_to[0]) */
                        dir_row = +1; /* default, we increment from values_from */
                    }

                    /* iterating over rows, do it once -> set for_stop_row = false
                     * needed if there is channel list index isn't at end yet */
                    for_stop_row = FALSE;
                    for (n = values_from[0]; for_stop_row == FALSE; n += dir_row) {
                        /* usual case for ranges, 2 dimensions */
                        if (dimensions == 2) {
                            if (values_from[1] > values_to[1]) {
                                dir_col = -1;
                            } else if (values_from[1] < values_to[1]) {
                                dir_col = +1;
                            }
                            /* iterating over columns, do it at least once -> set for_stop_col = false
                             * needed if there is channel list index isn't at end yet */
                            for_stop_col = FALSE;
                            for (m = values_from[1]; for_stop_col == FALSE; m += dir_col) {
                                /* here we have our values
                                 * row == n
                                 * col == m
                                 * call a function or something */
                                array[arr_idx].row = n;
                                array[arr_idx].col = m;
                                arr_idx++;
                                if (arr_idx >= MAXROW * MAXCOL) {
                                    return SCPI_RES_ERR;
                                }
                                if (m == (size_t)values_to[1]) {
                                    /* endpoint reached, stop column for-loop */
                                    for_stop_col = TRUE;
                                }
                            }
                            /* special case for range, example: (@2!1) */
                        } else if (dimensions == 1) {
                            /* here we have values
                             * row == n
                             * col == 0 (fixed number)
                             * call function or sth. */
                            array[arr_idx].row = n;
                            array[arr_idx].col = 0;
                            arr_idx++;
                            if (arr_idx >= MAXROW * MAXCOL) {
                                return SCPI_RES_ERR;
                            }
                        }
                        if (n == (size_t)values_to[0]) {
                            /* endpoint reached, stop row for-loop */
                            for_stop_row = TRUE;
                        }
                    }


                } else {
                    return SCPI_RES_ERR;
                }
                /* increase index */
                chanlst_idx++;
            } while (SCPI_EXPR_OK == SCPI_ExprChannelListEntry(context, &channel_list_param, chanlst_idx, &is_range, values_from, values_to, 4, &dimensions));
            /* while checks, whether incremented index is valid */
        }
        /* do something at the end if needed */
        /* array[arr_idx].row = 0; */
        /* array[arr_idx].col = 0; */
    }

    {
        size_t i;
        fprintf(stderr, "Chanlst: ");
		if(arr_idx != 0) //有通道参数
		{
			for (i = 0; i< arr_idx; i++) {
				//fprintf(stderr, "%d!%d, ", array[i].row, array[i].col);
				fprintf(stderr, "%d ", array[i].row);
			}			
		}
		else //无通道参数
		{
			fprintf(stderr, "NULL");
		}

        fprintf(stderr, "\r\n");
    }
    return SCPI_RES_OK;
}

/******************************************设置指令调用函数******************************************/
//设置关开机
static scpi_result_t set_ONOFF(scpi_t * context) {
    scpi_bool_t param1;
	
    if (!SCPI_ParamBool(context, &param1, TRUE)) {  //参数转化为布尔值
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_ONOFF = %d\r\n", param1);
	if(param1 == 1)
	{
		scpi_id = OUTPUT_ON;  //返回scpi查询结果
	}
	else
	{
		scpi_id = OUTPUT_OFF;  //返回scpi查询结果
	}
	scpi_parameter_count = 0; //统计此条指令带多少个参数
	CHECK_Chanlst(context);   //判断通道
    return SCPI_RES_OK;
}

//设置模式
static scpi_result_t set_MODE(scpi_t * context) {
    const char * value;
	char MODE[10];
	char CC_MODE[2] = "CC";
	char FIXED_MODE[5] = "FIXed";
	char FIX_MODE[3] = "FIX";
	char SAS_MODE[3] = "SAS";
	char TABLE_MODE[5] = "TABLe";
	char TABL_MODE[5] = "TABL";
    size_t value_len;

    if (!SCPI_ParamCharacters(context, &value, &value_len, TRUE)) {  //参数转化为字符
        return SCPI_RES_ERR;
    }
	strncpy(MODE,value,value_len);  //提取字符
	MODE[value_len] = '\0';
	printf("SET_MODE = %s\r\n", MODE); 
	
	if(strcmp(CC_MODE,MODE) == 0) //CC模式
	{
		scpi_id = MODE_CC;  //返回scpi查询结果
	}
	else if(strcmp(FIXED_MODE,MODE) == 0 || strcmp(FIX_MODE,MODE) == 0) //FIXed FIX 模式
	{
		scpi_id = MODE_FIXed;  //返回scpi查询结果
	}
	else if(strcmp(SAS_MODE,MODE) == 0)  //SAS模式
	{
		scpi_id = MODE_SAS;  //返回scpi查询结果
	}
	else if(strcmp(TABLE_MODE,MODE) == 0 || strcmp(TABL_MODE,MODE) == 0)  //TABLe TABL 模式
	{
		scpi_id = MODE_Table;  //返回scpi查询结果
	}
	scpi_parameter_count = 0; //统计此条指令带多少个参数
	CHECK_Chanlst(context);   //判断通道
    return SCPI_RES_OK;
}

//设置曲线峰值功率点处电流 IMP
static scpi_result_t set_SAS_IMP(scpi_t * context) {
    double param1;

    if (!SCPI_ParamDouble(context, &param1, TRUE)) {  //参数转化为浮点数
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_SAS_IMP %.10lf\r\n", param1);
	scpi_id = SET_SAS_IMP;      //返回scpi查询结果
	scpi_parameter_count = 1;   //统计此条指令带多少个参数
	scpi_parameter[0] = param1; //存储带参数的指令的参数值
	CHECK_Chanlst(context); //判断通道
    return SCPI_RES_OK;
}

//设置短路电流 ISC
static scpi_result_t set_SAS_ISC(scpi_t * context) {
    double param1;

    if (!SCPI_ParamDouble(context, &param1, TRUE)) {  //参数转化为浮点数
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_SAS_ISC %.10lf\r\n", param1);
	scpi_id = SET_SAS_ISC;      //返回scpi查询结果
	scpi_parameter_count = 1;   //统计此条指令带多少个参数
	scpi_parameter[0] = param1; //存储带参数的指令的参数值	
	CHECK_Chanlst(context); //判断通道
    return SCPI_RES_OK;
}

//设置曲线峰值功率点处电压 VMP
static scpi_result_t set_SAS_VMP(scpi_t * context) {
    double param1;

    if (!SCPI_ParamDouble(context, &param1, TRUE)) {  //参数转化为浮点数
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_SAS_VMP %.10lf\r\n", param1);
	scpi_id = SET_SAS_VMP;      //返回scpi查询结果
	scpi_parameter_count = 1;   //统计此条指令带多少个参数
	scpi_parameter[0] = param1; //存储带参数的指令的参数值	
	CHECK_Chanlst(context); //判断通道
    return SCPI_RES_OK;
}

//设置开路电压 VOC
static scpi_result_t set_SAS_VOC(scpi_t * context) {
    double param1;

    if (!SCPI_ParamDouble(context, &param1, TRUE)) {  //参数转化为浮点数
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_SAS_VOC %.10lf\r\n", param1);
	scpi_id = SET_SAS_VOC;      //返回scpi查询结果
	scpi_parameter_count = 1;   //统计此条指令带多少个参数
	scpi_parameter[0] = param1; //存储带参数的指令的参数值		
	CHECK_Chanlst(context);     //判断通道
    return SCPI_RES_OK;
}

//设置CC(FIXed)模式下电流
static scpi_result_t set_CC_CURR(scpi_t * context) {
    double param1;

    if (!SCPI_ParamDouble(context, &param1, TRUE)) {  //参数转化为浮点数
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CC_CURR %.10lf\r\n", param1);
	scpi_id = SET_CC_CURR;      //返回scpi查询结果
	scpi_parameter_count = 1;   //统计此条指令带多少个参数
	scpi_parameter[0] = param1; //存储带参数的指令的参数值		
	CHECK_Chanlst(context);     //判断通道
    return SCPI_RES_OK;
}

//设置CC(FIXed)模式下电压
static scpi_result_t set_CC_VOLT(scpi_t * context) {
    double param1;

    if (!SCPI_ParamDouble(context, &param1, TRUE)) {  //参数转化为浮点数
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CC_VOLT %.10lf\r\n", param1);
	scpi_id = SET_CC_VOLT;      //返回scpi查询结果
	scpi_parameter_count = 1;   //统计此条指令带多少个参数
	scpi_parameter[0] = param1; //存储带参数的指令的参数值		
	CHECK_Chanlst(context); //判断通道
    return SCPI_RES_OK;
}

//设置模拟器编码
static scpi_result_t set_SER_NUM(scpi_t * context) {
    const char * value;
	char NUM[11];  //10位编码 加一位'\0'
    size_t value_len;

    if (!SCPI_ParamCharacters(context, &value, &value_len, TRUE)) {  //参数转化为字符
        return SCPI_RES_ERR;
    }
	strncpy(NUM,value,value_len);
	NUM[value_len] = '\0';
	printf("SET_SER_NUM = %s\r\n", NUM);
	scpi_id = SERIAL_NUMBER;    //返回scpi查询结果
	strcpy(serial_number,NUM);  //存储编码
	scpi_parameter_count = 0;   //统计此条指令带多少个参数
    return SCPI_RES_OK;
}

//设置显示电压校正系数
static scpi_result_t set_CALI_VOLT(scpi_t * context) {
    double a,b,c,d,e;  //5个参数
	
    if (!SCPI_ParamDouble(context, &a, TRUE)) {  //参数转化为浮点数
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_VOLT a = %.10lf\r\n", a);
	
    if (!SCPI_ParamDouble(context, &b, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_VOLT b = %.10lf\r\n", b);
	
    if (!SCPI_ParamDouble(context, &c, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_VOLT c = %.10lf\r\n", c);
	
    if (!SCPI_ParamDouble(context, &d, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_VOLT d = %.10lf\r\n", d);
	
    if (!SCPI_ParamDouble(context, &e, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_VOLT e = %.10lf\r\n", e);
	
	scpi_id = CALIBRATION_VOLTAGE;      //返回scpi查询结果
	scpi_parameter_count = 5;   //统计此条指令带多少个参数
	scpi_parameter[0] = a; //存储带参数的指令的参数值
	scpi_parameter[1] = b; //存储带参数的指令的参数值
	scpi_parameter[2] = c; //存储带参数的指令的参数值
	scpi_parameter[3] = d; //存储带参数的指令的参数值
	scpi_parameter[4] = e; //存储带参数的指令的参数值
    return SCPI_RES_OK;
}

//设置显示电流分段校正系数
static scpi_result_t set_CALI_CURR(scpi_t * context) {
    double K1,K2,K3,K4;  //8个参数
	double B1,B2,B3,B4;
	
    if (!SCPI_ParamDouble(context, &K1, TRUE)) {  //参数转化为浮点数
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_CURR K1 = %.10lf\r\n", K1);
	
    if (!SCPI_ParamDouble(context, &B1, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_CURR B1 = %.10lf\r\n", B1);
	
    if (!SCPI_ParamDouble(context, &K2, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_CURR K2 = %.10lf\r\n", K2);
	
    if (!SCPI_ParamDouble(context, &B2, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_CURR B2 = %.10lf\r\n", B2);
	
    if (!SCPI_ParamDouble(context, &K3, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_CURR K3 = %.10lf\r\n", K3);
	
    if (!SCPI_ParamDouble(context, &B3, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_CURR B3 = %.10lf\r\n", B3);
	
    if (!SCPI_ParamDouble(context, &K4, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_CURR K4 = %.10lf\r\n", K4);
	
    if (!SCPI_ParamDouble(context, &B4, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_CALI_CURR B4 = %.10lf\r\n", B4);

	scpi_id = CALIBRATION_CURRENT;      //返回scpi查询结果
	scpi_parameter_count = 8;   //统计此条指令带多少个参数
	scpi_parameter[0] = K1; //存储带参数的指令的参数值
	scpi_parameter[1] = B1; //存储带参数的指令的参数值
	scpi_parameter[2] = K2; //存储带参数的指令的参数值
	scpi_parameter[3] = B2; //存储带参数的指令的参数值
	scpi_parameter[4] = K3; //存储带参数的指令的参数值
	scpi_parameter[5] = B3; //存储带参数的指令的参数值
	scpi_parameter[6] = K4; //存储带参数的指令的参数值
	scpi_parameter[7] = B4; //存储带参数的指令的参数值
    return SCPI_RES_OK;
}

//设置曲线ISC分段校正系数
static scpi_result_t set_ISC(scpi_t * context) {
    double K1,K2,K3,K4;  //8个参数
	double B1,B2,B3,B4;
    if (!SCPI_ParamDouble(context, &K1, TRUE)) {  //参数转化为浮点数
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_ISC K1 = %.10lf\r\n", K1);
	
    if (!SCPI_ParamDouble(context, &B1, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_ISC B1 = %.10lf\r\n", B1);
	
    if (!SCPI_ParamDouble(context, &K2, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_ISC K2 = %.10lf\r\n", K2);
	
    if (!SCPI_ParamDouble(context, &B2, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_ISC B2 = %.10lf\r\n", B2);
	
    if (!SCPI_ParamDouble(context, &K3, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_ISC K3 = %.10lf\r\n", K3);
	
    if (!SCPI_ParamDouble(context, &B3, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_ISC B3 = %.10lf\r\n", B3);
	
    if (!SCPI_ParamDouble(context, &K4, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_ISC K4 = %.10lf\r\n", K4);
	
    if (!SCPI_ParamDouble(context, &B4, TRUE)) {
        return SCPI_RES_ERR;
    }
    fprintf(stderr, "SET_ISC B4 = %.10lf\r\n", B4);

	scpi_id = CALIBRATION_ISC;      //返回scpi查询结果
	scpi_parameter_count = 8;   //统计此条指令带多少个参数
	scpi_parameter[0] = K1; //存储带参数的指令的参数值
	scpi_parameter[1] = B1; //存储带参数的指令的参数值
	scpi_parameter[2] = K2; //存储带参数的指令的参数值
	scpi_parameter[3] = B2; //存储带参数的指令的参数值
	scpi_parameter[4] = K3; //存储带参数的指令的参数值
	scpi_parameter[5] = B3; //存储带参数的指令的参数值
	scpi_parameter[6] = K4; //存储带参数的指令的参数值
	scpi_parameter[7] = B4; //存储带参数的指令的参数值
    return SCPI_RES_OK;
}

//设置任意曲线电流值 不做参数存储
static scpi_result_t set_TABL_CURR(scpi_t * context) {
    fprintf(stderr, "SET_TABL_CURR\r\n");
	scpi_id = SET_TABL_CURR;  //返回scpi查询结果
	scpi_parameter_count = 0; //统计此条指令带多少个参数
    return SCPI_RES_OK;
}

//设置任意曲线电压值 不做参数存储
static scpi_result_t set_TABL_VOLT(scpi_t * context) {
    fprintf(stderr, "SET_TABL_VOLT\r\n");
	scpi_id = SET_TABL_VOLT;  //返回scpi查询结果
	scpi_parameter_count = 0; //统计此条指令带多少个参数
    return SCPI_RES_OK;
}

//保存任意曲线电流数据1
static scpi_result_t set_SAVE_CURR1(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR1\r\n");
	scpi_id = SET_SAVE_CURR1;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据2
static scpi_result_t set_SAVE_CURR2(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR2\r\n");
	scpi_id = SET_SAVE_CURR2;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据3
static scpi_result_t set_SAVE_CURR3(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR3\r\n");
	scpi_id = SET_SAVE_CURR3;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据4
static scpi_result_t set_SAVE_CURR4(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR4\r\n");
	scpi_id = SET_SAVE_CURR4;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据5
static scpi_result_t set_SAVE_CURR5(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR5\r\n");
	scpi_id = SET_SAVE_CURR5;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据6
static scpi_result_t set_SAVE_CURR6(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR6\r\n");
	scpi_id = SET_SAVE_CURR6;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据7
static scpi_result_t set_SAVE_CURR7(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR7\r\n");
	scpi_id = SET_SAVE_CURR7;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据8
static scpi_result_t set_SAVE_CURR8(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR8\r\n");
	scpi_id = SET_SAVE_CURR8;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据9
static scpi_result_t set_SAVE_CURR9(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR9\r\n");
	scpi_id = SET_SAVE_CURR9;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据10
static scpi_result_t set_SAVE_CURR10(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR10\r\n");
	scpi_id = SET_SAVE_CURR10;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据11
static scpi_result_t set_SAVE_CURR11(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR11\r\n");
	scpi_id = SET_SAVE_CURR11;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据12
static scpi_result_t set_SAVE_CURR12(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR12\r\n");
	scpi_id = SET_SAVE_CURR12;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据13
static scpi_result_t set_SAVE_CURR13(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR13\r\n");
	scpi_id = SET_SAVE_CURR13;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据14
static scpi_result_t set_SAVE_CURR14(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR14\r\n");
	scpi_id = SET_SAVE_CURR14;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电流数据15
static scpi_result_t set_SAVE_CURR15(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_CURR15\r\n");
	scpi_id = SET_SAVE_CURR15;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据1
static scpi_result_t set_SAVE_VOLT1(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT1\r\n");
	scpi_id = SET_SAVE_VOLT1;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据2
static scpi_result_t set_SAVE_VOLT2(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT2\r\n");
	scpi_id = SET_SAVE_VOLT2;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据3
static scpi_result_t set_SAVE_VOLT3(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT3\r\n");
	scpi_id = SET_SAVE_VOLT3;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据4
static scpi_result_t set_SAVE_VOLT4(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT4\r\n");
	scpi_id = SET_SAVE_VOLT4;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据5
static scpi_result_t set_SAVE_VOLT5(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT5\r\n");
	scpi_id = SET_SAVE_VOLT5;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据6
static scpi_result_t set_SAVE_VOLT6(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT6\r\n");
	scpi_id = SET_SAVE_VOLT6;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据7
static scpi_result_t set_SAVE_VOLT7(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT7\r\n");
	scpi_id = SET_SAVE_VOLT7;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据8
static scpi_result_t set_SAVE_VOLT8(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT8\r\n");
	scpi_id = SET_SAVE_VOLT8;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据9
static scpi_result_t set_SAVE_VOLT9(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT9\r\n");
	scpi_id = SET_SAVE_VOLT9;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据10
static scpi_result_t set_SAVE_VOLT10(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT10\r\n");
	scpi_id = SET_SAVE_VOLT10;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据11
static scpi_result_t set_SAVE_VOLT11(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT11\r\n");
	scpi_id = SET_SAVE_VOLT11;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据12
static scpi_result_t set_SAVE_VOLT12(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT12\r\n");
	scpi_id = SET_SAVE_VOLT12;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据13
static scpi_result_t set_SAVE_VOLT13(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT13\r\n");
	scpi_id = SET_SAVE_VOLT13;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据14
static scpi_result_t set_SAVE_VOLT14(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT14\r\n");
	scpi_id = SET_SAVE_VOLT14;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//保存任意曲线电压数据15
static scpi_result_t set_SAVE_VOLT15(scpi_t * context) {
    fprintf(stderr, "SET_SAVE_VOLT15\r\n");
	scpi_id = SET_SAVE_VOLT15;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//上位机选择曲线号开机
static scpi_result_t set_SEL_NUM(scpi_t * context) {
    const char * value;
	size_t value_len;
	char CURVER[10];
	char CURVER1[10] = "CURVE1"; char CURVer1[10] = "CURve1";
	char CURVER2[10] = "CURVE2"; char CURVer2[10] = "CURve2";
	char CURVER3[10] = "CURVE3"; char CURVer3[10] = "CURve3";
	char CURVER4[10] = "CURVE4"; char CURVer4[10] = "CURve4";
	char CURVER5[10] = "CURVE5"; char CURVer5[10] = "CURve5";
	char CURVER6[10] = "CURVE6"; char CURVer6[10] = "CURve6";
	char CURVER7[10] = "CURVE7"; char CURVer7[10] = "CURve7";
	char CURVER8[10] = "CURVE8"; char CURVer8[10] = "CURve8";
	char CURVER9[10] = "CURVE9"; char CURVer9[10] = "CURve9";
	char CURVER10[10] = "CURVE10"; char CURVer10[10] = "CURve10";
	char CURVER11[10] = "CURVE11"; char CURVer11[10] = "CURve11";
	char CURVER12[10] = "CURVE12"; char CURVer12[10] = "CURve12";
	char CURVER13[10] = "CURVE13"; char CURVer13[10] = "CURve13";
	char CURVER14[10] = "CURVE14"; char CURVer14[10] = "CURve14";
	char CURVER15[10] = "CURVE15"; char CURVer15[10] = "CURve15";

    if (!SCPI_ParamCharacters(context, &value, &value_len, TRUE)) {  //参数转化为字符
        return SCPI_RES_ERR;
    }
	
	scpi_id = SET_SEL_NUM;    //返回scpi查询结果
	scpi_parameter_count = 1; //统计此条指令带多少个参数
	
	strncpy(CURVER,value,value_len);  //提取字符
	CURVER[value_len] = '\0';
	printf("CURVE = %s\r\n",CURVER);
	
	//根据字符存储参数
	if(strcmp(CURVER1,CURVER) == 0 || strcmp(CURVer1,CURVER) == 0)
	{
		scpi_parameter[0] = 1;
	}
	else if(strcmp(CURVER2,CURVER) == 0 || strcmp(CURVer2,CURVER) == 0)
	{
		scpi_parameter[0] = 2;
	}
	else if(strcmp(CURVER3,CURVER) == 0 || strcmp(CURVer3,CURVER) == 0)
	{
		scpi_parameter[0] = 3;
	}
	else if(strcmp(CURVER4,CURVER) == 0 || strcmp(CURVer4,CURVER) == 0)
	{
		scpi_parameter[0] = 4;
	}
	else if(strcmp(CURVER5,CURVER) == 0 || strcmp(CURVer5,CURVER) == 0)
	{
		scpi_parameter[0] = 5;
	}
	else if(strcmp(CURVER6,CURVER) == 0 || strcmp(CURVer6,CURVER) == 0)
	{
		scpi_parameter[0] = 6;
	}
	else if(strcmp(CURVER7,CURVER) == 0 || strcmp(CURVer7,CURVER) == 0)
	{
		scpi_parameter[0] = 7;
	}
	else if(strcmp(CURVER8,CURVER) == 0 || strcmp(CURVer8,CURVER) == 0)
	{
		scpi_parameter[0] = 8;
	}
	else if(strcmp(CURVER9,CURVER) == 0 || strcmp(CURVer9,CURVER) == 0)
	{
		scpi_parameter[0] = 9;
	}
	else if(strcmp(CURVER10,CURVER) == 0 || strcmp(CURVer10,CURVER) == 0)
	{
		scpi_parameter[0] = 10;
	}
	else if(strcmp(CURVER11,CURVER) == 0 || strcmp(CURVer11,CURVER) == 0)
	{
		scpi_parameter[0] = 11;
	}
	else if(strcmp(CURVER12,CURVER) == 0 || strcmp(CURVer12,CURVER) == 0)
	{
		scpi_parameter[0] = 12;
	}
	else if(strcmp(CURVER13,CURVER) == 0 || strcmp(CURVer13,CURVER) == 0)
	{
		scpi_parameter[0] = 13;
	}
	else if(strcmp(CURVER14,CURVER) == 0 || strcmp(CURVer14,CURVER) == 0)
	{
		scpi_parameter[0] = 14;
	}
	else if(strcmp(CURVER15,CURVER) == 0 || strcmp(CURVer15,CURVER) == 0)
	{
		scpi_parameter[0] = 15;
	}
	else
	{
		scpi_parameter[0] = 0;
	}
	printf("SET_SEL_NUM = %f\r\n", scpi_parameter[0]); 
	
    return SCPI_RES_OK;
}

/******************************************查询指令调用函数******************************************/
//查询ID
static scpi_result_t check_ID(scpi_t * context) {
    fprintf(stderr, "CHECK_ID\r\n");
	scpi_id = CHECK_ID;  //返回scpi查询结果
    return SCPI_RES_OK;
}

//查询开关机状态
static scpi_result_t check_ONOFF(scpi_t * context) {
    fprintf(stderr, "CHECK_ONOFF\r\n");
	scpi_id = OUTPUT_STATE;  //返回scpi查询结果
	CHECK_Chanlst(context);  //判断通道
    return SCPI_RES_OK;
}

//查询模式设置
static scpi_result_t check_MODE(scpi_t * context) {
    fprintf(stderr, "CHECK_MODE\r\n");
	scpi_id = MODE_STATE;    //返回scpi查询结果
	CHECK_Chanlst(context);  //判断通道
    return SCPI_RES_OK;
}

//查询曲线峰值功率点处电流 IMP
static scpi_result_t check_SAS_IMP(scpi_t * context) {
    fprintf(stderr, "CHECK_SAS_IMP\r\n");
	scpi_id = CHECK_SAS_IMP; //返回scpi查询结果
	CHECK_Chanlst(context);  //判断通道
    return SCPI_RES_OK;
}

//查询短路电流 ISC
static scpi_result_t check_SAS_ISC(scpi_t * context) {
    fprintf(stderr, "CHECK_SAS_ISC\r\n");
	scpi_id = CHECK_SAS_ISC; //返回scpi查询结果
	CHECK_Chanlst(context);  //判断通道
    return SCPI_RES_OK;
}

//查询曲线峰值功率点处电压 VMP
static scpi_result_t check_SAS_VMP(scpi_t * context) {
    fprintf(stderr, "CHECK_SAS_VMP\r\n");
	scpi_id = CHECK_SAS_VMP; //返回scpi查询结果
	CHECK_Chanlst(context);  //判断通道
    return SCPI_RES_OK;
}

//设置开路电压 VOC
static scpi_result_t check_SAS_VOC(scpi_t * context) {
    fprintf(stderr, "CHECK_SAS_VOC\r\n");
	scpi_id = CHECK_SAS_VOC; //返回scpi查询结果
	CHECK_Chanlst(context);  //判断通道
    return SCPI_RES_OK;
}

//查询CC(FIXed)模式下电流
static scpi_result_t check_CC_CURR(scpi_t * context) {
    fprintf(stderr, "CHECK_CC_CURR\r\n");
	scpi_id = CHECK_CC_CURR; //返回scpi查询结果
	CHECK_Chanlst(context); //判断通道
    return SCPI_RES_OK;
}

//查询CC(FIXed)模式下电压
static scpi_result_t check_CC_VOLT(scpi_t * context) {
    fprintf(stderr, "CHECK_CC_VOLT\r\n");
	scpi_id = CHECK_CC_VOLT; //返回scpi查询结果
	CHECK_Chanlst(context); //判断通道
    return SCPI_RES_OK;
}

//查询实时电流
static scpi_result_t check_SCALAR_CURR(scpi_t * context) {
    fprintf(stderr, "CHECK_SCALAR_CURR\r\n");
	scpi_id = CHECK_CURR;   //返回scpi查询结果
	CHECK_Chanlst(context); //判断通道
    return SCPI_RES_OK;
}

//查询实时电压
static scpi_result_t check_SCALAR_VOLT(scpi_t * context) {
    fprintf(stderr, "CHECK_SCALAR_VOLT\r\n");
	scpi_id = CHECK_VOLT;   //返回scpi查询结果
	CHECK_Chanlst(context); //判断通道
    return SCPI_RES_OK;
}

//查询系统错误
static scpi_result_t check_SYS_ERR(scpi_t * context) {
    fprintf(stderr, "CHECK_SYS_ERR\r\n");
	scpi_id = CHECK_SYSTEM_ERROR; //返回scpi查询结果
    return SCPI_RES_OK;
}

//查询机器错误
static scpi_result_t check_MAC_ERR(scpi_t * context) {
    fprintf(stderr, "CHECK_MAC_ERR\r\n");
	scpi_id = CHECK_MACHINE_ERROR; //返回scpi查询结果
    return SCPI_RES_OK;
}

//命令列表
const scpi_command_t scpi_commands[] = {
	//设置指令
	{.pattern = "OUTPut[:STATe]", .callback = set_ONOFF,},        //设置关开机
	{.pattern = "[SOURce]:CURRent:MODE", .callback = set_MODE,},  //设置模式
	{.pattern = "[SOURce]:CURRent:SAS:IMP", .callback = set_SAS_IMP,},  //设置曲线峰值功率点处电流 IMP
	{.pattern = "[SOURce]:CURRent:SAS:ISC", .callback = set_SAS_ISC,},  //设置短路电流 ISC
	{.pattern = "[SOURce]:VOLTage:SAS:VMP", .callback = set_SAS_VMP,},  //设置曲线峰值功率点处电压 VMP
	{.pattern = "[SOURce]:VOLTage:SAS:VOC", .callback = set_SAS_VOC,},  //设置开路电压 VOC
	{.pattern = "[SOURce]:CURRent[:LEVel][:IMMediate][:AMPLitude]", .callback = set_CC_CURR,},  //设置CC(FIXed)模式下电流
	{.pattern = "[SOURce]:VOLTage[:LEVel][:IMMediate][:AMPLitude]", .callback = set_CC_VOLT,},  //设置CC(FIXed)模式下电压
	{.pattern = "SERIAL:NUMBER", .callback = set_SER_NUM,},          //设置模拟器编码
	{.pattern = "CALIBRATION:VOLTAGE", .callback = set_CALI_VOLT,},  //设置显示电压校正系数
	{.pattern = "CALIBRATION:CURRENT", .callback = set_CALI_CURR,},  //设置显示电流分段校正系数
	{.pattern = "CALIBRATION:ISC", .callback = set_ISC,},            //设置曲线ISC分段校正系数
	{.pattern = "MEMory:TABLe:CURRent[:MAGNitude]", .callback = set_TABL_CURR,},  //设置任意曲线电流值
	{.pattern = "MEMory:TABLe:VOLTage[:MAGNitude]", .callback = set_TABL_VOLT,},  //设置任意曲线电压值
	{.pattern = "MEMory:TABLe:SELect", .callback = set_SEL_NUM,},  //上位机选择曲线号开机
	
	{.pattern = "MEMory:SAVE:CURRent:CURve1", .callback = set_SAVE_CURR1,},  //保存任意曲线电流数据1
	{.pattern = "MEMory:SAVE:CURRent:CURve2", .callback = set_SAVE_CURR2,},  //保存任意曲线电流数据2
	{.pattern = "MEMory:SAVE:CURRent:CURve3", .callback = set_SAVE_CURR3,},  //保存任意曲线电流数据3
	{.pattern = "MEMory:SAVE:CURRent:CURve4", .callback = set_SAVE_CURR4,},  //保存任意曲线电流数据4
	{.pattern = "MEMory:SAVE:CURRent:CURve5", .callback = set_SAVE_CURR5,},  //保存任意曲线电流数据5
	{.pattern = "MEMory:SAVE:CURRent:CURve6", .callback = set_SAVE_CURR6,},  //保存任意曲线电流数据6
	{.pattern = "MEMory:SAVE:CURRent:CURve7", .callback = set_SAVE_CURR7,},  //保存任意曲线电流数据7
	{.pattern = "MEMory:SAVE:CURRent:CURve8", .callback = set_SAVE_CURR8,},  //保存任意曲线电流数据8
	{.pattern = "MEMory:SAVE:CURRent:CURve9", .callback = set_SAVE_CURR9,},  //保存任意曲线电流数据9
	{.pattern = "MEMory:SAVE:CURRent:CURve10", .callback = set_SAVE_CURR10,},  //保存任意曲线电流数据10
	{.pattern = "MEMory:SAVE:CURRent:CURve11", .callback = set_SAVE_CURR11,},  //保存任意曲线电流数据11
	{.pattern = "MEMory:SAVE:CURRent:CURve12", .callback = set_SAVE_CURR12,},  //保存任意曲线电流数据12
	{.pattern = "MEMory:SAVE:CURRent:CURve13", .callback = set_SAVE_CURR13,},  //保存任意曲线电流数据13
	{.pattern = "MEMory:SAVE:CURRent:CURve14", .callback = set_SAVE_CURR14,},  //保存任意曲线电流数据14
	{.pattern = "MEMory:SAVE:CURRent:CURve15", .callback = set_SAVE_CURR15,},  //保存任意曲线电流数据15

	{.pattern = "MEMory:SAVE:VOLTage:CURve1", .callback = set_SAVE_VOLT1,},  //保存任意曲线电压数据1
	{.pattern = "MEMory:SAVE:VOLTage:CURve2", .callback = set_SAVE_VOLT2,},  //保存任意曲线电压数据2
	{.pattern = "MEMory:SAVE:VOLTage:CURve3", .callback = set_SAVE_VOLT3,},  //保存任意曲线电压数据3
	{.pattern = "MEMory:SAVE:VOLTage:CURve4", .callback = set_SAVE_VOLT4,},  //保存任意曲线电压数据4
	{.pattern = "MEMory:SAVE:VOLTage:CURve5", .callback = set_SAVE_VOLT5,},  //保存任意曲线电压数据5
	{.pattern = "MEMory:SAVE:VOLTage:CURve6", .callback = set_SAVE_VOLT6,},  //保存任意曲线电压数据6
	{.pattern = "MEMory:SAVE:VOLTage:CURve7", .callback = set_SAVE_VOLT7,},  //保存任意曲线电压数据7
	{.pattern = "MEMory:SAVE:VOLTage:CURve8", .callback = set_SAVE_VOLT8,},  //保存任意曲线电压数据8
	{.pattern = "MEMory:SAVE:VOLTage:CURve9", .callback = set_SAVE_VOLT9,},  //保存任意曲线电压数据9
	{.pattern = "MEMory:SAVE:VOLTage:CURve10", .callback = set_SAVE_VOLT10,},  //保存任意曲线电压数据10
	{.pattern = "MEMory:SAVE:VOLTage:CURve11", .callback = set_SAVE_VOLT11,},  //保存任意曲线电压数据11
	{.pattern = "MEMory:SAVE:VOLTage:CURve12", .callback = set_SAVE_VOLT12,},  //保存任意曲线电压数据12
	{.pattern = "MEMory:SAVE:VOLTage:CURve13", .callback = set_SAVE_VOLT13,},  //保存任意曲线电压数据13
	{.pattern = "MEMory:SAVE:VOLTage:CURve14", .callback = set_SAVE_VOLT14,},  //保存任意曲线电压数据14
	{.pattern = "MEMory:SAVE:VOLTage:CURve15", .callback = set_SAVE_VOLT15,},  //保存任意曲线电压数据15
	
	//查询指令
	{.pattern = "*IDN?", .callback = check_ID,},  //查询ID
	{.pattern = "OUTPut[:STATe]?", .callback = check_ONOFF,},        //查询开关机状态
	{.pattern = "[SOURce]:CURRent:MODE?", .callback = check_MODE,},  //查询模式设置
	{.pattern = "[SOURce]:CURRent:SAS:IMP?", .callback = check_SAS_IMP,},  //查询曲线峰值功率点处电流 IMP
	{.pattern = "[SOURce]:CURRent:SAS:ISC?", .callback = check_SAS_ISC,},  //查询短路电流 ISC
	{.pattern = "[SOURce]:VOLTage:SAS:VMP?", .callback = check_SAS_VMP,},  //查询曲线峰值功率点处电压 VMP
	{.pattern = "[SOURce]:VOLTage:SAS:VOC?", .callback = check_SAS_VOC,},  //查询开路电压 VOC
	{.pattern = "[SOURce]:CURRent[:LEVel][:IMMediate][:AMPLitude]?", .callback = check_CC_CURR,},  //查询CC(FIXed)模式下电流
	{.pattern = "[SOURce]:VOLTage[:LEVel][:IMMediate][:AMPLitude]?", .callback = check_CC_VOLT,},  //查询CC(FIXed)模式下电压
	{.pattern = "MEASure[:SCALar]:CURRent[:DC]?", .callback = check_SCALAR_CURR,},  //查询实时电流
	{.pattern = "MEASure[:SCALar]:VOLTage[:DC]?", .callback = check_SCALAR_VOLT,},  //查询实时电压
	{.pattern = "SYSTem:ERRor?", .callback = check_SYS_ERR,},  //查询系统错误
	{.pattern = "*FAULT?", .callback = check_MAC_ERR,},        //查询机器错误
	
    SCPI_CMD_LIST_END
};

size_t SCPI_Write(scpi_t * context, const char * data, size_t len) {
    (void) context;
    return fwrite(data, 1, len, stdout);
}

scpi_result_t SCPI_Flush(scpi_t * context) {
    (void) context;

    return SCPI_RES_OK;
}

int SCPI_Error(scpi_t * context, int_fast16_t err) {
    (void) context;

    //fprintf(stderr, "**ERROR: %d, \"%s\"\r\n", (int16_t) err, SCPI_ErrorTranslate(err)); //打印出错信息
    return 0;
}

scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    (void) context;

    if (SCPI_CTRL_SRQ == ctrl) {
        fprintf(stderr, "**SRQ: 0x%X (%d)\r\n", val, val);
    } else {
        fprintf(stderr, "**CTRL %02x: 0x%X (%d)\r\n", ctrl, val, val);
    }
    return SCPI_RES_OK;
}

scpi_result_t SCPI_Reset(scpi_t * context) {
    (void) context;

    fprintf(stderr, "**Reset\r\n");
    return SCPI_RES_OK;
}

scpi_result_t SCPI_SystemCommTcpipControlQ(scpi_t * context) {
    (void) context;

    return SCPI_RES_ERR;
}

scpi_interface_t scpi_interface = {
    .error = SCPI_Error,
    .write = SCPI_Write,
    .control = SCPI_Control,
    .flush = SCPI_Flush,
    .reset = SCPI_Reset,
};

char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

scpi_t scpi_context;
