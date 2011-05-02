/* ==========================================================================*/
/*  - - - - - - -   p r o t o c o l d e f s. h   - - - - - - - - - - - - - - */
/* ==========================================================================*/
/*    Definition File for the SDR-xx protocol and Control Items              */
/*  Created 29-12-2001														 */
/*  Modified 12-09-2008		msw    added  CI_RX_AD_MODES                     */
/*	Modified 03-03-2009		msw	   added  CI_RX_OUT_FILE_UPDATEPARAMS        */
/*  Modified 07-05-2009		msw    added  CI_TX_DA_MODE                      */
/*  Modified 11-19-2009		msw    add CI_TX_PULSE_MODE,CI_GENERAL_PRODUCT_ID*/
/*  Modified 04-27-2011		msw    added new NetSDR messages                 */
/* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
#ifndef PROTOCOLDEFS_H
#define PROTOCOLDEFS_H

#define ASCP_INTERFACE_VERSION 9

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

#define CI_GENERAL_OPTIONS 0x000A
 #define CI_GENERAL_OPTIONS_REQLEN 4
 #define CI_GENERAL_OPTIONS_SETRESPLEN 10
 #define GENERAL_CI_GENERAL_OPTION1_SOUND 1		//sound enabled
 #define GENERAL_CI_GENERAL_OPTION1_REFPRESENT 2	//reflock present
 #define GENERAL_CI_GENERAL_OPTION1_DNPRESENT 4	//down converter present
 #define GENERAL_CI_GENERAL_OPTION1_UPPRESENT 8	//up converter present
 #define GENERAL_CI_GENERAL_OPTION1_X2PRESENT 0x10	//X2 Option present

  #define GENERAL_CI_GENERAL_OPTION2_CUST1 1		//custom option

  #define GENERAL_CI_GENERAL_OPTION3_MAINVAR (0x0000000F)	//main board variant
  #define GENERAL_CI_GENERAL_OPTION3_REFVAR  (0x000000F0)	//reflock variant
  #define GENERAL_CI_GENERAL_OPTION3_DNVAR   (0x00000F00)	//down converter variant
  #define GENERAL_CI_GENERAL_OPTION3_UPVAR   (0x0000F000)	//up converter variant


#define CI_GENERAL_SECURITY_CODE 0x000B
 #define CI_GENERAL_SECURITY_CODE_REQLEN 8
 #define CI_GENERAL_SECURITY_CODE_SETRESPLEN 8

/*  Receiver Specific Control Items  */
//defines for message channel ID for RX1 or RX2 used in many of the Rx control item messages
#define CI_RX_CHAN_1 0
#define CI_RX_CHAN_2 2
#define CI_RX_CHAN_ALL 0xFF
#define RX_CHAN_1_INDEX (CI_RX_CHAN_1>>1)		//index values for arrays
#define RX_CHAN_2_INDEX (CI_RX_CHAN_2>>1)


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

#define CI_RX_CHAN_SETUP 0x0019
 #define CI_RX_CHAN_SETUP_REQLEN 4
 #define CI_RX_CHAN_SETUP_SETRESPLEN 5
  #define CI_RX_CHAN_SETUP_SINGLE_1 0
  #define CI_RX_CHAN_SETUP_SINGLE_2 1
  #define CI_RX_CHAN_SETUP_SINGLE_SUM 2
  #define CI_RX_CHAN_SETUP_SINGLE_DIF 3
  #define CI_RX_CHAN_SETUP_DUAL_AD1 4
  #define CI_RX_CHAN_SETUP_DUAL_AD2 5
  #define CI_RX_CHAN_SETUP_DUAL_AD12 6

#define CI_RX_FREQUENCY 0x0020
 #define CI_RX_FREQUENCY_REQLEN 5
 #define CI_RX_FREQUENCY_SETRESPLEN 10
  #define CI_RX_FREQUENCY_NCO 0				//param 1 == channel ID
  #define CI_RX_FREQUENCY_DISPLAY 1

#define CI_RX_NCOPHASE 0x0022
 #define CI_RX_NCOPHASE_REQLEN 5
 #define CI_RX_NCOPHASE_SETRESPLEN 9		//param 1 == channel ID param 2 == 32bit phase offset

#define CI_RX_ADCGAIN 0x0023
 #define CI_RX_ADCGAIN_REQLEN 5
 #define CI_RX_ADCGAIN_SETRESPLEN 7			//param 1 == channel ID param 2 == 16bit A/D gain

#define CI_RX_RF_GAIN 0x0038
 #define CI_RX_RF_GAIN_REQLEN 5
 #define CI_RX_RF_GAIN_SETRESPLEN 6			//param 1 == channel ID

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
  #define CI_RX_RF_FILTER_BYPASS 11
  #define CI_RX_RF_FILTER_NOPASS 12

#define CI_RX_AF_GAIN 0x0048
 #define CI_RX_AF_GAIN_REQLEN 5
 #define CI_RX_AF_GAIN_SETRESPLEN 6

#define CI_RX_AD_MODES 0x008A
 #define CI_RX_AD_MODES_REQLEN 5
 #define CI_RX_AD_MODES_SETRESPLEN 6
  #define CI_AD_MODES_DITHER 0x01	//bit field defs
  #define CI_AD_MODES_PGA 0x02

#define CI_RX_DOWNCONVERT_SETUP 0x008C		//not used
 #define CI_RX_DOWNCONVERT_SETUP_REQLEN 5
 #define CI_RX_DOWNCONVERT_SETUP_SETRESPLEN 6

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
 #define CI_RX_CALIBRATION_DATA_SETRESPLEN 7	//param 1 == channel ID param 2== 16 bit DC offset

/*  Transmitter Specific Control Items  */
#define CI_TX_DA_MODE 0x012A
 #define CI_TX_DA_MODE_REQLEN 5
 #define CI_TX_DA_MODE_SETRESPLEN 6
  #define CI_DA_MODE_OFF 0x00		//mode defs
  #define CI_DA_MODE_AD 0x01
  #define CI_DA_MODE_NCOTRACK 0x02
  #define CI_DA_MODE_NOISE 0x03

#define CI_TX_CW_MSG 0x0150
 #define CI_TX_CW_MSG_REQLEN 4
 #define CI_TX_CW_MSG_SETRESPLEN 16  //param1 = 1 byte WPM, param2 = 1 byte tone/100Hz, param3 == 10 byte ascii msg

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
