/****************************************************************************
 *
 *  NGPIB_IO.C source file
 *
 *  Description:
 *  This source file contains all TNT register level code necessary to
 *  program the TNT as a Talker/Listener. It contains source for all
 *  GPIB status, initilization, addressing and I/O functions.
 *
 *  All functions are preceded with a header that describes the function,
 *  lists equivalent functions and related functions, shows examples of
 *  usage, and a note (or warning) that defines things to watch out for
 *  and whether or not the routine is a "user/system/developer" function.
 *  The "user" functions are the high level functions that are used to
 *  program the TNT4882 and are described in the ESP manual. A "system"
 *  function is a low level routine that is called by a "user/system/developer"
 *  function. A "developer" function is a routine that SHOULD be modified
 *  by the developer to meet 488.1 and/or 488.2 specifications for
 *  the device.
 *
 *  The index listed below has the following format:
 *
 *  Function_Group
 *   function_type Function() macro/also_known_as - Re#defines_of_Function()
 *
 *
 *  Functions listed as "aka" or "macro" are defined in the "gpib_io.h"
 *  header file.
 *
 *  Function Index:
 *  ===============
 *  Status
 *   u Update_4882_Status()        aka - Set_4882_Status,Clear_4882_Status
 *   u Update_INTERFACE_STATUS()
 *   u Wait_For_Interface()
 *   s Generate_Error()
 *   u Read_4882_Status()          macro
 *
 *  Initilization
 *   u Initialize_Interface()
 *   u High_Speed_Select()
 *   u Set_Timeout()
 *
 *  Addressing
 *   u Change_Primary_Address()
 *   u Change_Secondary_Address()
 *   u Change_Multiple_Addresses()
 *   u Set_Address_Mode()
 *   s Validate_Primary_Address()
 *   s Validate_Secondary_Address()
 *
 *  I/O
 *   u User_GPIB_IO()              aka - Send,Receive
 *   u IO_Control()                aka - Interface_On,Interface_Off
 *   s Setup_TNT_IO()
 *   s GPIB_DMA_IO()
 *   s Virtual_To_Physical()
 *   s GPIB_PROG_IO()
 *   s DONE_Handler()
 *   s Get_DATA_COUNT()            macro
 *   u Read_GPIB_Lines()           macro
 *
 *  Miscelaneous
 *   s CPT_Handler()
 *   s Validate_CF_Command()
 *
 *
 *  Compiler Options:
 *  =================
 *  Microsoft_C /AH /Gs /Ot
 *  Borland_C   -mh -O2
 *
 *  Date Last Modified:
 *    7/13/93
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "IO_PARAM.h"
#include "gpmc.h"
#include "funtion.h"
#include "NGPIB_IO.h"
//#include <dos.h>
//#include <conio.h>
#include <sys/time.h>  //使用定时器

/************* Globals ***************/
/* DMA page register locations    */
int page_reg[] = { 0x87, 0x83, 0x81, 0x82, 0x8f, 0x8b, 0x89, 0x8a };

unsigned short MR_4882_status[5]; /* 4882 status memory registers   */

unsigned long int DATA_COUNT; /* Transfer count                 */

int INTERFACE_ERROR; /* Error Code                     */

int INTERFACE_STATUS; /* Interface Status               */

int Primary_Address = PRIMARY_ADDRESS; /* Primary Address Value          */
int Secondary_Address = SECONDARY_ADDRESS; /* Secondary Address Value        */
int Address_Mode = ADDRESS_MODE;
int Multiple_Address_Table[32]; /* Secondary Address Lookup Table */
int Multiple_Address_List[32] = MULTIPLE_ADDRESSES;
int Number_Of_Addresses = NUMBER_OF_ADDRESSES;

int MULTI_ADDRESS = NONE;

int CURRENT_ADDRESS;

unsigned long int Requested_Count = 0; /* Requested transfer count       */
/* Timeout time                   */
struct timeout_type Timeout = { TIMEOUT_FACTOR_INDEX, USE_BYTE_TIMEOUTS };

int HCAS = FALSE;

/**********************************  STATUS  ********************************
 *
 *  Update_4882_Status(): Can be used to set and clear Memory Registers (MR)
 *                        that reflect the structure of the 488.2 Status
 *                        reporting model.
 *
 *                        defined registers: STB,SRE,ESE,ESR,IST
 *
 *                        Can be expanded to include optional 488.2 status
 *                        registers.
 *
 *  Also defined as(aka): Set_4882_Status(),Clear_4882_Status()
 *
 *  Related Functions:    Read_4882_Status()
 *
 *  Valid Parameters:     status_register = STB(0),SRE(1),ESE(2),ESR(3),IST(4)
 *                        byte            = 0,1,2...0xff
 *                        operation       = SET(1),CLEAR(0)
 *
 *  Example(s):
 *    1) Set_4882_Status(SRE,0x02);     Enable setting of SRQ on ESB
 *       Set_4882_Status(STB,0x04);     Set MAV bit
 *       Set_4882_Status(ESR,0xff);     Enable setting of ESB on any event
 *          .........
 *       Set_4882_Status(ESE,0x20);     Set ESE bit (SRQ will be set)
 *       Clear_4882_Status(ESR,0x02);   Clear Bit 1 in ESR
 *
 *    2) Set_4882_Status(IST,0x01);     Sets the IST bit
 *       Clear_4882_Status(IST,0x01);   Clear IST bit
 *       Clear_4882_Status(xxx,0xff);   Clears all bits of xxx memory register
 *
 *    3) Set_4882_Status(ESR,0x02);     Set 0x02 in ESR
 *       Set_4882_Status(ESR,0x90);     Set 0x90 in ESR
 *       value = Read_4882_Status(ESR); Read ESR ... ESR == 0x92
 *
 *  Note:                 This is a user function.
 *                        When setting a new register value it is OR'd with
 *                        the old register value. When clearing a register
 *                        bit pattern it is ~and with the old register value.
 *
 ****************************************************************************/

void Update_4882_Status(int status_register, unsigned short byte, int operation) {
	int set_srq;

	MR_4882_status[STB] = TNT_In(R_spsr); /* Get STB                        */

	switch (operation) { /* Set or Clear ?                 */
	case SET:
		/* Cannot set SRQ directly        */
		if ((status_register == STB) || (status_register == SRE))
			byte &= ~0x40;

		MR_4882_status[status_register] |= byte;/* OR in new register value    */

		if (status_register == IST)
			TNT_Out(R_auxmr, F_ist1); /* if IST set IST bit             */

		break;

	case CLEAR:
		MR_4882_status[status_register] &= ~byte;/* ~AND requested bits         */
		if (status_register == IST)
			TNT_Out(R_auxmr, F_ist0); /* if IST clear IST bit           */
		break;

	default:
		Generate_Error(EARG); /* Neither SET/CLEAR then EARG    */
		break;
	}
	/* If ESE&ESR set ESB bit         */
	MR_4882_status[STB] |=
			(MR_4882_status[ESE] & MR_4882_status[ESR]) ? 0x20 : 0;

	/* If STB&SRE set RQS bit         */
	set_srq = (MR_4882_status[STB] & MR_4882_status[SRE]) ? TRUE : FALSE;

	if (set_srq) /* If SRQ desired                 */
		TNT_Out(R_auxmr, F_reqt); /*   Set request true             */
	else
		/* Else                           */
		TNT_Out(R_auxmr, F_reqf); /*   Set request false            */

	TNT_Out(R_spmr, MR_4882_status[STB]); /* Set new serial poll byte       */
}

