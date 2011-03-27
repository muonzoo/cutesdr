////////////////////////////////////////////////////////////////////////
// ad6620.cpp: implementation of the Cad6620 class.
//
// Creates the messages required by the sdriq and sdr14 to load the
// internal AD6620 digital downconverter.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
////////////////////////////////////////////////////////////////////////

//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2010 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//==========================================================================================
#include "ad6620.h"
#include "protocoldefs.h"
#include <QDebug>

// Defines for all the AD6620 registers
#define ADR_MODECTRL 0x300			//8 bits
 #define MODECTRL_RESET (1<<0)	//pwrs up in soft reset with this bit set
 #define MODECTRL_SREAL (0<<1)
 #define MODECTRL_DREAL (1<<1)
 #define MODECTRL_SCOMPLEX (1<<2)
 #define MODECTRL_SYNCMASTER (1<<3)
#define ADR_NCOCTRL 0x301			//8 bits
 #define NCOCTRL_BYPASS (1<<0)
 #define NCOCTRL_PHZDITHER (1<<1)
 #define NCOCTRL_AMPDITHER (1<<2)
#define ADR_NCOSYNCMASK 0x302	//32 bits (write all 1's)
#define ADR_NCOFREQ 0x303
#define ADR_NCOPHZOFFSET 0x304
#define ADR_CIC2SCALE 0x305	//8 bits, range 0 to 6 each count==6dB atten
#define ADR_CIC2M 0x306		//8 bits, 0 to 15( decimation of 1 to 16 )
#define ADR_CIC5SCALE 0x307	//8 bits, range 0 to 20 each count == 6dB
#define ADR_CIC5M 0x308		//0 to 31(decimation 1 to 32)
#define ADR_RCFCTRL 0x309	// 8 bits
#define ADR_RCFM 0x30A	//8 bits 0 to 31 (decimation 1 to 32)
#define ADR_RCFOFFSET 0x30B	//8 bit filter coef offset for RCF filter
#define ADR_TAPS 0x30C		//8 bits 0 to 255(number of RCF taps 1 to 256)
#define ADR_RESERVED 0x30D	//must be zero



//These tables are used to calculate the decimation stage gains
// which are a function of the decimation rate of each stage
const int CIC2_SCALE_TBL[17] = {
	0,
	0,0,2,2,	//1-16
	3,4,4,4,
	5,5,5,6,
	6,6,6,6
};
const int CIC5_SCALE_TBL[33] = {
	0,
	0,  0, 3, 5,	//1-32
	7,  8,10,10,
	11,12,13,13,
	14,15,15,15,
	16,16,17,17,
	17,18,18,18,
	19,19,19,20,
	20,20,20,20
};

//  Parameters for the 6620 filter

//////////////////////  5 KHz  //////////////////////////////////////
// Pass bandwidth	.0025	.001 dB
// Stop bandwidth	.004069	-90 dB
const int FIL5KHZ_CIC2RATE = 16;
const int FIL5KHZ_CIC5RATE = 32;
const int FIL5KHZ_RCFRATE = 16;
const int FIL5KHZ_USEABLEBW = 5000;
const int FIL5KHZ_TAPS = 256;
const int FIL5KHZ_COEF[FIL5KHZ_TAPS] =
{
 672, -502, 442, -232, 228, -117, 112, -89, 32, -103, -38, -138, -105, -184, -166, -229,
 -217, -263, -251, -280, -260, -270, -238, -229, -183, -155, -95, -50, 22, 80, 159, 224,
 303, 367, 439, 493, 549, 583, 615, 622, 621, 595, 559, 496, 423, 325, 217, 89,
 -46, -195, -344, -501, -650, -798, -930, -1052, -1149, -1226, -1272, -1291, -1272, -1222, -1132, -1009,
 -847, -654, -428, -177, 98, 387, 687, 988, 1285, 1565, 1826, 2053, 2244, 2385, 2475, 2504,
 2469, 2365, 2193, 1948, 1636, 1257, 819, 326, -210, -782, -1374, -1975, -2569, -3141, -3674, -4153,
 -4559, -4879, -5094, -5193, -5162, -4991, -4670, -4194, -3558, -2763, -1810, -704, 547, 1931, 3436, 5043,
 6735, 8490, 10285, 12097, 13900, 15668, 17376, 18999, 20512, 21891, 23116, 24168, 25031, 25690, 26135, 26359,
 26359, 26135, 25690, 25031, 24168, 23116, 21891, 20512, 18999, 17376, 15668, 13900, 12097, 10285, 8490, 6735,
 5043, 3436, 1931, 547, -704, -1810, -2763, -3558, -4194, -4670, -4991, -5162, -5193, -5094, -4879, -4559,
 -4153, -3674, -3141, -2569, -1975, -1374, -782, -210, 326, 819, 1257, 1636, 1948, 2193, 2365, 2469,
 2504, 2475, 2385, 2244, 2053, 1826, 1565, 1285, 988, 687, 387, 98, -177, -428, -654, -847,
 -1009, -1132, -1222, -1272, -1291, -1272, -1226, -1149, -1052, -930, -798, -650, -501, -344, -195, -46,
 89, 217, 325, 423, 496, 559, 595, 621, 622, 615, 583, 549, 493, 439, 367, 303,
 224, 159, 80, 22, -50, -95, -155, -183, -229, -238, -270, -260, -280, -251, -263, -217,
 -229, -166, -184, -105, -138, -38, -103, 32, -89, 112, -117, 228, -232, 442, -502, 672
};

