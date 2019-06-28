/****************************************************************************
 *
 *  NGPIB_IO.H ngpib_io.c header file.
 *
 *  Description:
 *  ============
 *  Contains function prototypes, global gpib constants and
 *  the TNT T/L register and bit/field constants.
 *
 ****************************************************************************/

#ifndef __NGPIB_IO_H
#define __NGPIB_IO_H

#if(USE_HUGE_BUFFERS)
  #define ptr_size huge
#else
  #define ptr_size
#endif


struct address_type {int offset;
		    int page;
		    };
/***************************************** Status                           */

void Update_4882_Status(int status_register,unsigned short byte,int operation);
  #define Set_4882_Status(status_register,byte)   Update_4882_Status(status_register,byte,SET)
  #define Clear_4882_Status(status_register,byte) Update_4882_Status(status_register,byte,CLEAR)

#define Read_4882_Status(status_register)  ((status_register==STB)? TNT_In(R_spsr) :(MR_4882_status[status_register]))

void Wait_For_Interface(int mask);

void Generate_Error(int code);

#define TF(l) ((l)==0 ? (0): (1))


/***************************************** Initialization                   */

void Initialize_Interface(void);

void High_Speed_Select(int cable_length,int enable);

void Set_Timeout(int factor,int enable);


/****************************************** Addressing Functions            */

void Change_Primary_Address(int new_address);

void Change_Secondary_Address(int new_address);

void Change_Multiple_Addresses(int *new_address_list,int num_of_addresses);

void Set_Address_Mode(int mode);

void Validate_Primary_Address(void);

void Validate_Secondary_Address(void);


/****************************************** I/O                             */

void User_GPIB_IO(int io_type, int io_method, char * buf, unsigned long int cnt, int term, int sync_or_async);

#define Send(buf,cnt,term)          User_GPIB_IO(OUTPUT,USE_DMA,buf,cnt,term,SYNC)
#define Receive(buf,cnt,term)       User_GPIB_IO(INPUT, USE_DMA,buf,cnt,term,SYNC)

#define Read_GPIB_Lines()   ((TNT_In(R_dsr)|TNT_In(R_bsr)<<8))

#define Talk_Only_Enable()    TNT_Out(R_admr,F_talkonly)

#define Listen_Only_Enable()  TNT_Out(R_admr,F_listenonly)

void IO_Control(int enable);
  #define Interface_On() IO_Control(ENABLE)
  #define Interface_Off() IO_Control(DISABLE)


/****************************************** Low Level GPIB I/O Functions    */

void Setup_TNT_IO(int io_type, unsigned long int cnt, int term);

void GPIB_DMA_IO(int io_type,char *buf,unsigned long int cnt,int term);

void Virtual_to_Physical(char *buf, struct address_type *physical_address);

void GPIB_PROG_IO(int io_type,char *buf,unsigned long int cnt,int term);

void DONE_Handler(int io_type,unsigned long int *count_sent);

#define Get_DATA_COUNT() (Requested_Count+(((unsigned long int)TNT_In(R_cnt0))|((unsigned long int)TNT_In(R_cnt1)<<8)|((unsigned long int)TNT_In(R_cnt2)<<16)|((unsigned long int)TNT_In(R_cnt3)<<24)))

int Update_INTERFACE_STATUS(void);

/****************************************** Low Level Register IO Functions */

/*
#if(USE_MICROSOFT_C)
  #define TNT_Out(reg,byte) outp(TNT_BASE_ADDRESS+reg,byte)

  #define TNT_Out_Word(reg,word) outpw(TNT_BASE_ADDRESS+reg,word)

  #define TNT_In_Word(reg) ((int)inpw(TNT_BASE_ADDRESS+reg))

  #define TNT_In(reg)      ((unsigned short) inp(TNT_BASE_ADDRESS+reg))

  #define DMA_Out(reg,byte) outp(reg,byte)

  #define DMA_In(reg) ((unsigned short) inp(reg))
#else
  #define TNT_Out(reg,byte) outportb(TNT_BASE_ADDRESS+reg,byte)

  #define TNT_Out_Word(reg,word) outport(TNT_BASE_ADDRESS+reg,word)

  #define TNT_In_Word(reg) ((int)inport(TNT_BASE_ADDRESS+reg))

  #define TNT_In(reg)      ((unsigned short) inportb(TNT_BASE_ADDRESS+reg))

  #define DMA_Out(reg,byte) outportb(reg,byte)

  #define DMA_In(reg) ((unsigned short) inportb(reg))
#endif
*/

