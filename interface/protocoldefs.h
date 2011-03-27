/* ==========================================================================*/
/*  - - - - - - -   p r o t o c o l d e f s. h   - - - - - - - - - - - - - - */
/* ==========================================================================*/
/*    Definition File for the SDR-xx protocol and Control Items              */
/*  Created 29-12-2001														 */
/*  Modified 12-09-2008		msw    added  CI_RX_AD_MODES                     */
/*	Modified 03-03-2009		msw	   added  CI_RX_OUT_FILE_UPDATEPARAMS        */
/*  Modified 07-05-2009		msw    added  CI_TX_DA_MODE                      */
/*  Modified 11-19-2009		msw    add CI_TX_PULSE_MODE,CI_GENERAL_PRODUCT_ID*/
/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
#ifndef PROTOCOLDEFS_H
#define PROTOCOLDEFS_H

#define ASCP_INTERFACE_VERSION 7

/*---------------------------------------------------------------------------*/
/*---------------------> Message Header Defines <----------------------------*/
/*---------------------------------------------------------------------------*/
#define LENGTH_MASK 0x1FFF			/* mask for message length               */
#define TYPE_MASK 0xE0				/* mask for upper byte of header         */

#define TYPE_HOST_SET_CITEM (0<<5)
#define TYPE_HOST_REQ_CITEM (1<<5)
#define TYPE_HOST_REQ_CITEM_RANGE (2<<5)
#define TYPE_HOST_DATA_ITEM0 (4<<5)
#define TYPE_HOST_DATA_ITEM1 (5<<5)
#define TYPE_HOST_DATA_ITEM2 (6<<5)
#define TYPE_HOST_DATA_ITEM3 (7<<5)

#define TYPE_TARG_RESP_CITEM (0<<5)
#define TYPE_TARG_UNSOLICITED_CITEM (1<<5)
#define TYPE_TARG_RESP_CITEM_RANGE (2<<5)
#define TYPE_TARG_DATA_ITEM0 (4<<5)
#define TYPE_TARG_DATA_ITEM1 (5<<5)
#define TYPE_TARG_DATA_ITEM2 (6<<5)
#define TYPE_TARG_DATA_ITEM3 (7<<5)

#define TYPE_DATA_ITEM_ACK (3<<5)
 #define DATA_ITEM_ACK_LENGTH (3)

/*  2 byte NAK response to any unimplemented messages */
#define TARG_RESP_NAK (0x0002)

#define MAX_MSG_LENGTH (8192+2)
/*---------------------------------------------------------------------------*/
/*----------------------> Control Item Defines <-----------------------------*/
/*---------------------------------------------------------------------------*/

/*  General Interface Control Items  */
#define CI_GENERAL_INTERFACE_NAME 0x0001
 #define CI_GENERAL_INTERFACE_NAME_REQLEN 4		//response is variable length zero terminated string

#define CI_GENERAL_INTERFACE_SERIALNUM 0x0002
 #define CI_GENERAL_INTERFACE_SERIALNUM_REQLEN 4	//response is variable length zero terminated string

#define CI_GENERAL_INTERFACE_VERSION 0x0003
 #define CI_GENERAL_INTERFACE_VERSION_REQLEN 4
 #define CI_GENERAL_INTERFACE_VERSION_SETRESPLEN 6	//2 byte version

#define CI_GENERAL_HARDFIRM_VERSION 0x0004
 #define CI_GENERAL_HARDFIRM_VERSION_REQLEN 5
 #define CI_GENERAL_HARDFIRM_VERSION_SETRESPLEN 7
	#define CI_GENERAL_HARDFIRM_VERSION_PARAM_BOOTVER 0
	#define CI_GENERAL_HARDFIRM_VERSION_PARAM_APPVER 1
	#define CI_GENERAL_HARDFIRM_VERSION_PARAM_HWVER 2

#define CI_GENERAL_STATUS_CODE 0x0005
 #define CI_GENERAL_STATUS_CODE_REQLEN 4
 #define CI_GENERAL_STATUS_CODE_SETRESPLEN 5
  #define GENERAL_STATUS_IDLE 0x0B		//not running
  #define GENERAL_STATUS_BUSY 0x0C		//running
  #define GENERAL_STATUS_ADOVERLOAD 0x20
  #define GENERAL_STATUS_BOOTIDLE 0x0E	//in booloader mode idle
  #define GENERAL_STATUS_BOOTBUSY 0x0F	//in bootloader mode programming
  #define GENERAL_STATUS_BOOTERROR 0x80	//in booloader mode with programming error