/****************************************************************************
 *
 *  Update_INTERFACE_STATUS(): Used to update INTERFACE_STATUS word by reading
 *                             R_isr0,R_isr1,R_isr2,R_isr3,R_adsr,R_spmr and
 *                             updating the appropriate INTERFACE_STATUS bits.
 *
 *  INTERFACE_STATUS bits:
 *
 *    ERR  (1<<15)   Error                - Check INTERFACE_ERROR for error
 *    TIMO (1<<14)   Timeout              - If timeouts are used
 *    END  (1<<13)   End/EOI/EOS          - End of transmition received
 *    EOS  (1<<12)   End of String        - End of string received
 *    RQS  (1<<11)   Requesting Service   - TNT asserting SRQ line
 *    IFC  (1<<10)   Interface Clear      - Interface Clear Asserted
 *    SPOLL (1<<9)   Serial Poll Active   - Serial Poll Byte STB is requested
 *    UCMPL (1<<8)   User Complete        - User I/O function terminated
 *
 *    LOK   (1<<7)   Local Lockout        - Lockout front panel controls
 *    REM   (1<<6)   Remote Programming   - TNT in remote programming state
 *    ASYNC (1<<5)   Asyncronous I/O      - NOT USED IN NON-INT ESP
 *    DTAS  (1<<4)   Trigger Active State - Requested Device Trigger
 *    DCAS  (1<<3)   Clear Active State   - Requested Device Clear
 *    LACS  (1<<2)   Listener Active      - TNT listen addressed
 *    TACS  (1<<1)   Talker Active        - TNT talk addressed
 *    NACS  (1<<0)   Not Active State     - TNT not addressed
 *
 *  Related Functions:         Wait_For_Interface()
 *
 *  Valid Parameters:          NONE
 *
 *  Example(s):
 *    1) new_status=Update_INTERFACE_STATUS(); get new INTERFACE_STATUS
 *
 *  Note:                      This is a user function. INTERFACE_ERROR
 *                             means nothing unless ERR is set in
 *                             INTERFACE_STATUS.  Before calling a Send/Receive
 *                             function make sure ATN is not asserted.
 *                             If the HCE HS488command 0x1f is being sent
 *                             or if a multi-addressing option is being used,
 *                             the assertion of the ATN might mean that the
 *                             TNT is currently holding off the handshake
 *                             and is waiting on one of the Validate_X functions
 *                             to verify the heldoff command bytes.
 *                             Update_INTERFACE_STATUS() calls the verification
 *                             routines Validate_CF_Command(),Validate_Secondary
 *                             Address(), Validate_Primary_Address(). To detect
 *                             whether ATN is asserted use Read_GPIB_Lines().
 *
 ****************************************************************************/

int Update_INTERFACE_STATUS(void)
{
	int mr_isr0, mr_isr1, mr_isr2, mr_isr3, mr_spsr, mr_adsr;

	/* This is used to validate  这用于验证 */
	while ((Address_Mode == 0x02) && (TNT_In(R_isr1) & B_apt))
		Validate_Secondary_Address(); /*   multi-secondary addresses  多个二次地域  */
	while (TNT_In(R_isr1) & B_cpt) 	/* This is used to validate the   */
		CPT_Handler(); 				/*   HS488 CFE function and multi-primary addresses*/

	mr_isr1 = TNT_In(R_isr1); /* Read status register isr1      */
	mr_isr2 = TNT_In(R_isr2); /* Read status register isr2      */
	mr_isr0 = TNT_In(R_isr0); /* Read status register isr0      */
	mr_spsr = TNT_In(R_spsr); /* Read status register spsr      */
	mr_adsr = TNT_In(R_adsr); /* Read status register adsr      */
	//mr_isr3 = TNT_In(R_isr3);

	//printf("mr_isr0 = %x\n", mr_isr0);
	//printf("mr_isr1 = %x\n", mr_isr1);
	//printf("mr_isr2 = %x\n", mr_isr2);
	//printf("mr_isr3 = %x\n", mr_isr3);
	//printf("mr_spsr = %x\n", mr_spsr);
	//printf("mr_adsr = %x\n", mr_adsr);

	INTERFACE_STATUS &= (UCMPL | END | EOS | TIMO | ERR);/* Maintain I/O bits           */
	/* These are cleared at the       */
	/*   beginning of a new I/O call 这些在新的I / O调用开始时被清除  */

	/* Get new status                 */
	INTERFACE_STATUS |= ((mr_isr0 & B_to)?TIMO :0)|((mr_isr2 & B_rem)?REM :0)|(
			(mr_isr1 & B_end)?END :0)|((mr_isr0 & B_eos)?EOS :0)|(
			(mr_spsr & B_pend)?RQS :0)|((mr_isr0 & B_ifc)?IFC :0)|(
			(mr_isr0 & B_stbo)?SPOLL:0)|((mr_isr2 & B_lok)?LOK :0)|(
			(mr_isr1 & B_det)?DTAS :0)|((mr_isr1 & B_dec)?DCAS:0)|(
			(mr_adsr & B_ta)?TACS :0)|((mr_adsr & B_la)?LACS:0);

	INTERFACE_STATUS |= ((INTERFACE_STATUS & (LACS | TACS))? 0:NACS);

	return INTERFACE_STATUS; /* Return status                  */
}

/****************************************************************************
 *
 *  Wait_For_Interface():  Can be used to wait for a specified event
 *                         determined by "mask" and the INTERFACE_STATUS
 *                         status word.
 *
 *  Related Functions:     Update_INTERFACE_STATUS()
 *
 *  Valid Parameters:      mask = 0,1,2...0xffff (INTEFACE_STATUS bits)
 *
 *  Example(s):
 *    1) Wait_For_Interface(LACS);           Wait to be listen addressed
 *    2) Wait_For_Interface(LACS|REM|TACS);  Wait for any of LACS|TACS|REM
 *
 *  Note:                  This is a user function.
 *
 ****************************************************************************/

void Wait_For_Interface(int mask) { /* Get new status wait for mask   */
	while (!((Update_INTERFACE_STATUS()) & mask))
		;
}

/****************************************************************************
 *
 *  Generate_Error():  Used to Update INTERFACE_ERROR. INTERFACE_ERROR is
 *                     only valid when ERR is set in INTERFACE_STATUS.
 *
 *  INTERFACE_ERROR values:
 *
 *    xxxx  0  - No Error
 *    ENOL  1  - No Listeners I/O aborted because no listeners on bus.
 *    EARG  2  - Bad Argument in parameter list.
 *    EABO  3  - I/O Aborted due to timeout.
 *
 *  Related Functions: None
 *
 *  Valid Parameters:  code = 1,2..7
 *
 *  Note:              This is a system function.
 *
 ****************************************************************************/

void Generate_Error(int code) {
	INTERFACE_STATUS |= ERR; /* Set error bit & enter code     */
	INTERFACE_ERROR = code; /* Set error code                 */
}