//////////////////////  10 KHz  //////////////////////////////////////
// Pass bandwidth	.005	.001 dB
// Stop bandwidth	.008138	-90 dB
const int FIL10KHZ_CIC2RATE = 8;
const int FIL10KHZ_CIC5RATE = 32;
const int FIL10KHZ_RCFRATE = 16;
const int FIL10KHZ_USEABLEBW = 10000;
const int FIL10KHZ_TAPS = 256;
const int FIL10KHZ_COEF[FIL10KHZ_TAPS] =
{
 672, -502, 442, -232, 228, -117, 112, -89, 32, -103, -38, -138, -105, -184, -166, -229,
 -217, -263, -251, -280, -260, -270, -238, -229, -183, -155, -95, -50, 22, 80, 159, 224,
 303, 367, 439, 493, 549, 583, 615, 622, 621, 595, 559, 496, 423, 325, 217, 89,
 -46, -195, -344, -501, -650, -798, -930, -1052, -1149, -1226, -1272, -1291, -1272, -1222, -1132, -1009,
 -847, -654, -428, -177, 98, 387, 687, 988, 1285, 1565, 1826, 2053, 2244, 2385, 2475, 2504,
 2469, 2365, 2193, 1948, 1636, 1257, 819, 326, -210, -782, -1374, -1975, -2569, -3141, -3674, -4153,
 -4559, -4879, -5094, -5193, -5162, -4991, -4670, -4194, -3558, -2763, -1810, -704, 547, 1931, 3436, 5043,
 6735, 8490, 10285, 12097, 13900, 15668, 17376, 18999, 20512, 21891, 23116, 24168, 25031, 25690, 26135, 26359,
 26359, 26135, 25690, 25031, 24168, 23116, 21891, 20512, 18999, 17376, 15668, 13900, 12097, 10285, 8490, 6735,
 5043, 3436, 1931, 547, -704, -1810, -2763, -3558, -4194, -4670, -4991, -5162, -5193, -5094, -4879, -4559,
 -4153, -3674, -3141, -2569, -1975, -1374, -782, -210, 326, 819, 1257, 1636, 1948, 2193, 2365, 2469,
 2504, 2475, 2385, 2244, 2053, 1826, 1565, 1285, 988, 687, 387, 98, -177, -428, -654, -847,
 -1009, -1132, -1222, -1272, -1291, -1272, -1226, -1149, -1052, -930, -798, -650, -501, -344, -195, -46,
 89, 217, 325, 423, 496, 559, 595, 621, 622, 615, 583, 549, 493, 439, 367, 303,
 224, 159, 80, 22, -50, -95, -155, -183, -229, -238, -270, -260, -280, -251, -263, -217,
 -229, -166, -184, -105, -138, -38, -103, 32, -89, 112, -117, 228, -232, 442, -502, 672,
};

//////////////////////  25 KHz  //////////////////////////////////////
// Pass bandwidth	.0125	.001 dB
// Stop bandwidth	.018896	-90 dB
const int FIL25KHZ_CIC2RATE = 7;
const int FIL25KHZ_CIC5RATE = 21;
const int FIL25KHZ_RCFRATE = 12;
const int FIL25KHZ_USEABLEBW = 25000;
const int FIL25KHZ_TAPS = 256;
const int FIL25KHZ_COEF[FIL25KHZ_TAPS] =
{
 -81, 279, -99, 176, -6, 131, 37, 105, 44, 75, 25, 31, -17, -26, -72, -89,
 -130, -147, -178, -185, -199, -190, -183, -153, -122, -72, -21, 44, 108, 179, 242, 305,
 353, 392, 411, 414, 392, 352, 286, 202, 97, -20, -149, -280, -410, -529, -633, -712,
 -763, -778, -757, -693, -590, -448, -272, -67, 156, 391, 623, 844, 1037, 1195, 1303, 1355,
 1342, 1261, 1110, 892, 612, 281, -90, -483, -881, -1265, -1614, -1908, -2128, -2257, -2283, -2197,
 -1994, -1676, -1250, -730, -134, 514, 1185, 1849, 2471, 3018, 3457, 3757, 3893, 3845, 3600, 3155,
 2513, 1689, 709, -395, -1579, -2795, -3986, -5093, -6055, -6810, -7302, -7479, -7296, -6720, -5728, -4311,
 -2476, -242, 2356, 5268, 8433, 11778, 15221, 18674, 22045, 25242, 28177, 30765, 32933, 34617, 35768, 36352,
 36352, 35768, 34617, 32933, 30765, 28177, 25242, 22045, 18674, 15221, 11778, 8433, 5268, 2356, -242, -2476,
 -4311, -5728, -6720, -7296, -7479, -7302, -6810, -6055, -5093, -3986, -2795, -1579, -395, 709, 1689, 2513,
 3155, 3600, 3845, 3893, 3757, 3457, 3018, 2471, 1849, 1185, 514, -134, -730, -1250, -1676, -1994,
 -2197, -2283, -2257, -2128, -1908, -1614, -1265, -881, -483, -90, 281, 612, 892, 1110, 1261, 1342,
 1355, 1303, 1195, 1037, 844, 623, 391, 156, -67, -272, -448, -590, -693, -757, -778, -763,
 -712, -633, -529, -410, -280, -149, -20, 97, 202, 286, 352, 392, 414, 411, 392, 353,
 305, 242, 179, 108, 44, -21, -72, -122, -153, -183, -190, -199, -185, -178, -147, -130,
 -89, -72, -26, -17, 31, 25, 75, 44, 105, 37, 131, -6, 176, -99, 279, -81,
};