/*  Receiver Specific Control Items  */
#define CI_RX_STATE 0x0018
 #define CI_RX_STATE_REQLEN 5
 #define CI_RX_STATE_SETRESPLEN 8
	//NetSDR/SDR-IP parameters
  #define RX_STATE_DATACOMPLEX 0x80		//param 1 NetSDR channel and data type
  #define RX_STATE_DATAREAL 0x00

  #define RX_STATE_IDLE 0x01			//param 2   run state
  #define RX_STATE_ON 0x02

  #define MODE_CONTIGUOUS24 0x80		//param 3   run mode
  #define MODE_CONTIGUOUS16 0x00
  #define MODE_CONTINUOUS24 0x81
  #define MODE_CONTINUOUS16 0x01
  #define MODE_HWSYNC24 0x83
  #define MODE_HWSYNC16 0x03

	//SDRIQ/14 parameters
#define RX_STATE_COMPLEX_HF 0x81		//SDR-IQ/14 param 1 channel and data type


#define CI_GENERAL_PRODUCT_ID 0x0009
 #define CI_GENERAL_PRODUCT_ID_REQLEN 4
 #define CI_GENERAL_PRODUCT_ID_SETRESPLEN 8
// Product ID for SDRIP
  #define SDRIP_PRODID0 0x53
  #define SDRIP_PRODID1 0x44
  #define SDRIP_PRODID2 0x52
  #define SDRIP_PRODID3 0x03
// Product ID for NETSDR
  #define NETSDR_PRODID0 0x53
  #define NETSDR_PRODID1 0x44
  #define NETSDR_PRODID2 0x52
  #define NETSDR_PRODID3 0x04

#define CI_RX_FREQUENCY 0x0020
 #define CI_RX_FREQUENCY_REQLEN 5
 #define CI_RX_FREQUENCY_SETRESPLEN 10
  #define CI_RX_FREQUENCY_NCO 0			//param 1
  #define CI_RX_FREQUENCY_DISPLAY 1

#define CI_RX_RF_GAIN 0x0038
 #define CI_RX_RF_GAIN_REQLEN 5
 #define CI_RX_RF_GAIN_SETRESPLEN 6

#define CI_RX_IF_GAIN 0x0040		//not used by NetSDR/SDRIP
 #define CI_RX_IF_GAIN_REQLEN 5
 #define CI_RX_IF_GAIN_SETRESPLEN 6

#define CI_RX_RF_FILTER 0x0044
 #define CI_RX_RF_FILTER_REQLEN 5
 #define CI_RX_RF_FILTER_SETRESPLEN 6
  #define CI_RX_RF_FILTER_AUTO 0
  #define CI_RX_RF_FILTER_BP1 1
  #define CI_RX_RF_FILTER_BP2 2
  #define CI_RX_RF_FILTER_BP3 3
  #define CI_RX_RF_FILTER_BP4 4
  #define CI_RX_RF_FILTER_BP5 5
  #define CI_RX_RF_FILTER_BP6 6
  #define CI_RX_RF_FILTER_BP7 7
  #define CI_RX_RF_FILTER_BP8 8
  #define CI_RX_RF_FILTER_BP9 9
  #define CI_RX_RF_FILTER_BP10 10
  #define CI_RX_RF_FILTER_BP11 11

#define CI_RX_AF_GAIN 0x0048
 #define CI_RX_AF_GAIN_REQLEN 5
 #define CI_RX_AF_GAIN_SETRESPLEN 6

#define CI_RX_AD_MODES 0x008A
 #define CI_RX_AD_MODES_REQLEN 5
 #define CI_RX_AD_MODES_SETRESPLEN 6
  #define CI_AD_MODES_DITHER 0x01	//bit field defs
  #define CI_AD_MODES_PGA 0x02
  #define CI_AD_MODES_TRANSV 0x04	//use transverter path as input source