/**************************** INITIALIZATION ********************************
 *
 *  Initialize_Interface(): Sets the TNT into a known initialized state,
 *                          clears global values (mask registers,status words),
 *                          and loads current addressing setup. The initial
 *                          setup is determined by parameters in the file
 *                          "io_param.h".
 *
 *                          Normally, this function is executed once at power
 *                          up, however it may be used to reinitialize the
 *                          interface during operation.
 *
 *  Related Functions:      NONE
 *
 *  Valid Parameters:       NONE
 *
 *  Example(s):
 *    1)  Initialize_Interface();
 *
 *  Note:                   This is a user function.
 *
 *                          If Initialize_Interface() is called later in the
 *                          operation of the device:
 *
 *                           1) The addressing modes and addresses will not
 *                                change. 
 *                           2) The timeout setings will not change.
 *
 ****************************************************************************/

void Initialize_Interface(void) {
	TNT_Out(R_intr, 0); /*   Disable AT-BOARD interrupts   */

	INTERFACE_STATUS = 0; /* Initialize Globals to zero     */
	INTERFACE_ERROR = 0; /*    "         "      "   "      */
	DATA_COUNT = 0; /*    "         "      "   "      */
	HCAS = FALSE;

	TNT_Out(R_cmdr, F_softreset); /* Reset FIFOS                    */

	TNT_Out(R_spmr, 0x80); /* This sequence of commands      */
	TNT_Out(R_auxmr, 0x80); /*   insures that the TNT         */
	TNT_Out(R_auxmr, 0x99); /*   will be in the normal 7210   */
	TNT_Out(R_keyrg, 0); /*   mode and not 9914            */

	/* Enable "One Chip Mode"         */
	High_Speed_Select(CABLE_LENGTH, DISABLE);/* Initialize TNT to one chip mode*/

	TNT_Out(R_auxmr, F_chrst); /* Reset TNT                      */

	Clear_4882_Status(STB, 0xff); /* Initialize Serial Poll Byte    */
	Clear_4882_Status(SRE, 0xff); /* Initialize SRE memory register */
	Clear_4882_Status(ESR, 0xff); /* Initialize ESR memory register */
	Clear_4882_Status(ESE, 0xff); /* Initialize ESE memory register */

	Set_Address_Mode(Address_Mode); /* Set TNT Addressing Mode        */

	switch (Address_Mode) {
	case (0x00): /* Set Primary Address            */
		Change_Primary_Address(Primary_Address);
		break;

	case (0x01): /* Set Primary & Secondary Address*/
		Change_Primary_Address(Primary_Address);
		Change_Secondary_Address(Secondary_Address);
		break;

	case (0x02): /* Set Primary & Secondary Addresses*/
		Change_Primary_Address(Primary_Address);
		Change_Multiple_Addresses(Multiple_Address_List, Number_Of_Addresses);
		break;

	case (0x03): /* Set Primary Addresses          */
		Change_Multiple_Addresses(Multiple_Address_List, Number_Of_Addresses);
		break;
	}
	/* Set DMA timer value to limit   */
#if(USE_DMA && USE_DEMAND_MODE_DMA)       /*   demand mode transfers.       */
	TNT_Out(R_timer,0x6a); /*   6a is twos complement 95     */
#endif                                    /*   0x95*100ns=15us max hold time*/

	/* Set auxri for static bits and  */
	/*   possibly ultra short t1 delay*/
	TNT_Out(R_auxmr, HR_auxri | B_sisb | ((USE_HIGH_SPEED_T1) ? B_ustd : 0));

	/* Set auxrb to hold off on unknown*/
#if(USE_HS488_HANDLER)                    /*   commands and high speed t1    */
	TNT_Out(R_auxmr, HR_auxrb | B_hldacpt | ((USE_HIGH_SPEED_T1) ? B_hst1 : 0));

#else                                     /* If not using HS488 set only hst1*/
	TNT_Out(R_auxmr,HR_auxrb|((USE_HIGH_SPEED_T1)? B_hst1 : 0));
#endif

	TNT_Out(R_auxmr, F_hldi); /* Issue hold off immediately     */
	TNT_Out(R_auxmr, F_pon); /* Clear Power On                 */

	TNT_Out(R_imr0, B_glint); /* Enable setting of tlcint       */
}

/****************************************************************************
 *
 *  Set_Address_Mode():  Can be used to switch the type of addressing
 *                       used by the TNT4882.  For modes 0,1,2,3, this
 *                       function turns off addressing until one or more
 *                       of the related functions are executed.
 *
 *  Related Functions:   Change_Primary_Address(), Change_Secondary_Address()
 *                       Change_Multiple_Addresses()
 *
 *  Valid Parameters:    mode = 0x00 - Single Primary Addressing
 *                        "   = 0x01 - Single Primary Single Secondary Ad.
 *                        "   = 0x02 - Single Primary Multiple Secondary Ad.
 *                        "   = 0x03 - Multiple Primary Addressing
 *                        "   = 0x04 - Talk Only Mode
 *                        "   = 0x05 - Listen Only Mode
 *                        "   = 0x06 - No Address
 *  Example(s):
 *    1) Set_Address_Mode(1);         Set to single primary single secondary
 *       Change_Primary_Address(4);   Set primary address
 *       Change_Secondary_Address(10);Set secondary address (address now
 *                                       enabled)
 *
 *    2) int new_add[5]={1,3,5,7,9};          Address list
 *       Set_Address_Mode(3);                 Set multi primary addressing
 *       Change_Multiple_Addresses(new_add,5);Set new addresses (addresses
 *                                              now enabled)
 *    3) Set_Address_Mode(4);  Set to talk only mode
 *       Set_Address_Mode(5);  Set to listen only mode
 *
 *    4) Set_Address_Mode(6);  Disable TNT from talking or listening
 *
 *  Note:                This is a user function.  Addressing is not enabled
 *                       in modes 0,1,2,3 until one or more of the related
 *                       addressing functions is executed. Address mode 7
 *                       is reserved by the IO_Control() function.
 *
 ****************************************************************************/

void Set_Address_Mode(int mode) {
	TNT_Out(R_admr, F_noaddress); /* Clear address mode             */
	TNT_Out(R_adr, B_dt | B_dl); /* Disable talk & listener        */
	TNT_Out(R_adr, B_ars | B_dt | B_dl); /*   capabilities                 */
	TNT_Out(R_auxmr, F_lut); /* Untalk TNT4882                 */
	TNT_Out(R_auxmr, F_lul); /* Unlisten TNT4882               */

	if (Address_Mode == 0x03) { /* If multi-primary address clear */
		TNT_Out(R_auxmr, HR_auxrf | 0); /*   NDAC holdoffs and CPT int.   */
	}

	switch (mode) {
	case (0x00): /* Set single primary address mode*/
		TNT_Out(R_admr, F_normalprimary);
		break;

	case (0x01): /* Set single primary single      */
		TNT_Out(R_admr, F_normalextended); /*   secondary address mode       */
		break;

	case (0x02): /* Set single primary multiple    */
		TNT_Out(R_admr, F_primarymultiextended);/*secondary address mode       */
		break;

	case (0x03): /* Set multi primary address mode */
		TNT_Out(R_admr, F_noaddress);
		break;

	case (0x04): /* Set talk only mode             */
		TNT_Out(R_admr, F_talkonly);
		break;

	case (0x05): /* Set listen only mode           */
		TNT_Out(R_admr, F_listenonly);
		break;
	}
	if (mode != 0x07)
		Address_Mode = mode; /* Copy of current mode           */
}

/****************************************************************************
 *
 *  CPT_Handler():     Reads the unknown command out of the command
 *                     pass through register and calls the proper
 *                     function to handle command.
 *                     Enabled by the USE_HS488_HANDLER option in
 *                     "io_param.h" and/or ADDRESS_MODE 3.
 *
 *  Related Functions: High_Speed_Select(),Validate_CF_Command(),
 *                     Validate_Primary_Address()
 *
 *  Valid Parameters:  NONE
 *
 *  Note:              This is a system function.
 *
 ****************************************************************************/