//////////////////////  50 KHz  //////////////////////////////////////
// Pass bandwidth	.025	.001 dB
// Stop bandwidth	.037792	-90 dB
const int FIL50KHZ_CIC2RATE = 8;
const int FIL50KHZ_CIC5RATE = 30;
const int FIL50KHZ_RCFRATE = 5;
const int FIL50KHZ_USEABLEBW = 50000;
const int FIL50KHZ_TAPS = 256;
const int FIL50KHZ_COEF[FIL50KHZ_TAPS] =
{
-115, 572, 286, 894, 855, 1164, 1031, 971, 601, 278,-156,-424,-607,-546,-363,-34 ,
 269, 511, 560, 445, 158,-174,-474,-610,-558,-300, 62, 431, 663, 689, 471, 88 ,
-355,-694,-821,-668,-282, 228, 686, 938, 880, 519,-44,-622,-1023,-1093,-794,-202 ,
 491, 1058, 1291, 1097, 511,-282,-1025,-1457,-1414,-880,-14, 908, 1568, 1727, 1299, 401 ,
-689,-1602,-2017,-1759,-880, 353, 1536, 2258, 2242, 1452, 113,-1343,-2423,-2728,-2110,-724 ,
 997, 2479, 3195, 2849, 1494,-465,-2390,-3612,-3661,-2442,-292, 2109, 3946, 4541, 3597, 1330 ,
-1574,-4154,-5488,-5011,-2740, 688, 4177, 6519, 6787, 4690, 724,-3918,-7684,-9155,-7544,-3033 ,
 3174, 9131, 12703, 12256, 7260,-1366,-11320,-19399,-22305,-17564,-4306, 16319, 41233, 66039, 85987, 97095 ,
 97095, 85987, 66039, 41233, 16319,-4306,-17564,-22305,-19399,-11320,-1366, 7260, 12256, 12703, 9131, 3174 ,
-3033,-7544,-9155,-7684,-3918, 724, 4690, 6787, 6519, 4177, 688,-2740,-5011,-5488,-4154,-1574 ,
 1330, 3597, 4541, 3946, 2109,-292,-2442,-3661,-3612,-2390,-465, 1494, 2849, 3195, 2479, 997 ,
-724,-2110,-2728,-2423,-1343, 113, 1452, 2242, 2258, 1536, 353,-880,-1759,-2017,-1602,-689 ,
 401, 1299, 1727, 1568, 908,-14,-880,-1414,-1457,-1025,-282, 511, 1097, 1291, 1058, 491 ,
-202,-794,-1093,-1023,-622,-44, 519, 880, 938, 686, 228,-282,-668,-821,-694,-355 ,
 88, 471, 689, 663, 431, 62,-300,-558,-610,-474,-174, 158, 445, 560, 511, 269 ,
-34,-363,-546,-607,-424,-156, 278, 601, 971, 1031, 1164, 855, 894, 286, 572,-115
};

