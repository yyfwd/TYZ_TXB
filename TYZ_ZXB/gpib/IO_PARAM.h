/**************************************************************************
 *
 *  IO_PARAM.H device parameter header file
 *
 *  Description: This file contains important global information
 *               such as hardware and data configuration options.
 *
 *  Note:        This header file is for the non-interrupt driven package.
 *
 **************************************************************************/

/******* GPIB Parameters **************************************************/


/******* Addressing         ***********************************************/

#ifndef __IO_PARAM_H
#define __IO_PARAM_H

#define ADDRESS_MODE            0    /* 0 single primary                  */
				     /* 1 single primary single secondary */
				     /* 2 single primary multiple secondary */
				     /* 3 multiple primary                */
				     /* 4 talk only mode                  */
				     /* 5 listnen only mode               */
				     /* 6 no addressing                   */

#define PRIMARY_ADDRESS         1    /* GPIB chip's GPIB primary address  */

#define SECONDARY_ADDRESS       5    /* GPIB chip's GPIB secondary address*/

#define MULTIPLE_ADDRESSES   {1,2,3,4,5} /* GPIB multiple primary or secondary*/
				     /*   addresses if using ADDRESS_MODE */
				     /*   options 2 or 3                  */

#define NUMBER_OF_ADDRESSES     5    /* Number of addresses in list above */

/******* I/O                ***********************************************/

#define TIMEOUT_FACTOR_INDEX   0     /* Timeout time factor index         */
				     /*   0 disables timeouts             */
#define USE_BYTE_TIMEOUTS      YES   /* Reset Timeout timer on each byte  */

#define READ_EOS_BYTE  0x0a          /* EOS byte for reads/inputs/receives*/
#define WRITE_EOS_BYTE 0x0a          /* EOS byte for writes/outputs/sends */

#define USE_EIGHT_BIT_EOS_COMPARE NO /* Compare EOS byte 7 or 8 bits      */
#define USE_TRANSMIT_EOI_WITH_EOS NO /* Send EOI with EOS byte (writes)   */

#define USE_HIGH_SPEED_T1      YES   /* Short T1 delay while doing three  */
				     /*   wire handshaking                */

/******* Miscelanious Interrupt Handlers **********************************/

#define USE_SPOLL_BIT          YES   /*   STBO (SPOLL) - Serial Poll      */

#define USE_HS488_HANDLER      YES   /*   CPT - Command Passthrough       */
				     /*     (recognizes the HSC function) */


/******* Initial High Speed Parameters ************************************/

#define HS_MODE       ONE_CHIP       /* High speed transfer mode          */
#define CABLE_LENGTH  15             /* Total length of cable in system   */



/******* TNT, DMA & Interrupt Hardware Parameters *************************/

#define TNT_BASE_ADDRESS    0x2c0    /* Board Base I/O address            */
#define DMA_CHANNEL         6        /* DMA channel in use                */
//#define USE_DMA             YES      /* Include DMA code ?                */
#define USE_DMA             NO
#define USE_DEMAND_MODE_DMA YES      /* Will use single cycle otherwise   */


/******* Software Configuration *******************************************/

#define USE_HUGE_BUFFERS  NO        /* If buffers will cross 64k segment */
				     /*   boundaries.  Buffer sizes > 64k */
#define USE_MICROSOFT_C   NO         /* If using Microsoft C instead of   */
				     /*   Borland C                       */

#define NO  0
#define YES 1

#endif