void CPT_Handler(void) {
	int mask;

	mask = 0x7f & TNT_In(R_cptr); /* Read R_cptr                    */

	if ((Address_Mode == 0x03) && (mask >= 0x20) && (mask <= 0x5f)) {
		Validate_Primary_Address();
	} else if ((mask == 0x1f) || HCAS) { /* If CFE has been received       */
		Validate_CF_Command(); /*   & mask in secondary range    */
	} else {
		TNT_Out(R_auxmr, F_nonvalid); /* Continue the handshake         */
	}
}

/****************************************************************************
 *
 *  Validate_CF_Command(): Call decodes the CF command and configures
 *                         TNT timing registers by calling the
 *                         High_Speed_Select() configuration function.
 *
 *  Related Functions:     High_Speed_Select()
 *
 *  Valid Parameters:      NONE
 *
 *  Note:                  This is a system function.
 *
 ****************************************************************************/

void Validate_CF_Command(void) {
	int mask;

	mask = 0x7f & TNT_In(R_cptr); /* Read R_cptr (bit 7 not valid)  */

	if ((HCAS) && (mask >= 0x60) && (mask <= 0x6f)) {
		HCAS = FALSE;
		High_Speed_Select(mask & 0x0f, ENABLE); /* Configure timing registers     */
	} else {
		HCAS = (mask == 0x1f) ? TRUE : FALSE; /* If byte=CFE function HCAS=TRUE */
		High_Speed_Select(mask & 0x0f, DISABLE); /* Configure timing registers     */
	}

	TNT_Out(R_auxmr, F_valid); /* Validate in order to continue  */
}

/****************************************************************************
 *
 *  High_Speed_Select(): Can be used to locally set the HS488 timing values.
 *                       The Validate_CF_Function() also calls this routine
 *                       when the high speed configuration function (0x1f
 *                       universal command byte) and cable length
 *                       (0x6X secondary command byte)is sent over the GPIB.
 *                       This function also places the TNT into "one chip"
 *                       mode.
 *
 *  Related Functions:   Validate_CF_Function()
 *
 *  Valid Parameters:    cable_length = 0,1,2...15 (meters of cable)
 *                       enable       = ENABLE(1), DISABLE(0)
 *
 *  Example(s)
 *    1) High_Speed_Select(3,ENABLE); 3m of cable turn on HS488 capabilities
 *
 *    2) High_Speed_Select(x,DISABLE); x dont care, turn off HS488
 *                                       use normal three wire gpib handshake
 *
 *  Note:                This is a user and a system function.
 *
 ****************************************************************************/

void High_Speed_Select(int cable_length, int enable) {
	/* deglitch table                 */
	int deglitch[16] = { 0, 0, 0, 0, B_dga, B_dga, B_dga, B_dga, B_dga | B_dgb,
			B_dga | B_dgb, B_dga | B_dgb, B_dga | B_dgb, B_dga | B_dgb, B_dga
					| B_dgb, B_dga | B_dgb, B_dga | B_dgb };

	/* t11 factors                    */
	int t11[16] = { NO_TSETUP, NO_TSETUP, NO_TSETUP, 0, 0, 2, 2, 2, 4, 4, 6, 6,
			6, 9, 9, 9 };

	/* t12 factors                    */
	int t12[16] = { 0, 0, 0, 2, 2, 4, 4, 4, 6, 6, 8, 8, 8, 11, 11, 11 };

	/* If cable length out of range   */
	if ((cable_length > 15) || (cable_length < 0)) {
		Generate_Error(EARG); /*   set EARG                     */
		cable_length = 15; /*   default to largest setting   */
	}

	TNT_Out(R_hssel, F_onechip); /* Set TNT to one chip WINK mode  */

	if (enable) { /* If "enable" HS488              */
		TNT_Out(R_sh_cnt, HR_t1 | 0); /*   disable programmable t1      */
		TNT_Out(R_sh_cnt, HR_t12 | t12[cable_length]);/* set t12 timing value      */
		TNT_Out(R_sh_cnt, HR_t17 | 18); /*      set t17 to 500ns          */

		if (NO_TSETUP == t11[cable_length]) { /* If no tsetup disable tsetup    */
			TNT_Out(R_hier, B_notsetup | deglitch[cable_length]);
		} else { /* Else set tsetup time           */
			TNT_Out(R_sh_cnt, HR_t11 | t11[cable_length]);
			TNT_Out(R_hier, deglitch[cable_length]);
		}

		TNT_Out(R_misc, B_hse); /* Enable HS mode                 */
	} else {
		TNT_Out(R_misc, 0); /* Disable HS mode                */
		TNT_Out(R_hier, deglitch[15]); /* Set deglitching circuits to    */
	} /*   slowest value                */
}

/****************************************************************************
 *
 *  Set_Timeout(): Can be used to set the a timeout value. The information
 *                 set by this function is used by Setup_TNT_IO() to enable
 *                 the TNT4882 timer.
 *	可用于设置超时值。 Setup_TNT_IO（）使用此功能设置的信息启用TNT4882定时器。
 *  Related Functions: NONE
 *
 *  Valid Parameters:
 *
 *  factor_index =  0  1   2    3    4  5  6   7   8    9    a  b c  d  e   f
 *      val(sec)- off 16u 32u 128u 256u 1m 4m 16m 33m 131m 262m 1 4 17 34 134
 *
 *  byte_timeout = ENABLE(1),DISABLE(0)
 *
 *  Examples:
 *    1) Set_Timeout(0,ENABLE);  Disable timeouts.
 *
 *    2) Set_Timeout(4,ENABLE);  Timeouts set to 256us, byte timeouts.
 *
 *    3) Set_Timeout(9,DISABLE); Timeouts set to 131ms, timeout time for
 *                               entire transfer.
 *
 *  Note:          This is a user function. Do not use with queues unless
 *                 a queue has been disabled and the Send/Recieve function
 *                 in lieu of the queue. If using timeouts it is best to use
 *                 byte_timeouts.
 *		这是一个用户功能。除非队列已被禁用且发送/接收功能代替队列，否则不要与队列一起使用。
 *		如果使用超时，最好使用byte_timeouts。
 ****************************************************************************/

void Set_Timeout(int factor_index, int byte_timeout) {
	Timeout.factor_index = factor_index; /* Set factor index               */
	Timeout.byte_timeout = byte_timeout; /* Timeout on each byte?  每个字节超时？   */
}

/****************************** ADDRESSING **********************************
 *
 *  Change_Primary_Address(): Used for changing primary address of TNT.
 *                            Sets global Primary_Address values.
 *
 *  Related Functions:        Change_Secondary_Address(),
 *                            Change_Multiple_Addresses(),
 *                            Set_Address_Mode()
 *
 *  Valid Parameters:         new_address = 0,1,2...30
 *
 *  Example:
 *    1)  Change_Primary_Address(0x10); Set primary address to 0x10
 *
 *  Note:                     This is a user function. This function
 *                            will return EARG if not in correct
 *                            Address_Mode.
 *
 ****************************************************************************/