//////////////////////  100 KHz  //////////////////////////////////////
// Pass bandwidth	.0125	.001 dB
// Stop bandwidth	.018896	-90 dB
const int FIL100KHZ_CIC2RATE = 5;
const int FIL100KHZ_CIC5RATE = 30;
const int FIL100KHZ_RCFRATE = 4;
const int FIL100KHZ_USEABLEBW = 100000;
const int FIL100KHZ_TAPS = 256;
const int FIL100KHZ_COEF[FIL100KHZ_TAPS] =
{
 111, 108, 197, 150, 69,-150,-405,-679,-847,-866,-689,-380,-23, 244, 337, 222,
-27,-290,-429,-371,-131, 176, 402, 426, 227,-107,-406,-515,-361,-8, 374, 586 ,
 504, 149,-309,-639,-655,-326, 199, 657, 801, 531,-39,-630,-929,-758,-173, 545 ,
 1024, 994, 435,-394,-1071,-1227,-742, 170, 1054, 1438, 1085, 131,-956,-1608,-1449,-509, 765 ,
 1716, 1818, 959,-467,-1739,-2170,-1474, 53, 1653, 2479, 2038, 481,-1436,-2717,-2634,-1140 ,
 1064, 2851, 3238, 1922,-515,-2847,-3823,-2822,-237, 2665, 4354, 3835, 1218,-2256,-4793,-4956 ,
-2465, 1563, 5093, 6183, 4030,-501,-5191,-7525,-6007,-1062, 5000, 9018, 8574, 3371,-4365,-10755 ,
-12118,-6953, 2955, 12982, 17627, 13228, 184,-16444,-28453,-27679,-9595, 24220, 65916, 103759, 126224, 126224 ,
 103759, 65916, 24220,-9595,-27679,-28453,-16444, 184, 13228, 17627, 12982, 2955,-6953,-12118,-10755,-4365 ,
 3371, 8574, 9018, 5000,-1062,-6007,-7525,-5191,-501, 4030, 6183, 5093, 1563,-2465,-4956,-4793 ,
-2256, 1218, 3835, 4354, 2665,-237,-2822,-3823,-2847,-515, 1922, 3238, 2851, 1064,-1140,-2634 ,
-2717,-1436, 481, 2038, 2479, 1653, 53,-1474,-2170,-1739,-467, 959, 1818, 1716, 765,-509 ,
-1449,-1608,-956, 131, 1085, 1438, 1054, 170,-742,-1227,-1071,-394, 435, 994, 1024, 545 ,
-173,-758,-929,-630,-39, 531, 801, 657, 199,-326,-655,-639,-309, 149, 504, 586 ,
 374,-8,-361,-515,-406,-107, 227, 426, 402, 176,-131,-371,-429,-290,-27, 222 ,
 337, 244,-23,-380,-689,-866,-847,-679,-405,-150, 69, 150, 197, 108, 111
};

//////////////////////  150 KHz  //////////////////////////////////////
const int FIL150KHZ_CIC2RATE = 5;
const int FIL150KHZ_CIC5RATE = 28;
const int FIL150KHZ_RCFRATE = 3;
const int FIL150KHZ_USEABLEBW = 150000;
const int FIL150KHZ_TAPS = 256;
const int FIL150KHZ_COEF[FIL150KHZ_TAPS] =
{
 80,-1272,-1501,-2506,-1995,-1237, 225, 985, 1065, 218,-585,-897,-355, 403, 811, 432 ,
-300,-777,-504, 221, 771, 585,-145,-776,-676, 61, 783, 777, 36,-784,-884,-149 ,
 774, 996, 279,-749,-1108,-427, 706, 1217, 591,-641,-1321,-772, 553, 1415, 968,-437 ,
-1496,-1177, 293, 1560, 1398,-119,-1602,-1628,-87, 1620, 1863, 325,-1608,-2101,-598, 1562 ,
 2339, 905,-1478,-2571,-1247, 1350, 2793, 1624,-1175,-3001,-2036, 947, 3190, 2484,-660,-3353 ,
-2967, 307, 3484, 3486, 118,-3575,-4042,-626, 3619, 4637, 1227,-3606,-5273,-1936, 3523, 5956 ,
 2775,-3354,-6694,-3770, 3080, 7501, 4965,-2669,-8398,-6426, 2076, 9422, 8256,-1227,-10635,-10642 ,
-11, 12151, 13929, 1901,-14192,-18856,-5052, 17262, 27312, 11251,-22720,-45866,-28628 ,
 35237, 119172, 178193, 178193, 119172, 35237,-28628,-45866,-22720, 11251, 27312, 17262,-5052,-18856,-14192, 1901 ,
 13929, 12151,-11,-10642,-10635,-1227, 8256, 9422, 2076,-6426,-8398,-2669, 4965, 7501, 3080,-3770 ,
-6694,-3354, 2775, 5956, 3523,-1936,-5273,-3606, 1227, 4637, 3619,-626,-4042,-3575, 118, 3486 ,
 3484, 307,-2967,-3353,-660, 2484, 3190, 947,-2036,-3001,-1175, 1624, 2793, 1350,-1247,-2571 ,
-1478, 905, 2339, 1562,-598,-2101,-1608, 325, 1863, 1620,-87,-1628,-1602,-119, 1398, 1560 ,
 293,-1177,-1496,-437, 968, 1415, 553,-772,-1321,-641, 591, 1217, 706,-427,-1108,-749 ,
 279, 996, 774,-149,-884,-784, 36, 777, 783, 61,-676,-776,-145, 585, 771, 221 ,
-504,-777,-300, 432, 811, 403,-355,-897,-585, 218, 1065, 985, 225,-1237,-1995,-2506 ,
-1501,-1272, 80
};