/****************************************** Miscelaneous Control Funtions   */

void CPT_Handler(void);

void Validate_CF_Command(void);

/******************************************  Useful Constants               */

#define TRUE     1
#define YES      1
#define ENABLE   1
#define SET      1

#define FALSE    0
#define NO       0
#define NONE     0
#define DISABLE  0
#define CLEAR    0

#define INPUT    4
#define INPUT_BYTE 16
#define OUTPUT   2

#define INTERFACE  1
#define USER       1
#define ERROR_CODE 2

#define DMA        1
#define PROG       0

#define NO_TSETUP 0xff


/****************************************** Interface Status Constants      */

extern int INTERFACE_STATUS;

#define ERR  (1<<15)
#define TIMO (1<<14)
#define END  (1<<13)
#define EOS  (1<<12)

#define RQS  (1<<11)
#define IFC  (1<<10)
#define SPOLL (1<<9)
#define UCMPL (1<<8)

#define LOK   (1<<7)
#define REM   (1<<6)
#define ASYNC (1<<5)
#define DTAS  (1<<4)

#define DCAS  (1<<3)
#define LACS  (1<<2)
#define TACS  (1<<1)
#define NACS  (1<<0)

#define SYNC    0
#define NONE    0
#define EOI     END

extern unsigned long int DATA_COUNT;
extern unsigned long int Requested_Count;
extern int Address_Mode;

/****************************************** Error Codes                     */

extern int INTERFACE_ERROR;

#define ENOL 1
#define EARG 2
#define EABO 3


/****************************************** High Speed Constants            */

#define ONE_CHIP       2


/****************************************** Addressing Globals              */

extern int CURRENT_ADDRESS;
extern int MULTI_ADDRESS;

/****************************************** 488.2 Status Regiters           */

extern unsigned short MR_4882_status[];

#define STB 0
#define SRE 1
#define ESE 2
#define ESR 3
#define IST 4

/****************************************** Needed Types                    */



struct timeout_type { int factor_index;
		      int byte_timeout;
		    };


/****************************************************************************
 *
 * Define TNT register map and TL related bits and functions
 *
 * FORMAT:
 *      register       address
 *            B_bits         value
 *            F_function     value     (F_field)
 *
 ****************************************************************************/

/****************************** Data IO/OUT Registers                       */

#define R_dir          0x00
#define R_cdor         0x00

/****************************** Interrupt Mask and Status Registers         */

#define R_imr0         0x1d

#if(USE_SPOLL_BIT)
  #define     B_glint    0xc0
#else
  #define     B_glint    (1<<7)
#endif

  #define     B_bto      (1<<4)
#define R_isr0         0x1d
  #define     B_cdba     (1<<7)
  #define     B_stbo     (1<<6)
  #define     B_nl       (1<<5)
  #define     B_eos      (1<<4)
  #define     B_ifc      (1<<3)
  #define     B_atn      (1<<2)
  #define     B_to       (1<<1)
  #define     B_sync     (1<<0)

#define R_imr1         0x02
#define R_isr1         0x02
  #define     B_cpt      (1<<7)
  #define     B_apt      (1<<6)
  #define     B_det      (1<<5)
  #define     B_end      (1<<4)
  #define     B_dec      (1<<3)
  #define     B_err      (1<<2)
  #define     B_do       (1<<1)
  #define     B_di       (1<<0)

#define R_imr2         0x04
  #define     B_dmao     (1<<5)
  #define     B_dmai     (1<<4)
#define R_isr2         0x04
  #define     B_int      (1<<7)
  #define     B_lok      (1<<5)
  #define     B_rem      (1<<4)
  #define     B_lokc     (1<<2)
  #define     B_remc     (1<<1)
  #define     B_adsc     (1<<0)