void Change_Primary_Address(int new_address) { /* If not modes 0,1 or 2          */
	/*   do not set primary address   */
	if ((Address_Mode != 0x00) && (Address_Mode != 0x01)
			&& (Address_Mode != 0x02))
	{
		Generate_Error(EARG);
		return;
	}

	Primary_Address = new_address; /* Set Global Primary_Ad. value   */

	TNT_Out(R_adr, new_address); /* Load new address setting       */
	CURRENT_ADDRESS = (new_address << 8) | (CURRENT_ADDRESS & 0x00ff);
}

/****************************************************************************
 *
 *  Change_Secondary_Address(): Used for changing secondary address of TNT.
 *                              Sets global Secondary_Address values.
 *
 *  Related Functions:          Change_Primary_Address(),
 *                              Change_Multiple_Addresses(),
 *                              Set_Address_Mode()
 *
 *  Valid Parameters:           new_address = 0,1,2...30
 *
 *  Example:
 *    1)  Change_Secondary_Address(0x10); Set secondary address to 0x10
 *
 *  Note:                       This is a user function. This function
 *                              will return EARG if not in Address_Mode 1.
 *
 ****************************************************************************/

void Change_Secondary_Address(int new_address) {
	if (Address_Mode != 0x01) { /* If not in mode 1               */
		Generate_Error(EARG); /*   do not set secondary address */
		return;
	}

	Secondary_Address = new_address; /* Set Global Secondary_Ad. val.  */
	TNT_Out(R_adr, B_ars | new_address); /* Load new secondary address     */
	CURRENT_ADDRESS = new_address | (CURRENT_ADDRESS & 0xff00);
}

/****************************************************************************
 *
 *  Change_Multiple_Addresses(): Used for changing multiple primary/secondary
 *                               addresses of TNT.
 *
 *  Related Functions:           Change_Secondary_Address(),
 *                               Change_Primary_Address(),
 *                               Set_Address_Mode(),
 *                               Validate_Primary_Address(),
 *                               Validate_Secondary_Address()
 *
 *  Valid Parameters:            new_address_list = {0,1,2...30} (any combo)
 *                               num_of_addresses = 0,1,2...31
 *
 *  Example(s):
 *    1)  int new[5]={0x4,0x5,0x13,0x10};   New address list
 *        Set_Address_Mode(3);              Set multi primary addressing
 *        Change_Multiple_Addresses(new,4); Set primary addresses
 *
 *    2)  int new[5]={0x4,0x5,0x13,0x10};   New address list
 *        Set_Address_Mode(2);              Set single primary
 *                                            multi secondary addressing mode
 *        Change_Primary_Address(2);        Set primary address
 *        Change_Multiple_Addresses(new,4); Set secondary addresses
 *
 *  Note:                        This is a user function. This function
 *                               will return EARG if not in Address Mode 2
 *                               or Address_Mode 3.
 *
 ****************************************************************************/

void Change_Multiple_Addresses(int *new_address_list, int num_of_addresses) {
	int i; /* Count Integer                  */

	/* If not address mode 2 or 3     */
	/*   do not set multiple addresses*/
	if ((Address_Mode != 0x02) && (Address_Mode != 0x03)) {
		Generate_Error(EARG);
		return;
	}

	memset(Multiple_Address_Table, 0, 32); /* Clear lookup table             */

	for (i = 0; i < num_of_addresses; i++) { /* Build new lookup table         */
		Multiple_Address_Table[*(new_address_list + i)] = 1;
		/* Build new address list global  */
		Multiple_Address_List[i] = *(new_address_list + i);
	}

	Number_Of_Addresses = num_of_addresses; /* Set Num global                 */

	if (Address_Mode == 0x03) /* If mode 3 set CPT holdoff      */
		TNT_Out(R_auxmr, HR_auxrf | B_dhata | B_dhala);/* NDAC holdoff on primary ad.*/
}

/****************************************************************************
 *
 *  Validate_Primary_Address():   Used by CPT_Handler() in order
 *                                to validate the primary address in the
 *                                pass through register. Updates global
 *                                CURRENT_ADDRESS value.
 *
 *  Related Functions:            CPT_Handler(), Change_Multiple_Addresses()
 *
 *  Valid Parameters:             NONE
 *
 *  Note:                         This is a system function.
 *
 ****************************************************************************/

void Validate_Primary_Address(void) {
	int primary_address;

	primary_address = TNT_In(R_cptr); /* Get Primary Address from cpt   */
	/* Check to see if valid address  */
	if (Multiple_Address_Table[primary_address & 0x1f]) {

		TNT_Out(R_auxmr, F_valid); /* Valid: send valid aux. function*/
		CURRENT_ADDRESS = ((primary_address & 0x1f) << 8)
				| (CURRENT_ADDRESS & 0x00ff);

		if (primary_address & 0x20) {
			TNT_Out(R_auxmr, F_lut); /*   Untalk TNT                   */
			TNT_Out(R_admr, F_listenonly);
			TNT_Out(R_admr, F_noaddress);
		}

		if (primary_address & 0x40) {
			TNT_Out(R_auxmr, F_lul); /*   Unlisten TNT                 */
			TNT_Out(R_admr, F_talkonly);
			TNT_Out(R_admr, F_noaddress);
		}

	} else {
		TNT_Out(R_auxmr, F_nonvalid); /* Not valid:  nonvalid function  */
	}
}

/****************************************************************************
 *
 *  Validate_Secondary_Address(): Used by Update_INTERFACE_STATUS in order
 *                                to validate the current secondary address
 *                                in the pass through register. Returns
 *                                the Secondary Address and updates global
 *                                Secondary_Address value.
 *
 *  Example:
 *    Validate_Secondary_Address(); Check address list and validate.
 *
 *  Note:                         This is a user function.
 *
 ****************************************************************************/

void Validate_Secondary_Address(void) {
	int secondary_address;
	secondary_address = TNT_In(R_cptr) & 0x1f; /* Get Secondary Address from cpt */

	/* Check to see if valid address  */
	if (Multiple_Address_Table[secondary_address]) {
		TNT_Out(R_auxmr, F_valid); /* Valid: send valid aux. function*/
		Secondary_Address = secondary_address;
		CURRENT_ADDRESS = (secondary_address) | (CURRENT_ADDRESS & 0xff00);
	} else {
		TNT_Out(R_auxmr, F_nonvalid); /* Not valid: nonvalid function   */
	}
}