//////////////////////  190 KHz  //////////////////////////////////////
const int FIL190KHZ_CIC2RATE = 10;
const int FIL190KHZ_CIC5RATE = 17;
const int FIL190KHZ_RCFRATE = 2;
const int FIL190KHZ_USEABLEBW = 190000;
const int FIL190KHZ_TAPS = 256;
const int FIL190KHZ_COEF[FIL190KHZ_TAPS] =
{
 3447 ,-2833 ,-1187 ,-2836 , 784 ,-262 ,-551 ,-1155 , 655 , 341 ,-484 ,-824 , 627 , 573 ,-493 ,-814 ,
 597 , 739 ,-495 ,-905 , 556 , 899 ,-472 ,-1036 , 498 , 1065 ,-419 ,-1189 , 417 , 1238 ,-335 ,-1355 ,
 308 , 1416 ,-219 ,-1527 , 167 , 1595 ,-66 ,-1700 ,-8 , 1771 , 124 ,-1870 ,-221 , 1939 , 355 ,-2031 ,
-475 , 2096 , 628 ,-2177 ,-772 , 2235 , 946 ,-2304 ,-1114 , 2350 , 1310 ,-2404 ,-1504 , 2435 , 1723 ,-2470 ,
-1944 , 2482 , 2188 ,-2494 ,-2436 , 2483 , 2707 ,-2468 ,-2984 , 2429 , 3285 ,-2381 ,-3593 , 2308 , 3925 ,-2221 ,
-4268 , 2107 , 4635 ,-1974 ,-5017 , 1808 , 5426 ,-1618 ,-5854 , 1390 , 6314 ,-1128 ,-6799 , 819 , 7322 ,-465 ,
-7882 , 50 , 8492 , 429 ,-9154 ,-993 , 9888 , 1653 ,-10702 ,-2438 , 11626 , 3378 ,-12684 ,-4524 , 13926 , 5947 ,
-15415 ,-7763 , 17256 , 10157 ,-19614 ,-13468 , 22786 , 18349 ,-27322 ,-26280 , 34392 , 41334 ,-46742 ,-79378 , 69315 , 263870 ,
 263870 , 69315 ,-79378 ,-46742 , 41334 , 34392 ,-26280 ,-27322 , 18349 , 22786 ,-13468 ,-19614 , 10157 , 17256 ,-7763 ,-15415 ,
 5947 , 13926 ,-4524 ,-12684 , 3378 , 11626 ,-2438 ,-10702 , 1653 , 9888 ,-993 ,-9154 , 429 , 8492 , 50 ,-7882 ,
-465 , 7322 , 819 ,-6799 ,-1128 , 6314 , 1390 ,-5854 ,-1618 , 5426 , 1808 ,-5017 ,-1974 , 4635 , 2107 ,-4268 ,
-2221 , 3925 , 2308 ,-3593 ,-2381 , 3285 , 2429 ,-2984 ,-2468 , 2707 , 2483 ,-2436 ,-2494 , 2188 , 2482 ,-1944 ,
-2470 , 1723 , 2435 ,-1504 ,-2404 , 1310 , 2350 ,-1114 ,-2304 , 946 , 2235 ,-772 ,-2177 , 628 , 2096 ,-475 ,
-2031 , 355 , 1939 ,-221 ,-1870 , 124 , 1771 ,-8 ,-1700 ,-66 , 1595 , 167 ,-1527 ,-219 , 1416 , 308 ,
-1355 ,-335 , 1238 , 417 ,-1189 ,-419 , 1065 , 498 ,-1036 ,-472 , 899 , 556 ,-905 ,-495 , 739 , 597 ,
-814 ,-493 , 573 , 627 ,-824 ,-484 , 341 , 655 ,-1155 ,-551 ,-262 , 784 ,-2836 ,-1187 ,-2833 , 3447
};

//////////////////////  250 KHz  //////////////////////////////////////
const int FIL250KHZ_CIC2RATE = 5;
const int FIL250KHZ_CIC5RATE = 11;
const int FIL250KHZ_RCFRATE = 4;
const int FIL250KHZ_USEABLEBW = 250000;
const int FIL250KHZ_TAPS = 220;
const int FIL250KHZ_COEF[FIL250KHZ_TAPS] =
{
 12, -10, -7, -31, -35, -41, -25, -3, 29, 48, 50, 23, -20, -64, -83, -63,
 -6, 66, 118, 119, 59, -44, -142, -185, -139, -14, 140, 247, 244, 115, -93, -286,
 -360, -261, -12, 278, 466, 442, 188, -197, -532, -639, -434, 20, 520, 817, 734, 266,
 -394, -931, -1057, -659, 119, 925, 1351, 1138, 322, -746, -1551, -1655, -929, 344, 1577, 2139,
 1674, 312, -1348, -2495, -2494, -1228, 789, 2612, 3296, 2378, 160, -2366, -3951, -3694, -1535, 1630,
 4301, 5070, 3343, -275, -4155, -6356, -5566, -1839, 3280, 7353, 8174, 4900, -1348, -7792, -11163, -9262,
 -2215, 7235, 14660, 15836, 8827, -4646, -19277, -27834, -23877, -4514, 27947, 66209, 100118, 120005, 120005, 100118,
 66209, 27947, -4514, -23877, -27834, -19277, -4646, 8827, 15836, 14660, 7235, -2215, -9262, -11163, -7792, -1348,
 4900, 8174, 7353, 3280, -1839, -5566, -6356, -4155, -275, 3343, 5070, 4301, 1630, -1535, -3694, -3951,
 -2366, 160, 2378, 3296, 2612, 789, -1228, -2494, -2495, -1348, 312, 1674, 2139, 1577, 344, -929,
 -1655, -1551, -746, 322, 1138, 1351, 925, 119, -659, -1057, -931, -394, 266, 734, 817, 520,
 20, -434, -639, -532, -197, 188, 442, 466, 278, -12, -261, -360, -286, -93, 115, 244,
 247, 140, -14, -139, -185, -142, -44, 59, 119, 118, 66, -6, -63, -83, -64, -20,
 23, 50, 48, 29, -3, -25, -41, -35, -31, -7, -10, 12,
};