#define R_imr3         0x12
#define R_isr3         0x1a
  #define     B_x        (1<<7)
  #define     B_intsrc2  (1<<6)
  #define     B_intsrc1  (1<<5)
  #define     B_intstop  (1<<4)
  #define     B_nff      (1<<3)
  #define     B_nef      (1<<2)
  #define     B_tlcint   (1<<1)
  #define     B_done     (1<<0)

/***************************** Serial Poll Status and Mode Registers        */

#define R_spsr         0x06
  #define     B_pend     (1<<6)
#define R_spmr         0x06
  #define     B_rsv      (1<<6)

/***************************** Address Mode, Status and Control Registers   */

#define R_adsr         0x08
  #define     B_natn     (1<<6)
  #define     B_spms     (1<<5)
  #define     B_lpas     (1<<4)
  #define     B_tpas     (1<<3)
  #define     B_la       (1<<2)
  #define     B_ta       (1<<1)

#define R_admr         0x08
  #define     F_talkonly             0xb0
  #define     F_listenonly           0x70

  #define     F_noaddress            0x30
  #define     F_normalprimary        0x31
  #define     F_normalextended       0x32
  #define     F_primarymultiextended 0x33


#define R_adr          0x0c
  #define     B_ars      (1<<7)
  #define     B_dt       (1<<6)
  #define     B_dl       (1<<5)

#define R_adr0         0x0c
#define R_adr1         0x0e

/*****************************   Data Count Registers                       */

#define R_cnt0         0x14
#define R_cnt1         0x16
#define R_cnt2         0x09
#define R_cnt3         0x0b

#define R_cptr         0x0a

/*****************************    Auxillary and Hidden Registers            */
#define R_auxmr        0x0a
   #define F_pon         0x00
   #define F_chrst       0x02
   #define F_rhdf        0x03
   #define F_seoi        0x06
   #define F_nonvalid    0x07
   #define F_valid       0x0f
   #define F_ist0        0x01
   #define F_ist1        0x09
   #define F_lut         0x0b
   #define F_lul         0x0c
   #define F_reqt        0x18
   #define F_reqf        0x19
   #define F_hldi        0x51

   /*************************   Clear Interrupt Flags                       */
   #define F_clrDET      0x54
   #define F_clrEND      0x55
   #define F_clrDEC      0x56
   #define F_clrERR      0x57
   #define F_clrLOKC     0x59
   #define F_clrREMC     0x5a
   #define F_clrADSC     0x5b
   #define F_clrIFCI     0x5c
   #define F_clrATNI     0x5d
   #define F_clrSYNC     0x5e
   #define F_setSYNC     0x5f

   /*************************    Hidden Auxillary Registers                 */
#define HR_ppr         0x60
   #define B_u           (1<<4)
   #define B_s           (1<<3)

#define HR_auxra       0x80
   #define F_normal      0x00
   #define F_hlda        0x01
   #define F_hlde        0x02
   #define B_endoneos    (1<<2)|((USE_EIGHT_BIT_EOS_COMPARE)? (1<<4):0)
#if(!(USE_TRANSMIT_EOI_WITH_EOS))
   #define B_xeoiweos    (1<<3)|((USE_EIGHT_BIT_EOS_COMPARE)? (1<<4):0)
#else
   #define B_xeoiweos    0x00
#endif

#define HR_auxrb       0xa0
   #define B_hldacpt     (1<<0)
   #define B_hst1        (1<<2)


#define HR_auxre       0xc0
   #define B_dhdc        (1<<0)
   #define B_dhdt        (1<<1)
   #define B_dhadc       (1<<2)
   #define B_dhadt       (1<<3)

#define HR_auxrf       0xd0
   #define B_dhall       (1<<0)
   #define B_dhunlt      (1<<1)
   #define B_dhala       (1<<2)
   #define B_dhata       (1<<3)

#define HR_auxrg       0x40
   #define B_ches        (1<<0)
   #define B_ntnl        (1<<3)

