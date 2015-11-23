#ifndef __ArtNet
#define __ArtNet

//-------------------------------------------------------------------------------
#include "c:\Entwickl\M100\RNet.AVR\RNet.h"
//-------------------------------------------------------------------------------
// IP address configuration – Static Addressing
// Default networkSwitch == false so IP is 2.?.?.?  
//-------------------------------------------------------------------------------
uint32 ArtNetMAXToIP(uint8 *mac,bool networkSwitch);


// initialization of Art-Net
void ArtNetInit(void);
void ArtNetHandlePacket(void);
void ArtNetSendPollReply(void);
	
#endif