//////////////////////  500 KHz  //////////////////////////////////////
const int FIL500KHZ_CIC2RATE = 2;
const int FIL500KHZ_CIC5RATE = 29;
const int FIL500KHZ_RCFRATE = 2;
const int FIL500KHZ_USEABLEBW = 500000;
const int FIL500KHZ_TAPS = 116;
const int FIL500KHZ_COEF[FIL500KHZ_TAPS] =
{
 -124, -118, 211, 746, 770, 52, -573, -174, 643, 459, -609, -758, 494, 1096, -247, -1419,
 -154, 1675, 710, -1801, -1407, 1732, 2204, -1405, -3037, 771, 3818, 202, -4440, -1516, 4779, 3142,
 -4706, -5014, 4091, 7022, -2814, -9019, 768, 10812, 2133, -12169, -5958, 12807, 10771, -12368, -16660, 10361,
 23800, -5990, -32604, -2353, 44095, 18918, -60919, -60113, 87967, 253562, 253562, 87967, -60113, -60919, 18918, 44095,
 -2353, -32604, -5990, 23800, 10361, -16660, -12368, 10771, 12807, -5958, -12169, 2133, 10812, 768, -9019, -2814,
 7022, 4091, -5014, -4706, 3142, 4779, -1516, -4440, 202, 3818, 771, -3037, -1405, 2204, 1732, -1407,
 -1801, 710, 1675, -154, -1419, -247, 1096, 494, -758, -609, 459, 643, -174, -573, 52, 770,
 746, 211, -118, -124
};

//////////////////////  1000 KHz  //////////////////////////////////////
const int FIL1000KHZ_CIC2RATE = 2;
const int FIL1000KHZ_CIC5RATE = 13;
const int FIL1000KHZ_RCFRATE = 2;
const int FIL1000KHZ_USEABLEBW = 1000000;
const int FIL1000KHZ_TAPS = 52;
const int FIL1000KHZ_COEF[FIL1000KHZ_TAPS] =
{
 155, -946, -3137, -4394, -1611, 3281, 3993, -1821, -6300, -1205, 7795, 5988, -7134, -11977, 3025, 17757,
 5598, -21134, -19379, 18859, 38850, -5161, -65511, -37716, 102796, 241472, 241472, 102796, -37716, -65511, -5161, 38850,
 18859, -19379, -21134, 5598, 17757, 3025, -11977, -7134, 5988, 7795, -1205, -6300, -1821, 3993, 3281, -1611,
 -4394, -3137, -946, 155
};

//////////////////////  1500 KHz  //////////////////////////////////////
const int FIL1500KHZ_CIC2RATE = 2;
const int FIL1500KHZ_CIC5RATE = 8;
const int FIL1500KHZ_RCFRATE = 2;
const int FIL1500KHZ_USEABLEBW = 1500000;
const int FIL1500KHZ_TAPS = 32;
const int FIL1500KHZ_COEF[FIL1500KHZ_TAPS] =
{
 -2838, -7081, -7827, 591, 11843, 11249, -5268, -17456, -2992, 26223, 27127, -17480, -57306, -18005, 104558, 216808,
 216808, 104558, -18005, -57306, -17480, 27127, 26223, -2992, -17456, -5268, 11249, 11843, 591, -7827, -7081, -2838,
};

//////////////////////  2000 KHz  ///////////////////////////////////
const int FIL2000KHZ_CIC2RATE = 2;
const int FIL2000KHZ_CIC5RATE = 5;
const int FIL2000KHZ_RCFRATE = 2;
const int FIL2000KHZ_USEABLEBW = 2000000;
const int FIL2000KHZ_TAPS = 20;
const int FIL2000KHZ_COEF[FIL2000KHZ_TAPS] =
{
 1045 , 8235 , 18078 , 19813 , 148 ,-31087, -38801 , 9369 ,
 100534,  174811 , 174811 , 100534 , 9369 ,-38801 ,-31087 , 148 ,
 19813,  18078 , 8235 , 1045
};