/***********************************  I/O  **********************************
 *
 *  User_GPIB_IO():        Can be used to send or recieve GPIB data.
 *
 *  Also defined as(aka):  Send(),Receive(),Send_ASYNC(),Receive_ASYNC()
 *
 *  Related Functions:     Setup_TNT_IO(),GPIB_DMA_IO(),GPIB_PROG_IO()
 *                         DONE_Handler()
 *
 *  Valid Parameters:      io_type = INPUT(4), OUTPUT(2)
 *                         method  = DMA(1), PROG(0)
 *                         buf     = char ptr to data
 *                         cnt     = 0 to 0xffffffff
 *                         term    = EOI(0x2000),EOS(0x1000),NONE(0),EOI|EOS(0x3000)
 *                         sync_or_async=SYNC(0), ASYNC(0x0020)
 *
 *  Example(s):
 *                         Read up to 100 bytes of data using DMA until terminated
 *                         by EOI syncronously (ie - wait till completed).
 *    1) User_GPIB_IO(INPUT,DMA,buf,100,EOI,SYNC);
 *
 *                         Write 11 bytes of data using PROGrammed I/O sending
 *                         EOS with EOI.
 *
 *    2) "io_param.h" file ... WRITE_EOS_BYTE = 0x0a;
 *       User_GPIB_IO(OUTPUT,PROG,"AbCd hello\n",11,EOI|EOS,SYNC);
 *
 *    3) Receive(buf,100,EOI);                  This is equivalent to (1)
 *
 *    4) "io_param.h" file ... WRITE_EOS_BYTE = 0x0a;
 *       Send_ASYNC("AbCd hello\n",11,EOI|EOS); This is equivalent to (2)
 *
 *  Note:                  This is a user function.
 *                         The method of transfer (DMA/PROG) for Send,Receive,
 *                         is determined by the parameter "USE_DMA  YES/NO" in
 *                         "io_param.h". If using HS488 or a multiple
 *                         addressing option make sure that the controller
 *                         is not asserting ATN before doing a Send/Receive.
 *                         The TNT will perform a NDAC holdoff on the HCE
 *                         command if you are using HS488. This command
 *                         must be verified by the Validate_CF_Command()
 *                         function. Similarly if using multi-addressing
 *                         the TNT will perform a NDAC holdoff until the
 *                         primary/secondary address is verified via the
 *                         Validate_Primary/Secondary_Address() function.
 *                         The "Validate" functions are called by the
 *                         Update_INTERFACE_STATUS() routine. To detect whether
 *                         the ATN is asserted use Read_GPIB_Lines().
 *
 ****************************************************************************/

 //打印系统时间
void printf_curr_time(char * str)
{
    struct timeval start;
    gettimeofday( &start, NULL);
    printf("%s : %ld.%ld\n", str, start.tv_sec, start.tv_usec);
}

void User_GPIB_IO(int io_type,int method,char *buf,unsigned long int cnt,int term,int sync_or_async)
{
	unsigned long int count_sent=0; /* Local count variable           */
	unsigned long int count;
	//strcat(buf,"\n");
	Update_INTERFACE_STATUS(); /* Update to get current status   */
	/* Clear I/O status bits          */
	INTERFACE_STATUS&=~(UCMPL|TIMO|END|EOS|ERR);
	DATA_COUNT=0; /* Clear count global             */

	if( ((Address_Mode==0x02)||(Address_Mode==0x03)) && (io_type==OUTPUT) && (CURRENT_ADDRESS!=MULTI_ADDRESS))
	{
		Generate_Error(EABO);
		return;
	}

	if(cnt==0)
	{ /* If cnt is zero                 */
		INTERFACE_STATUS|=UCMPL;
		return;
	}

	switch(method)
	{ /* Setup selected I/O             */

#if(USE_DMA)
		case DMA: /* Do DMA I/O */
		 /* Continue until terminated      */
		/*   or count = 0                 */
		while((!(INTERFACE_STATUS&(TIMO|END|EOS|ERR))) && (cnt!=0)) {

			count = cnt;
			/* If odd start address or one    */
			/*   byte read required           */
			if(((int)buf % 2) || ((count==1)&&(io_type==INPUT))) {
				/*   PROGram I/O first byte       */
				GPIB_PROG_IO(io_type,buf,1,(count==1)?term:NONE);
			}
			else {
				/* Else                           */
				/*   If read odd count decrement  */
				if((count%2) && (io_type==INPUT)) count--;

				GPIB_DMA_IO(io_type,buf,count,term);/* Do DMA transfer            */
			}

			DONE_Handler(io_type,&count_sent);
			DATA_COUNT+=count_sent; /* Update total transfer count    */
			cnt-=count_sent; /* Update total requested count   */
			buf+=count_sent; /* Update buffer pointer          */
		}

		break;
#endif

		case PROG:
		while((!(INTERFACE_STATUS&(TIMO|END|EOS|ERR))) && (cnt!=0))
		{
			GPIB_PROG_IO(io_type,buf,cnt,term);		/* Setup PROGgrammed I/O trans.  */
			//printf("User_GPIB_IO io_type = %d buf %x = %s\n",io_type, buf, buf);
			//printf_curr_time("User_GPIB_IO");
			DONE_Handler(io_type,&count_sent);		/* Finish up and get count        */
			DATA_COUNT+=count_sent; 			/* Update total transfer count    */
			cnt-=count_sent; 					/* Update total requested count   */
			buf+=count_sent; 					/* Update buffer pointer          */
		}
		break;

	}
	INTERFACE_STATUS|=UCMPL; /* Set the user complete bit      */
}

/****************************************************************************
 *
 *  IO_Control():         Can be used to turn off/on the interface thus
 *                        stopping/allowing/aborting I/O.
 *
 *  Also defined as(aka): Interface_On(),Interface_Off()
 *
 *  Related Functions:    Initialize_Interface(),User_GPIB_IO()
 *
 *  Valid Parameters:     operation = START(1),STOP(0)
 *
 *  Examples:
 *    1) Interface_Off();     Stop  I/O
 *    2) Interface_On();      Start I/O
 *    3) IO_Control(ENABLE);  Start I/O
 *
 *  Note:                 This is a user function. If interface is disabled
 *                        by Interface_Off() it will be turned on by
 *                        either Interface_On() or Initialize_Interface()
 *
 ****************************************************************************/

void IO_Control(int operation) {
	switch (operation) { /* Select operation               */

	case ENABLE:
		Initialize_Interface();
		break;

	case DISABLE:
		Set_Address_Mode(0x07); /* Address mode 7 doesn't update  */
		/*   Address_Mode global          */
		TNT_Out(R_auxmr, F_chrst); /* Set everything to idle state   */
		break;
	}
}

/****************************************************************************
 *
 *  Setup_TNT_IO():    Used by GPIB_DMA_IO and GPIB_PROG_IO to setup
 *                     I/O for the TNT.
 *
 *  Related Functions: GPIB_DMA_IO(),GPIB_PROG_IO,USER_GPIB_IO(),
 *                     DONE_Handler()
 *
 *  Valid Parameters:  io_type = INPUT(4),OUTPUT(2)
 *                     cnt     = 0 to 0xffffffff
 *                     term    = EOI(0x2000),EOS(0x1000),NONE(0),EOI|EOS(0x3000)
 *
 *  Note:              This is a system function. The case for INPUT_BYTE
 *                     is included so that the byte after the end of the 
 *                     buffer is not overwritten, when reading into a buffer 
 *                     with an odd starting address and/or when using an odd 
 *                     count. 
 *                     
 ****************************************************************************/