#define CI_RX_IN_SAMPLE_RATE 0x00B0
 #define CI_RX_IN_SAMPLE_RATE_REQLEN 5
 #define CI_RX_IN_SAMPLE_RATE_SETRESPLEN 9

#define CI_RX_SYNCIN_MODE_PARAMETERS 0x00B4
 #define CI_RX_SYNCIN_MODE_PARAMETERS_REQLEN 5
 #define CI_RX_SYNCIN_MODE_PARAMETERS_SETRESPLEN 8	//p0=chan, p1=mode, p2,3 = number packets
  #define CI_RX_SYNCIN_MODE_OFF 0
  #define CI_RX_SYNCIN_MODE_NEGEDGE 1
  #define CI_RX_SYNCIN_MODE_POSEDGE 2
  #define CI_RX_SYNCIN_MODE_LOWLEVEL 3
  #define CI_RX_SYNCIN_MODE_HIGHLEVEL 4
  #define CI_RX_SYNCIN_MODE_MUTELOW 5
  #define CI_RX_SYNCIN_MODE_MUTEHIGH 6

#define CI_RX_PULSEOUT_MODE 0x0B6
 #define CI_RX_PULSEOUT_MODE_REQLEN 5
 #define CI_RX_PULSEOUT_MODE_SETRESPLEN 6
  #define CI_PULSEOUT_MODE_OFF 0		//mode defs
  #define CI_PULSEOUT_MODE_RUNSTATE 1
  #define CI_PULSEOUT_MODE_RUNTOG 2
  #define CI_PULSEOUT_MODE_SAMPRATE 3

#define CI_RX_OUT_SAMPLE_RATE 0x00B8
 #define CI_RX_OUT_SAMPLE_RATE_REQLEN 5
 #define CI_RX_OUT_SAMPLE_RATE_SETRESPLEN 9

#define CI_RX_OUTPUT_PARAMS 0x00C4
 #define CI_RX_OUTPUT_PARAMS_REQLEN 4
 #define CI_RX_OUTPUT_PARAMS_SETRESPLEN 5

#define CI_RX_UDP_OUTPUT_PARAMS 0x00C5
 #define CI_RX_UDP_OUTPUT_PARAMS_REQLEN 4
 #define CI_RX_UDP_OUTPUT_PARAMS_SETRESPLEN 10

#define CI_RX_CALIBRATION_DATA 0x00D0
 #define CI_RX_CALIBRATION_DATA_REQLEN 5
 #define CI_RX_CALIBRATION_DATA_SETRESPLEN 7

/*  Transmitter Specific Control Items  */
#define CI_TX_DA_MODE 0x012A
 #define CI_TX_DA_MODE_REQLEN 5
 #define CI_TX_DA_MODE_SETRESPLEN 6
  #define CI_DA_MODE_OFF 0x00		//mode defs
  #define CI_DA_MODE_AD 0x01
  #define CI_DA_MODE_NCOTRACK 0x02
  #define CI_DA_MODE_NOISE 0x03

/* Aux Port Control Items  */
#define CI_OPEN_ASYNC_PORT 0x0200
 #define CI_OPEN_ASYNC_PORT_SETRESPLEN 14
#define CI_CLOSE_ASYNC_PORT 0x0201
 #define CI_CLOSE_ASYNC_PORT_SETRESPLEN 5


/*  Software/Firmware Update MODE Control Items  */
#define CI_UPDATE_MODE_CONTROL 0x0300
 #define CI_UPDATE_MODE_CONTROL_SETREQLEN 10
 #define CI_UPDATE_MODE_CONTROL_SETRESPLEN 10
 #define CI_UPDATE_MODE_CONTROL_ENTER 0
 #define CI_UPDATE_MODE_CONTROL_START 1
 #define CI_UPDATE_MODE_CONTROL_END 2
 #define CI_UPDATE_MODE_CONTROL_ABORT 3
 #define CI_UPDATE_MODE_CONTROL_ERASE 4

#define CI_UPDATE_MODE_PARAMS 0x0302
 #define CI_UPDATE_MODE_PARAMS_SETREQLEN 5
 #define CI_UPDATE_MODE_PARAMS_SETRESPLEN 17

 #define PROG_FLASH_ID 0
 #define CONFIG_FLASH_ID 1

#endif //PROTOCOLDEFS_H