//////////////////////  4000 KHz  ///////////////////////////////////
const int FIL4000KHZ_CIC2RATE = 2;
const int FIL4000KHZ_CIC5RATE = 4;
const int FIL4000KHZ_RCFRATE = 2;
const int FIL4000KHZ_USEABLEBW = 4000000;
const int FIL4000KHZ_TAPS = 16;
const int FIL4000KHZ_COEF[FIL4000KHZ_TAPS] =
{
 -30,-9364, 2158, 31085,-12771,-83607, 42342, 292332,
 292332, 42342,-83607,-12771, 31085, 2158,-9364,-30
};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Cad6620::Cad6620()
{
	m_NCOphzDither = true;
	m_NCOampDither = true;
	m_CIC2Rate = FIL190KHZ_CIC2RATE;
	m_CIC5Rate = FIL190KHZ_CIC5RATE;
	m_RCFRate = FIL190KHZ_RCFRATE;
	m_UseableBW = FIL190KHZ_USEABLEBW;
	for(int i=0; i<FIL190KHZ_TAPS; i++)
		m_FIRcoef[i] = FIL190KHZ_COEF[i];
	m_RCFTaps = FIL190KHZ_TAPS;
	m_TotalDecimationRate = m_CIC2Rate*m_CIC5Rate*m_RCFRate;
	m_CIC2Scale = CIC2_SCALE_TBL[m_CIC2Rate];
	m_CIC5Scale = CIC5_SCALE_TBL[m_CIC5Rate];
}

Cad6620::~Cad6620()
{

}