void Setup_TNT_IO(int io_type, unsigned long int cnt, int term) {
	unsigned long int twos_cnt = -cnt; /* Obtain the twos compliment cnt */
	Requested_Count = cnt; /* Save requested transfer cnt    */

	TNT_Out(R_cmdr, F_resetfifo); /* Reset TNT fifos                */

#if(USE_DMA)                              /* If using dma enable dma on     */
	TNT_Out(R_dmaenable, F_dmaon); /*  the evaluation board          */
#endif

	TNT_Out(R_cnt0, (char) (twos_cnt)); /* Load twos compliment count     */
	TNT_Out(R_cnt1, (char) (twos_cnt >> 8)); /*  into TNT count registers      */
	TNT_Out(R_cnt2, (char) (twos_cnt >> 16));
	TNT_Out(R_cnt3, (char) (twos_cnt >> 24));
	/* If using timeouts              */
	if (Timeout.factor_index > 0) { /*   Set B_to bit and maybe B_bto */
		//				11000000  10		1						1000
		TNT_Out(R_imr0, B_glint | B_to | ((Timeout.byte_timeout) ? B_bto : 0));
		//				0xf0
		TNT_Out(R_auxmr, HR_auxrj | Timeout.factor_index); /* Set time             */
	} else {
		TNT_Out(R_imr0, B_glint); /* Set write to imr0 to be sure   */
		TNT_Out(R_auxmr, HR_auxrj | 0); /*   B_to is cleared              */
	}

	switch (io_type) { /* Switch to input or output      */

	case INPUT_BYTE:
		TNT_Out(R_imr1, B_end); /* End transfer on eoi or eos     */

		TNT_Out(R_eosr, READ_EOS_BYTE); /* Set eos byte                   */
		TNT_Out(R_auxmr, HR_auxra | F_hlde | ((term & EOS)?B_endoneos:0));
		/* Configure for byte input       */
		TNT_Out(R_cfg, F_input_config & ~B_16bit);
		/* Holdoff on end & enable eos    */
		TNT_Out(R_cmdr, F_go); /* Start transfer state machine   */
		TNT_Out(R_auxmr, F_rhdf); /* Release holdoff                */
		break;

	case INPUT:

		TNT_Out(R_imr1, B_end); /* End transfer on eoi or eos     */

		TNT_Out(R_eosr, READ_EOS_BYTE); /* Set eos byte                   */
		TNT_Out(R_auxmr, HR_auxra | F_hlde | ((term & EOS)?B_endoneos:0));
		TNT_Out(R_cfg, F_input_config & ~B_16bit); /* Configure for word input       */
		/* Holdoff on end & enable eos    */
		TNT_Out(R_cmdr, F_go); /* Start transfer state machine   */
		TNT_Out(R_auxmr, F_rhdf); /* Release holdoff                */

		break;

	case OUTPUT:
		TNT_Out(R_imr1, B_err); /* End transfer on err            */

		TNT_Out(R_eosr, WRITE_EOS_BYTE); /* Set EOS byte                   */
		/* Holdoff on all & enable EOS    */
		TNT_Out(R_auxmr, HR_auxra | F_hlda | ((term & EOS)?B_xeoiweos:0));
		/* Configure for word output      */
		TNT_Out(R_cfg, (F_output_config & ~B_16bit) | ((term) ? B_ccen : 0));
		TNT_Out(R_auxmr, F_hldi); /* Hold off immediately           */
		TNT_Out(R_cmdr, F_go); /* Start transfer state machine   */
		break;

	default:
		Generate_Error(EARG); /* If io_type incorrect issue EARG*/
		break;
	}
}

#if(USE_DMA)
/****************************************************************************
 *
 *  GPIB_DMA_IO():     Used by User_GPIB_IO to setup DMA transfers.
 *                     This function configures the Intel 8237A DMA controller
 *                     to transfer data to/from the TNT fifos.
 *
 *
 *  Related Functions: GPIB_PROG_IO(),USER_GPIB_IO(),DONE_Handler()
 *
 *  Valid Parameters:  io_type = INPUT(4),OUTPUT(2)
 *                     buf     = char ptr to buffer
 *                     cnt     = 0 to 0xffffffff
 *                     term    = EOI(0x2000),EOS(0x1000),NONE(0),EOI|EOS(0x3000)
 *
 *  Note:              This is a system function.
 *bytesReceive : 1668.996650
 ****************************************************************************/

void GPIB_DMA_IO(int io_type,char ptr_size *buf,unsigned long int cnt,int term)
{
	unsigned long int twos_cnt,word_cnt,byte_cnt;/* Counts used               */
	unsigned long int bytes_to_boundary; /* address.page,address.offset    */
	struct address_type physical_address; /* bytes till 64k page boundary   */

	Virtual_to_Physical(buf, &physical_address);/* Obtain buffer physical     */
	/*    address                     */
	bytes_to_boundary=((-physical_address.offset)<<1) ? (-physical_address.offset)<<1:0xffff;
	/* 2*TWOS_COMPLIMENT = bytes      */
	/*  until segment boundary        */

	byte_cnt= (cnt>bytes_to_boundary) ? bytes_to_boundary : cnt;
	/* If cnt > bytes segment will    */
	/*   be crossed over              */

	term= (byte_cnt==cnt) ? term : NONE; /* Terminate if not crossing      */

	Setup_TNT_IO(io_type,byte_cnt,term);/* Setup TNT for IO               */

	DMA_Out(R_mask, B_disable|F_dmachipchannel);/* Disable DMA channel for    */
	/*  configuration                 */

	DMA_Out(R_bp0, F_reset); /* Reset lo/high byte pointer     */
	DMA_Out(R_bar, physical_address.offset);/* Low address to base addr reg   */
	DMA_Out(R_bar, physical_address.offset>>8);/* High address                */

	DMA_Out(R_page, physical_address.page); /* Output page no. to page reg    */

	word_cnt=(byte_cnt-1) >> 1; /* Word count = (byte count)/2    */
	/*   DMAC counts down to -1       */

	DMA_Out(R_bp0, F_reset); /* Byte pointer reset             */
	DMA_Out(R_bcntr, (char)(word_cnt)); /* Write low byte of word cnt     */
	DMA_Out(R_bcntr, (char)(word_cnt>>8)); /* Write high byte of word cnt    */

	DMA_Out(R_mode, (io_type==OUTPUT) ? F_dma_output:F_dma_input);
	DMA_Out(R_csr , F_reset); /* Reset command reg              */
	DMA_Out(R_mask, F_dmachipchannel); /* Enable dma channel             */

	while(!(TNT_In(R_isr3)&(B_done|B_tlcint)));/* Wait for done               */
}

/****************************************************************************
 *
 *  Virtual_To_Physical(): Used solely by GPIB_DMA_IO() to convert a
 *                         virtual buffer address to a physical address
 *                         using FP_SEG() and FP_OFF() calls.
 *
 *  Related Functions:     GPIB_DMA_IO()
 *
 *  Valid Parameters:      buf_add         =ptr to buffer
 *                         physical_address=ptr to physical address value
 *
 *  Note:                  This is a system function.
 *
 ****************************************************************************/

void Virtual_to_Physical(char ptr_size *buf_add, struct address_type *physical_address)
{
	unsigned long paddr;

	/* Get physical address           */
	/* PHYS=SEG(ADD)+OFF(ADD)         */
	paddr=(long)((long)(FP_SEG(((char far *)buf_add)))<<4)+FP_OFF(((char far *)buf_add));

	/* Form page number of BYTE       */
	physical_address->page =((long)(paddr & 0x000f0000) >> 16);

	/* Form offset of buffer          */
	physical_address->offset =((long)((paddr>>1) & 0x0000ffff));
}
#endif

