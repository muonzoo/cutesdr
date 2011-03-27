//////////////////////////////////////////////////////////////////////
// ascpmsg.h: interface/implementation for the CAscpMsg class.
//
// Helper Class to aid in creating and decoding ASCP format msgs
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef ASCPMSG_H
#define ASCPMSG_H
#include <QtGlobal>
#include "interface/protocoldefs.h"

#define MAX_ASCPMSG_LENGTH (8192+2)

class CAscpMsg
{
public:
	CAscpMsg(){Length=0;}
	//  For creating msgs to Tx
	// Call InitTxMsg with msg type to start creation
	// Add CItem and parameters, msg length is automatically created in hdr
	void InitTxMsg(quint8 type){Buf16[0] = type<<8; Buf16[0]+=2;}
	void AddCItem(quint16 item){Buf16[1] = item; Buf16[0]+=2;}
	void AddParm8(quint8 parm8){Buf8[ Buf16[0]&LENGTH_MASK ] = parm8; Buf16[0]++;}
	void AddParm16(quint16 parm16){Buf8[ (Buf16[0]&LENGTH_MASK) ] = parm16&0xFF; Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm16>>8)&0xFF;Buf16[0]++;}
	void AddParm32(quint32 parm32){Buf8[ (Buf16[0]&LENGTH_MASK) ] = parm32&0xFF; Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm32>>8)&0xFF;Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm32>>16)&0xFF;Buf16[0]++;
					Buf8[ (Buf16[0]&LENGTH_MASK) ] = (parm32>>24)&0xFF;Buf16[0]++;}

	// For reading msgs
	// Put raw received bytes in Buf8[]
	// Call InitRxMsg before reading msg
	// Call Get type,CItem, Length as needed
	// Call Getparmx() in sequence to read msg parameters in msg order.
	void InitRxMsg(){Length = 4;}
	quint8 GetType(){return Buf8[1]&TYPE_MASK;}
	quint16 GetLength(){return Buf16[0]&LENGTH_MASK;}
	quint16 GetCItem(){return Buf16[1];}
	quint8 GetParm8(){return Buf8[Length++];}
	quint16 GetParm16(){tmp16 = Buf8[Length++]; tmp16 |= (quint16)(Buf8[Length++])<<8;
						return tmp16;	}
	quint32 GetParm32(){tmp32 = Buf8[Length++]; tmp32 |= (quint32)(Buf8[Length++])<<8;
						tmp32 |= (quint32)(Buf8[Length++])<<16;
						tmp32 |= (quint32)(Buf8[Length++])<<24;
						return tmp32;	}
	union
	{
		quint8 Buf8[MAX_ASCPMSG_LENGTH];
		quint16 Buf16[MAX_ASCPMSG_LENGTH/2];
	};
private:
	quint16 Length;
	quint16 tmp16;
	quint32 tmp32;
};

#endif // ASCPMSG_H