//////////////////////////////////////////////////////////////////////
//  Called to load the AD6620 parameters of the SDR-14/IQ.
//////////////////////////////////////////////////////////////////////
void Cad6620::CreateLoad6620Msgs(int filter)
{
int i;
unsigned int tmp = 0;
	m_BufIndx = 0;
	switch(filter)
	{
		case BWKHZ_5:
			m_CIC2Rate = FIL5KHZ_CIC2RATE;
			m_CIC5Rate = FIL5KHZ_CIC5RATE;
			m_RCFRate = FIL5KHZ_RCFRATE;
			m_UseableBW = FIL5KHZ_USEABLEBW;
			for(i=0; i<FIL5KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL5KHZ_COEF[i];
			break;
		case BWKHZ_10:
			m_CIC2Rate = FIL10KHZ_CIC2RATE;
			m_CIC5Rate = FIL10KHZ_CIC5RATE;
			m_RCFRate = FIL10KHZ_RCFRATE;
			m_UseableBW = FIL10KHZ_USEABLEBW;
			for(i=0; i<FIL10KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL10KHZ_COEF[i];
			break;
		case BWKHZ_25:
			m_CIC2Rate = FIL25KHZ_CIC2RATE;
			m_CIC5Rate = FIL25KHZ_CIC5RATE;
			m_RCFRate = FIL25KHZ_RCFRATE;
			m_UseableBW = FIL25KHZ_USEABLEBW;
			for(i=0; i<FIL25KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL25KHZ_COEF[i];
			break;
		case BWKHZ_50:
			m_CIC2Rate = FIL50KHZ_CIC2RATE;
			m_CIC5Rate = FIL50KHZ_CIC5RATE;
			m_RCFRate = FIL50KHZ_RCFRATE;
			m_UseableBW = FIL50KHZ_USEABLEBW;
			for(i=0; i<FIL50KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL50KHZ_COEF[i];
			break;
		case BWKHZ_100:
			m_CIC2Rate = FIL100KHZ_CIC2RATE;
			m_CIC5Rate = FIL100KHZ_CIC5RATE;
			m_RCFRate = FIL100KHZ_RCFRATE;
			m_UseableBW = FIL100KHZ_USEABLEBW;
			for(i=0; i<FIL100KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL100KHZ_COEF[i];
			break;
		case BWKHZ_150:
			m_CIC2Rate = FIL150KHZ_CIC2RATE;
			m_CIC5Rate = FIL150KHZ_CIC5RATE;
			m_RCFRate = FIL150KHZ_RCFRATE;
			m_UseableBW = FIL150KHZ_USEABLEBW;
			for(i=0; i<FIL150KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL150KHZ_COEF[i];
			break;
		case BWKHZ_190:
			m_CIC2Rate = FIL190KHZ_CIC2RATE;
			m_CIC5Rate = FIL190KHZ_CIC5RATE;
			m_RCFRate = FIL190KHZ_RCFRATE;
			m_UseableBW = FIL190KHZ_USEABLEBW;
			for(i=0; i<FIL190KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL190KHZ_COEF[i];
			break;
		case BWKHZ_250:
			m_CIC2Rate = FIL250KHZ_CIC2RATE;
			m_CIC5Rate = FIL250KHZ_CIC5RATE;
			m_RCFRate = FIL250KHZ_RCFRATE;
			m_UseableBW = FIL250KHZ_USEABLEBW;
			for(i=0; i<FIL250KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL250KHZ_COEF[i];
			break;
		case BWKHZ_500:
			m_CIC2Rate = FIL500KHZ_CIC2RATE;
			m_CIC5Rate = FIL500KHZ_CIC5RATE;
			m_RCFRate = FIL500KHZ_RCFRATE;
			m_UseableBW = FIL500KHZ_USEABLEBW;
			for(i=0; i<FIL500KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL500KHZ_COEF[i];
			break;
		case BWKHZ_1000:
			m_CIC2Rate = FIL1000KHZ_CIC2RATE;
			m_CIC5Rate = FIL1000KHZ_CIC5RATE;
			m_RCFRate = FIL1000KHZ_RCFRATE;
			m_UseableBW = FIL1000KHZ_USEABLEBW;
			for(i=0; i<FIL1000KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL1000KHZ_COEF[i];
			break;
		case BWKHZ_1500:
			m_CIC2Rate = FIL1500KHZ_CIC2RATE;
			m_CIC5Rate = FIL1500KHZ_CIC5RATE;
			m_RCFRate = FIL1500KHZ_RCFRATE;
			m_UseableBW = FIL1500KHZ_USEABLEBW;
			for(i=0; i<FIL1500KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL1500KHZ_COEF[i];
			break;
		case BWKHZ_2000:
			m_CIC2Rate = FIL2000KHZ_CIC2RATE;
			m_CIC5Rate = FIL2000KHZ_CIC5RATE;
			m_RCFRate = FIL2000KHZ_RCFRATE;
			m_UseableBW = FIL2000KHZ_USEABLEBW;
			for(i=0; i<FIL2000KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL2000KHZ_COEF[i];
			break;
		case BWKHZ_4000:
			m_CIC2Rate = FIL4000KHZ_CIC2RATE;
			m_CIC5Rate = FIL4000KHZ_CIC5RATE;
			m_RCFRate = FIL4000KHZ_RCFRATE;
			m_UseableBW = FIL4000KHZ_USEABLEBW;
			for(i=0; i<FIL4000KHZ_TAPS; i++)
				m_FIRcoef[i] = FIL4000KHZ_COEF[i];
			break;
		default:
			break;
	}

	//create all the msgs needed to load the AD6620 registers in the radio
	//The messages are placed in m_MsgBuffer
	PutInMsgBuffer(ADR_MODECTRL, MODECTRL_SREAL|MODECTRL_RESET|MODECTRL_SYNCMASTER);
	if(m_NCOampDither)
		tmp |= NCOCTRL_AMPDITHER;
	if(m_NCOphzDither)
		tmp |= NCOCTRL_PHZDITHER;
	PutInMsgBuffer(ADR_NCOCTRL, tmp);
	PutInMsgBuffer(ADR_CIC2SCALE, m_CIC2Scale);
	PutInMsgBuffer(ADR_CIC2M, m_CIC2Rate-1);
	PutInMsgBuffer(ADR_CIC5SCALE, m_CIC5Scale);
	PutInMsgBuffer(ADR_CIC5M, m_CIC5Rate-1);

	PutInMsgBuffer(ADR_RCFCTRL, 4);	//this is IFGain
	PutInMsgBuffer(ADR_RCFM, m_RCFRate-1);
	PutInMsgBuffer(ADR_RCFOFFSET, 0);
	PutInMsgBuffer(ADR_TAPS, m_RCFTaps-1);
	for(int i=0; i<m_RCFTaps; i++)
	{
		PutInMsgBuffer(i, m_FIRcoef[i]);
	}
	PutInMsgBuffer(ADR_MODECTRL, MODECTRL_SREAL|MODECTRL_SYNCMASTER);
	m_BufLength = m_BufIndx;
	m_BufIndx = 0;
}


/////////////////////////////////////////////////////////////////////////////////////
//Called to put register data into the message buffer m_MsgBuffer
/////////////////////////////////////////////////////////////////////////////////////
void Cad6620::PutInMsgBuffer(int adr, quint32 data)
{
	m_MsgBuffer[m_BufIndx].adr = adr;
	m_MsgBuffer[m_BufIndx].data = data;
	m_MsgBuffer[m_BufIndx++].datah = 0;
	if(m_BufIndx >= MAX_MSGS)
		m_BufIndx = 0;
}

/////////////////////////////////////////////////////////////////////////////////////
//Called to get the next formatted message from the messge buffer to send directly
// to the radio for loading the AD6620 tegisters
// returns false when end of all msgs is reached and load is done
/////////////////////////////////////////////////////////////////////////////////////
bool Cad6620::GetNext6620Msg(CAscpMsg &pAscpMsg)
{
	if(m_BufIndx < m_BufLength)
	{
		pAscpMsg.InitTxMsg(TYPE_HOST_DATA_ITEM1);
		pAscpMsg.AddParm16(m_MsgBuffer[m_BufIndx].adr);
		pAscpMsg.AddParm32(m_MsgBuffer[m_BufIndx].data);
		pAscpMsg.AddParm8(m_MsgBuffer[m_BufIndx++].datah);
		return true;
	}
	else
		return false;
}