/****************************************************************************
 *
 *  GPIB_PROG_IO():    Used by User_GPIB_IO() to initiate PROGrammed I/O
 *                     GPIB transfers.
 *
 *                     Reading Word(s):
 *                     There are three cases that make programmed I/O
 *                     transfers more efficient.
 *                     1) B_nff|          B_nef - at least one word is ready
 *                     2) B_nff|B_intsrc2|B_nef - fifo at least half full
 *                     3)       B_intsrc2|B_nef - fifo is completely full
 *
 *                     Writing Word(s):
 *                     There are three cases that make programmed I/O
 *                     transfers more efficient.
 *                     1) B_nff|          B_nef - at least one word is empty
 *                     2) B_nff|B_intsrc2|B_nef - fifo at least half empty
 *                     3) B_nff|B_intsrc2       - fifo is completely empty
 *
 *  Related Functions: GPIB_DMA_IO(),USER_GPIB_IO(),DONE_Handler()
 *
 *  Valid Parameters:
 *                     io_type = INPUT(4),OUTPUT(2)
 *                     buf     = char ptr to buffer
 *                     cnt     = 0 to 0xffffffff
 *                     term    = EOI(0x2000),EOS(0x1000),NONE(0),EOI|EOS(0x3000)
 *
 *  Note:              This is a system function. In the cases of write
 *                     transfers less than the size of the fifos (32 bytes),
 *                     it is possible for OUTPUT to write more data to the
 *                     fifos than requested.
 *
 *                     Example of a 5 byte write transfer:
 *                       Initially the count of 5 is written to the TNT4882
 *                       count registers. When OUTPUT is called it
 *                       checks R_isr3 for B_nff,B_nef,B_intsrc2 and sees
 *                       that the fifos are empty. Since they are empty,
 *                       OUTPUT can write 32 bytes to the fifos (and
 *                       it will). Only 5 bytes will go out on the GPIB
 *                       however, and 27 useless bytes will be sitting in
 *                       the fifos. The next I/O process will reset the
 *                       fifo's data pointers.
 *
 *                     To avoid adding an extraneous counter, the code is
 *                     written to let the TNT4882 do the job of keeping track
 *                     of the data to be written to the GPIB.  A possible
 *                     hazard of this occurs if the data buffer is on the edge
 *                     of hardware I/O space. It is then possible that the
 *                     pointer to the data buffer could be updated to point to
 *                     some io port.  Doing reads from hardware I/O
 *                     may cause some undesirable results. It is quite unlikely
 *                     that a data buffer will be allocated at the boarder of
 *                     memory & I/O space.
 *
 ****************************************************************************/

void GPIB_PROG_IO(int io_type, char *buf, unsigned long int cnt, int term)
{
	int i;
	int mr_isr3;

	if ((cnt % 2) && (io_type == INPUT) && (cnt != 1))
		cnt--;

	if ((cnt == 1) && (io_type == INPUT)) /* If one byte read INPUT_BYTE    */
		io_type = INPUT_BYTE;

	Setup_TNT_IO(io_type, cnt, term); /* Setup TNT                      */

	switch (io_type) {
	case INPUT: /* Read data on not empty         */

		//printf("[GPMC_Test] INPUT \n");
		while (!((mr_isr3 = TNT_In(R_isr3)) & B_done))
		{
			//printf("[GPMC_Test] INPUT -> while mr_isr3 = %X \n", mr_isr3);
			if ((!(mr_isr3 & B_nef)) && (TNT_In(R_isr0)&B_to))break;

			switch(mr_isr3&(B_nff|B_intsrc2|B_nef)) {

				case (B_nef):
				case (B_nef|B_intsrc2): /* Fifo full read 15 words        */
				for(i=0;i<15;i++)
				{
					*( (short *)buf ) = TNT_In_Word(R_fifob);
					buf += 2;
				}
				//printf("case (B_nef|B_intsrc2): buf %x = %s \n", buf, buf );
				break;

				case (B_nff|B_intsrc2|B_nef):
				for(i=0;i<8;i++) /* Fifo half full read 8 words    */
				{
					*( (short *)buf ) = TNT_In_Word(R_fifob);
					buf += 2;
				}
				//printf("case (B_nff|B_intsrc2|B_nef): buf %x = %s \n", buf, buf );
				break;

				case (B_nff|B_nef): /* At least one byte in fifo      */
					*( (short *)buf ) = TNT_In_Word(R_fifob);
					buf += 2;
				//printf("case (B_nff|B_nef): buf %x = %s \n", buf, buf );
				break;
			}
		}

		break;

		case INPUT_BYTE:
			//printf("[GPMC_Test] INPUT_BYTE \n");
			while(!((mr_isr3=TNT_In(R_isr3))&B_done)) {

			if((!(mr_isr3&B_nef)) && (TNT_In(R_isr0)&B_to))
			break;

			if(mr_isr3&B_nef)
			*(buf)=TNT_In(R_fifob);
		}
		break;

		case OUTPUT: /* Write data on not full         */
			//printf("[GPMC_Test] OUTPUT \n");
			while(!((mr_isr3=TNT_In(R_isr3))&B_done))
			{

			if(mr_isr3&B_tlcint)
			break;

			switch(mr_isr3&(B_nff|B_intsrc2|B_nef))
			{

				case (B_nff):
				case (B_nff|B_intsrc2): /* 16 words in fifo are empty     */
				for(i=0;i<16;i++)
				{
					TNT_Out_Word(R_fifob,*((short *)buf));
					buf += 2;
				}
				break;

				case (B_nff|B_intsrc2|B_nef): /* 8 words in fifo are empty      */
				for(i=0;i<8;i++)
				{
					TNT_Out_Word(R_fifob,*((short *)buf));
					buf += 2;
				}
				break;

				case (B_nff|B_nef): /* 1 word in fifo is empty        */
					TNT_Out_Word(R_fifob,*((short *)buf));
					buf += 2;
				break;
			}
		}
		break;
	}
}

			/****************************************************************************
			 *
			 *  DONE_Handler():    Routine for completing an I/O transfer
			 *                     updating counts, and buffers.
			 *
			 *  Related Functions: GPIB_DMA_IO(),USER_GPIB_IO(),GPIB_PROG_IO(),
			 *
			 *  Valid Parameters:  io_type    = INPUT(4),OUTPUT(2)
			 *                     count_sent = ptr to count_sent value
			 *
			 *  Note:              This is a system function
			 *
			 ****************************************************************************/

void DONE_Handler(int io_type, unsigned long int *count_sent) {
	if (io_type == INPUT)
		MULTI_ADDRESS = CURRENT_ADDRESS;

	Update_INTERFACE_STATUS(); /* Get current status             */

	TNT_Out(R_cmdr, F_stop); /* Stop fifos                     */
	TNT_Out(R_cmdr, F_resetfifo); /* Reset the fifos                */

#if(USE_DMA)                              /* If using DMA                   */
	DMA_Out(R_mask,B_disable|F_dmachipchannel);/* Disable DMA channel on chip */
	TNT_Out(R_dmaenable,F_reset); /* Disable DMA on board           */
	DMA_In(R_csr); /* Clear channel status reg       */
#endif

	if (TNT_In(R_isr1) & B_end) /* If we received an END          */
		TNT_Out(R_auxmr, F_clrEND); /*   Clear status bit             */

	if (TNT_In(R_isr1) & B_err) {
		Generate_Error(ENOL); /* No listeners                   */
		TNT_Out(R_auxmr, F_clrERR); /* Clear error bit                */

		if (io_type == OUTPUT) {
			TNT_Out(R_hssel, F_onechip | B_go2sids); /* if error set to idle state.    */
			TNT_Out(R_hssel, F_onechip);
		}
	}

	if (Timeout.factor_index) { /* If we were using timeouts      */
		TNT_Out(R_auxmr, HR_auxrj | 0); /*   Clear timer                  */

		if (INTERFACE_STATUS & TIMO) /* If timeout set ERR             */
			Generate_Error(EABO); /*   operation aborted            */
	}

	*count_sent = Get_DATA_COUNT(); /* Obtain transfer count          */
}