#define HR_auxri       0xe0
   #define B_sisb        (1<<0)
   #define B_ustd        (1<<3)

#define HR_auxrj       0xf0

#define R_hier         0x13
   #define B_dga         (1<<7)
   #define B_dgb         (1<<6)
   #define B_notsetup    (1<<4)
   #define B_pmtweos     (1<<0)

#define R_eosr         0x0e

#define R_misc         0x15
   #define B_hse         (1<<4)
   #define B_slow        (1<<3)
   #define B_wrap        (1<<2)
   #define B_noas        (1<<1)
   #define B_nots        (1<<0)

#define R_sts1        0x10
   #define B_stdone      (1<<7)
   #define B_stin        (1<<5)
   #define B_ststop      (1<<4)
   #define B_halt        (1<<1)

#define R_cfg          0x10
   #define B_command     (1<<7)
   #define B_tlchlte     (1<<6)
   #define B_in          (1<<5)
   #define B_fifoa       (1<<4)
   #define B_ccen        (1<<3)
   #define B_tmoe        (1<<2)
   #define B_timbytn     (1<<1)
   #define B_16bit       (1<<0)
   #define F_input_config  (B_in|B_tlchlte|B_tmoe|B_timbytn|B_16bit|B_ccen)
   #define F_output_config      (B_tlchlte|B_tmoe|B_timbytn|B_16bit)

#define R_dsr          0x11
#define R_sh_cnt       0x11
   #define HR_t1         0x00
      #define B_pt1ena     (1<<5)
   #define HR_t11        0xc0
   #define HR_t12        0x80
   #define HR_t17        0x40

#define R_sasr3        0x13
#define R_hssel        0x0d
   #define B_go2sids     (1<<5)
   #define B_nodma       (1<<4)
   #define F_onechip      0x01

#define R_sasr2        0x15
#define R_tntcsr       0x17
#define R_keyrg        0x17

/****************************  TNTs FIFOs                                   */
#define R_fifob        0x18
#define R_fifoa        0x19

#define R_ccrg         0x1a
#define R_sasr         0x1b
#define R_dcr          0x1b

#define R_sts2        0x1c
   #define B_st168n      (1<<6)
   #define B_staffn      (1<<3)
   #define B_staefn      (1<<2)
   #define B_stbffn      (1<<1)
   #define B_stbefn      (1<<0)

#define R_cmdr         0x1c
   #define F_softreset   0x22
   #define F_resetfifo   0x10
   #define F_stop        0x08
   #define F_go          0x04

#define R_timer        0x1e

#define R_bsr          0x1f
#define R_bcr          0x1f

#define R_dmaenable 0x05
  #define F_dmaon      0x01


/****************************************************************************
 *
 *  8237A DMA controller register definitions for channels 5-7 (16 bit)
 *
 ****************************************************************************/

#define B_disable  (1<<2)                 /* DMA disable = bit 4 of cmd reg */

extern int page_reg[];

#if (DMA_CHANNEL>4)
  #define R_bar     (0x0c4+((DMA_CHANNEL-5)*4))/* DMA base address register */
  #define R_bcntr   (0x0c6+((DMA_CHANNEL-5)*4))/* DMA base count register   */
#else
  #define R_bar     (DMA_CHANNEL*4)
  #define R_bcntr   (DMA_CHANNEL*4+2)
#endif 

#define R_page    (page_reg[DMA_CHANNEL])

#define R_csr     0x0d0                   /* DMA command/status register    */

#define R_mask    0x0d4                   /* DMA set/clear mask register    */
  #define F_dmachipchannel (0x03 & DMA_CHANNEL)

#define R_mode    0x0d6                   /* DMA channel mode select        */

#if(USE_DEMAND_MODE_DMA)
  #define F_dma_input     (0x04|F_dmachipchannel)
  #define F_dma_output    (0x08|F_dmachipchannel)
#else
  #define F_dma_input     (0x44|F_dmachipchannel)
  #define F_dma_output    (0x48|F_dmachipchannel)
#endif

#define R_bp0     0x0d8                   /* DMA byte pointer reset         */
  #define F_reset 0x00

#define R_intr 0x07


#endif

