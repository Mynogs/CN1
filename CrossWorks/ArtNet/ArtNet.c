//-------------------------------------------------------------------------------
// Art-Net V1.0a
//-------------------------------------------------------------------------------
// Note: AVR is little endian:
// Addr   0  1  2  3
// Data  LL ML MH HH
//
//
//
//-------------------------------------------------------------------------------
#include "artNet.h"
#include "c:\Entwickl\M100\RNet.AVR\RNet.h"
#include "c:\Entwickl\M100\RNet.AVR\NV.h"

#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h> 
#include <stdlib.h> 
#include "DMX.h"

#if 0
	#define DEBUG printf
#else
	#define DEBUG(...)
#endif

// Move to RNet
#include "c:\Entwickl\AVR.lib\ENC28J60\ENC28J60.h"

//-------------------------------------------------------------------------------
#define EE_BASE         0x000
#define EE_IP           EE_BASE + 30
#define EE_MASK         EE_BASE + 34
#define EE_ROUTER_IP    EE_BASE + 38

#define EE_SUBNET    	  EE_BASE + 50
#define EE_INUNIVERSE	  EE_BASE + 51
#define EE_OUTUNIVERSE	EE_BASE + 52
#define EE_PORT    	    EE_BASE + 54
#define EE_NETCONFIG    EE_BASE + 56
#define EE_SHORTNAME	  EE_BASE + 60
#define EE_LONGNAME     EE_BASE + 78
//-------------------------------------------------------------------------------
// Op codes (byte-swapped)?
#define OP_POLL   	        		0x2000 // Poll
#define OP_POLLREPLY   	  			0x2100 // ArtPollReply
#define OP_POLLFPREPLY    			0x2200 // Additional advanced reply
#define OP_DIAGDATA	    				0x2300 // Diagnostics data message
#define OP_COMMAND	    				0x2400 // Send command / property strings.

#define OP_OUTPUT 	       			0x5000 // Zero start code dmx packets
#define OP_DMX	 	       				0x5000 // Zero start code dmx packets
#define OP_NZS	 	       				0x5100 // Non-Zero start code dmx packets (excluding RDM)
#define OP_ADDRESS 	  					0x6000 // Program Node Settings
#define OP_INPUT 	        			0x7000 // Setup DMX input enables

#define OP_TODREQUEST        		0x8000 // Requests RDM Discovery ToD
#define OP_TODDATA        			0x8100 // Send RDM Discovery ToD
#define OP_TODCONTROL        		0x8200 // Control RDM Discovery
#define OP_RDM	        				0x8300 // Non-Discovery RDM Message
#define OP_RDMSUB               0x8400 // Compressed subdevice data

#define OP_MEDIA	        			0x9000 // Streaming data from media server
#define OP_MEDIAPATCH          	0x9100 // Coord setup to media server
#define OP_MEDIACONTROL        	0x9200 // Control media server
#define OP_MEDIACONTROLREPLY   	0x9300 // Reply from media server

#define OP_TIMECODE          		0x9700 // Transports Time Code
#define OP_TIMESYNC          		0x9800 // Time & Date synchronise nodes
#define OP_TRIGGER          		0x9900 // Trigger and macro
#define OP_DIRECTORY          	0x9a00 // Request node file directory
#define OP_DIRECTORYREPLY      	0x9b00 // Send file directory macro

#define OP_VIDEOSETUP 	     		0xa010 // Setup video scan and font
#define OP_VIDEOP_ALETTE 	     	0xa020 // Setup colour palette
#define OP_VIDEODATA 	      		0xa040 // Video Data

#define OP_MACMASTER 	      		0xf000
#define OP_MACSLAVE 	     			0xf100
#define OP_FIRMWAREMASTER      	0xf200 // For sending firmware updates
#define OP_FIRMWAREREPLY       	0xf300 // Node reply during firmware update
#define OP_FILETNMASTER        	0xf400 // Upload user file to node
#define OP_FILEFNMASTER        	0xf500 // Download user file from node
#define OP_FILEFNREPLY         	0xf600 // Ack file packet received from node

#define OP_IPPROG	      				0xf800 // For sending IP programming info.
#define OP_IPPROGREPLY	     		0xf900 // Node reply during IP programming.
//-------------------------------------------------------------------------------
// Style codes for ArtPollReply
#define STYLE_NODE							0	// Responder is a Node (DMX <-> Ethernet Device)
#define STYLE_CONTROLLER				1	// Lighting console or similar
#define STYLE_MEDIA							2	// Media Server such as Video-Map, Hippotizer, RadLight, Catalyst, Pandora's Box
#define STYLE_ROUTE							3	// Data Routing device
#define STYLE_BACKUP						4	// Data backup / real time player such as Four-Play
#define STYLE_CONFIG						5	// Configuration tool such as DMX-Workshop
#define STYLE_VISUAL     				6 // Visualiser
//-------------------------------------------------------------------------------
// Status
#define RC_POWER_OK				0x01
#define RC_PARSE_FAIL			0x04
#define RC_SH_NAME_OK			0x06
#define RC_LO_NAME_OK			0x07
//-------------------------------------------------------------------------------
// Default values
#define SUBNET_DEFAULT			  0
#define INUNIVERSE_DEFAULT		1
#define OUTUNIVERSE_DEFAULT		0
#define PORT_DEFAULT			    0x1936
#define NETCONFIG_DEFAULT		  1
//-------------------------------------------------------------------------------
// Constantes
#define PORTS_LENGTH			  4
#define SHORT_NAME_LENGTH		18
#define LONG_NAME_LENGTH		64
#define PORT_NAME_LENGTH		32
#define MAX_DATA_LENGTH			511
//-------------------------------------------------------------------------------
// Settings
#define DMX_CHANNELS        1
#define PROTOCOL_VERSION 		14 		  // DMX-Hub protocol version.
#define FIRMWARE_VERSION 		0x0100	// DMX-Hub firmware version.
#define OEM_ID 				    	0  /////0xB108  // OEM Code (Registered to DMXControl, must be changed in own implementation!)
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
struct ArtNetHeader {
  uint8  id[8];         // "Art-Net"
  uint16 opCode;
};
//-------------------------------------------------------------------------------
struct ArtNetPoll {
	struct ArtNetHeader header;
	uint8  versionH;
	uint8  version;
	uint8  talkToMe;
	uint8  pad;
};
//-------------------------------------------------------------------------------
struct ArtNetPollReply {
	struct ArtNetHeader header;
  uint32 ip;
  uint16 port;
	uint8  versionInfo[2]; // H L
	uint8  netSwitch;
	uint8  subSwitch;
	uint8  oem[2]; // H L
	uint8  ubeaVersion;
	uint8  status1;
	uint16 estaMan;
	char   shortName[SHORT_NAME_LENGTH];
	char   longName[LONG_NAME_LENGTH];
	char   nodeReport[LONG_NAME_LENGTH];
	uint8  numPorts[2]; // H L
	uint8  portTypes[PORTS_LENGTH];
	uint8  goodInput[PORTS_LENGTH];
	uint8  goodOutput[PORTS_LENGTH];
	uint8  swin[PORTS_LENGTH];
	uint8  swout[PORTS_LENGTH];
	uint8  swVideo;
	uint8  swMacro;
	uint8  swRemote;
	uint8  spare1;
	uint8  spare2;
	uint8  spare3;
	uint8  style;
	uint8  mac[6];
  uint8  bindIP[4];
	uint8  bindIndex;
  uint8  status2;
	uint8  filler[26];
};
//-------------------------------------------------------------------------------
struct ArtNetIPProg {
	struct ArtNetHeader header;
	uint8  versionH;
	uint8  version;
	uint8  filler1;
	uint8  filler2;
	uint8  command;
	uint8  filler3;
	uint32  progIp;
	uint32  progSm;
	uint8  progPort[2];
	uint8  spare[8];
};
//-------------------------------------------------------------------------------
struct ArtNetIPProgReply {
	struct ArtNetHeader header;
	uint8  versionH;
	uint8  version;
	uint8  filler1;
	uint8  filler2;
	uint8  filler3;
	uint8  filler4;
	uint32  progIp;
	uint32  progSm;
	uint8  progPort[2];
	uint8 status;
	uint8  spare[7];
};
//-------------------------------------------------------------------------------
struct ArtNetAddress {
	struct ArtNetHeader header;
	uint8  versionH;
	uint8  version;
	uint8  filler1;
	uint8  filler2;
	char   shortName[SHORT_NAME_LENGTH];
	char   longName[LONG_NAME_LENGTH];
	uint8  swin[PORTS_LENGTH];
	uint8  swout[PORTS_LENGTH];
	uint8  subSwitch;
	uint8  swVideo;
	uint8  command;
};
//-------------------------------------------------------------------------------
struct ArtNetDMX {
	struct ArtNetHeader header;
	uint8  version[2]; // H L
	uint8  sequence;
	uint8  physical;
	uint16 universe;
	uint8  length[2]; // H L
	uint8  data[0];
};
//-------------------------------------------------------------------------------
struct {
	uint8  subNet;
	uint8  outputUniverse1;
	uint8  inputUniverse1;
	uint8  sendPollReplyOnChange;
	uint32 pollReplyTarget;
	uint16 pollReplyCount;
	uint8  status;
	char   shortName[18];
	char   longName[64];
	uint16 port;
	uint8  netConfig;
	uint8 sequence;
	uint8 ledMode; // 0:Unknown 1:Locate 2:Mute 3:Normal 
} artNet = {
	SUBNET_DEFAULT,
	OUTUNIVERSE_DEFAULT,
	INUNIVERSE_DEFAULT,
	false,
	(uint32) 0xffffffffL,
	0,
	RC_POWER_OK,
	"",
	"",
	PORT_DEFAULT,
	NETCONFIG_DEFAULT,
	1,
	3 // Normal led mode
};
//-------------------------------------------------------------------------------
// IP address configuration – Static Addressing
// Default networkSwitch == false so IP is 2.?.?.?  
//-------------------------------------------------------------------------------
uint32 ArtNetMAXToIP(uint8 *mac,bool networkSwitch)
{
  return MAKEIP(
    networkSwitch ? 10 : 2,
    ((OEM_ID >> 8) + OEM_ID + mac[3]) & 0xFF, 	
    mac[4],	
    mac[5]
  );	
}
//-------------------------------------------------------------------------------
// Prepare Art-Net UDP packet
// Fill in "Art-Net" and op code
//-------------------------------------------------------------------------------
static void* ArtNetPreparePacket(uint16 opCode,uint16 dataSize)
{
  //struct ETHHeader *eth = (struct ETHHeader*) ethernet.buffer;
  struct IPHeader *ip = (struct IPHeader*) &ethernet.buffer[sizeof(struct ETHHeader)];
	struct UDPHeader *udp = (struct UDPHeader*) &ethernet.buffer[sizeof(struct ETHHeader) + (ip->versionHeaderLength & 0x0F) * 4];
	uint8 *data = (uint8*) udp + sizeof(struct UDPHeader);	
	memset(data,0,dataSize);
	struct ArtNetHeader *header = (struct ArtNetHeader*) data;
	memcpy_P(header->id,PSTR("Art-Net"),8);
	header->opCode = opCode;
	return data;
}
//-------------------------------------------------------------------------------
// Finish (calc checksum, set ip's and mac's) and send packet
//-------------------------------------------------------------------------------
static void ArtNetFinishAndSendPacket(uint16 dataLength,uint32 destIP)
{
  DEBUG("ArtNetFinishAndSendPacket(%d %08lX)\n",dataLength,destIP);
  
  struct ETHHeader *eth = (struct ETHHeader*) ethernet.buffer;
  struct IPHeader *ip = (struct IPHeader*) &ethernet.buffer[sizeof(struct ETHHeader)];
	struct UDPHeader *udp = (struct UDPHeader*) &ethernet.buffer[sizeof(struct ETHHeader) + (ip->versionHeaderLength & 0x0F) * 4];
	
	ip->destIP = destIP; 
	ip->sourceIP = ethernet.thisIP;

  // IP header checksummer ausrechnen
  ip->totalLenght = ChangeEndian16((ip->versionHeaderLength & 0x0F) * 4 + sizeof(struct UDPHeader) + dataLength);
	ip->headerCheckSum = 0;
	ip->headerCheckSum = ChangeEndian16(Checksum16((uint16*) ip,(ip->versionHeaderLength & 0x0F) * 4));

  udp->sourcePort = ChangeEndian16(artNet.port);
  udp->destPort = ChangeEndian16(artNet.port);

	udp->dataLenght = ChangeEndian16(sizeof(struct UDPHeader) + dataLength);
  udp->checkSum = 0;

	memcpy(eth->destMAC,eth->sourceMAC,6);
	memcpy(eth->sourceMAC,ethernet.thisMAC,6);
  uint16 length = sizeof(struct ETHHeader) + ChangeEndian16(ip->totalLenght);

  DEBUG("ENC28J60PacketSend %d\n",length);
  //Dump(0,length,"ENC28J60PacketSend");
  ENC28J60PacketSend(length,ethernet.buffer);	
}
//-------------------------------------------------------------------------------
// Init the Art-Net 
//-------------------------------------------------------------------------------
void ArtNetInit(void) 
{
	// read Art-Net port
	artNet.port = NVLoadUInt16(EE_PORT);
	if (!artNet.port) artNet.port = PORT_DEFAULT;
	
	// read netconfig
	artNet.netConfig = NVLoadUInt8(EE_NETCONFIG);
	if (!artNet.netConfig) artNet.netConfig = NETCONFIG_DEFAULT;
	
	// read subnet
	artNet.subNet = NVLoadUInt8(EE_SUBNET);
	
	// read nr. of input universe
	artNet.inputUniverse1 = NVLoadUInt8(EE_INUNIVERSE);
	if (!artNet.inputUniverse1) artNet.inputUniverse1 = INUNIVERSE_DEFAULT;
	
	// read nr. of output universe
	artNet.outputUniverse1 = NVLoadUInt8(EE_OUTUNIVERSE);
		
	// read short name
	NVLoadBlock((uint8*) artNet.shortName,EE_SHORTNAME,SHORT_NAME_LENGTH);
	if (!artNet.shortName[0]) strcpy_P(artNet.shortName, PSTR("FinalDMX"));
	
	// read long name
	NVLoadBlock((uint8*) artNet.longName,EE_LONGNAME,LONG_NAME_LENGTH);
	if (!artNet.longName[0]) strcpy_P(artNet.longName, PSTR("FinalDMX ArtNet node"));
	
	// annouce that we are here 
	DEBUG("Annouce that we are here\n");
	
	// Fake a ArtNetPoll packet
  memset(ethernet.buffer,0,sizeof(ethernet.buffer));
	struct ETHHeader *eth = (struct ETHHeader*) ethernet.buffer;
	
	memset(eth->sourceMAC,0xFF,6);   
	eth->type = 0x0008;               

  struct IPHeader *ip = (struct IPHeader*) &ethernet.buffer[sizeof(struct ETHHeader)];
	ip->versionHeaderLength = 0x45;
	ip->tos = 0;
	ip->identification = 0;
	ip->fragmentOffsetFlags = 0;
	ip->ttl = 128;
	ip->protocol = 17;
	ip->protocol = 0x11; // UDP
	ip->sourceIP = -1;
	ip->headerCheckSum = 0;
	ip->headerCheckSum = ChangeEndian16(Checksum16((uint16*) ip,(ip->versionHeaderLength & 0x0F) * 4));
	
	ArtNetSendPollReply();
	// enable PollReply on changes
	artNet.sendPollReplyOnChange = true;
}
//-------------------------------------------------------------------------------
// A device, in response to a Controller’s ArtPoll, sends the ArtPollReply. 
// This packet is also broadcast to the Directed Broadcast address by 
// all Art-Net devices on power up.
//-------------------------------------------------------------------------------
void ArtNetSendPollReply(void) 
{
  struct ArtNetPollReply *msg = ArtNetPreparePacket(OP_POLLREPLY,sizeof(struct ArtNetPollReply));
  msg->ip = ethernet.thisIP; 
  msg->port = PORT_DEFAULT; 
  msg->versionInfo[0] = FIRMWARE_VERSION >> 8;
  msg->versionInfo[1] = FIRMWARE_VERSION;
  msg->netSwitch = 0;
  msg->subSwitch = artNet.subNet & 0x0F;
  msg->oem[0] = OEM_ID >> 8;
  msg->oem[1] = OEM_ID;
  msg->ubeaVersion = 0;
  msg->status1 = (2 << 4) | (artNet.ledMode << 6);
  msg->estaMan = ((uint16) 'A' << 8) + 'R';
  strcpy(msg->shortName,artNet.shortName);
  strcpy(msg->longName,artNet.longName);
  sprintf(msg->nodeReport, "#%04X [%04u] Node is ready",artNet.status,artNet.pollReplyCount);
  msg->numPorts[0] = 0;
  msg->numPorts[1] = CONFIG_ARTNET_CHANNELS;
  msg->portTypes[0] = BIT(7) | 0;
	msg->goodInput[0] = (1 << 3);
	#if CONFIG_ARTNET_CHANNELS == 2
		msg->portTypes[1] = BIT(7) | 0;
		msg->goodInput[1] = (1 << 3);
	#endif
  msg->goodOutput[0] = DMXTransmit(0,0) ? BIT(7) : 0;
	msg->swin[0] = (artNet.subNet << 4) | (artNet.inputUniverse1 & 0x0F);
	msg->swout[0] = (artNet.subNet << 4) | (artNet.outputUniverse1 & 0x0F);
	msg->style = STYLE_NODE;
	msg->status2 = BIT(2) | BIT(3); // DHCP captable, Art-Net 3
	memcpy(msg->mac,ethernet.thisMAC,6);
	DEBUG("artNet.sendPollReply:");
	ArtNetFinishAndSendPacket(sizeof(struct ArtNetPollReply),artNet.pollReplyTarget);
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void ArtNet_sendIpProgReply(uint32 target) 
{
 	struct ArtNetIPProgReply *msg = ArtNetPreparePacket(OP_IPPROGREPLY,sizeof(struct ArtNetIPProgReply));
	msg->versionH = 0;
 	msg->version = PROTOCOL_VERSION;
	msg->progIp = ethernet.thisIP;
 	msg->progSm = ethernet.thisMask;
 	msg->progPort[0] = artNet.port >> 8;
 	msg->progPort[1] = artNet.port;
  DEBUG("artNet.sendIpProgReply:");
 	ArtNetFinishAndSendPacket(sizeof(struct ArtNetIPProgReply),target);
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void ArtNet_sendDmxPacket(void) 
{
  struct ArtNetDMX *msg = ArtNetPreparePacket(OP_OUTPUT,sizeof(struct ArtNetDMX));
  msg->version[0] = 0;
  msg->version[1] = PROTOCOL_VERSION;
  msg->sequence = artNet.sequence++;
  if (!artNet.sequence) artNet.sequence = 1;
  msg->physical = 1;
  msg->universe = (artNet.subNet << 4) | artNet.inputUniverse1;
  msg->length[0] = DMX_CHANNELS >> 8;
  msg->length[1] = DMX_CHANNELS;
  ///////////memcpy(&(msg->data),(uint8 *)&artNet.dmxUniverse[0], artNet.dmxChannels);
  DEBUG("artNet.sendDmxPacket:");
  /////////ArtNetFinishAndSendPacket(sizeof(struct ArtNetDMX) + artNet.dmxChannels,-1);
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void processPollPacket(struct ArtNetPoll *poll) 
{
  struct IPHeader *ip = (struct IPHeader*) &ethernet.buffer[sizeof(struct ETHHeader)];
	artNet.sendPollReplyOnChange = !!(poll->talkToMe & BIT(1));
	artNet.pollReplyTarget = poll->talkToMe & BIT(0) ? ip->sourceIP : -1;
	ArtNetSendPollReply();
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void ProcessDMXPacket(struct ArtNetDMX *dmx)
{    
 	uint16 length = ((uint16) dmx->length[0] << 8) + dmx->length[1]; 
	/*
  printf("version %u \n",((uint16) dmx->version[0] << 7) + dmx->version[1]);		
  printf("sequence %d\n",dmx->sequence);		
  printf("physical %d\n",dmx->physical);		
  printf("universe %d\n",dmx->universe);		
  printf("length %u\n",length);		
  */ 
  
  #if 1
	  DMXSetTX(3);
	  DMXStart(0);
	  for (uint16 i = 0;i < length;i++) DMXByte(dmx->data[i]);
  #else
	  printf("DMXTransmit %d\n",DMXTransmit(dmx->data,length));  
  #endif
	
  /*		
	if (dmx->universe == ((artNet.subNet << 4) | artNet.outputUniverse1)) {
 	  if (artNet.dmxDirection == 0) {
	    artNet.dmxChannels = (dmx->length[0] << 8) | dmx->length[0];
	    memcpy((uint8*) &artNet.dmxUniverse[0], &(dmx->data), artNet.dmxChannels);
	
	    if (artNet.dmxTransmitting == false) {
				 // setup USART
				 ////PORTD |= (1 << 3);
				 ////UBRR   = (F_CPU / (250000 * 16L) - 1);
				 ////  UCSRC  = (1<<URSEL) | (3<<UCSZ0) | (1<<USBS);
				 ////  UCSRB  = (1<<TXEN) | (1<<TXCIE);
				 ////  UDR    = 0;							//start transmitting
	
	      artNet.dmxTransmitting = true;
	      if (artNet.sendPollReplyOnChange == true) {
	         artNet.pollReplyCounter++;
	         ArtNetSendPollReply();
	      }
	    }
	  }
	}
	*/
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void processAddressPacket(struct ArtNetAddress *address) 
{
  bool changed = false;
	if (address->shortName[0] != 0) {
		strcpy(artNet.shortName,address->shortName);
		NVSaveBlock((uint8*) artNet.shortName,EE_SHORTNAME,SHORT_NAME_LENGTH);
		artNet.status = RC_SH_NAME_OK;
		changed = true;
	}
	if (address->longName[0] != 0) {
		strcpy(artNet.longName, address->longName);
		NVSaveBlock((uint8*) artNet.longName,EE_LONGNAME,LONG_NAME_LENGTH);
		artNet.status = RC_LO_NAME_OK;
		changed = true;
	}
	if (address->swin[0] == 0) {
		artNet.inputUniverse1 = INUNIVERSE_DEFAULT;
		NVSaveUInt8(EE_INUNIVERSE,artNet.inputUniverse1);
		changed = true;
	} else if ((address->swin[0] & 128) == 128) {
		artNet.inputUniverse1 = address->swin[0] & 0x0F;
		NVSaveUInt8(EE_INUNIVERSE,artNet.inputUniverse1);
		changed = true;
	}
	if (address->swout[0] == 0) {
		artNet.outputUniverse1 = OUTUNIVERSE_DEFAULT;
		NVSaveUInt8(EE_OUTUNIVERSE,artNet.outputUniverse1);
		changed = true;
	} else if ((address->swout[0] & 128) == 128) {
		artNet.outputUniverse1 = address->swout[0] & 0x0F;
		NVSaveUInt8(EE_OUTUNIVERSE,artNet.outputUniverse1);
		changed = true;
	}
	if (address->subSwitch == 0) {
		artNet.subNet = SUBNET_DEFAULT;
		NVSaveUInt8(EE_SUBNET,artNet.subNet);
		changed = true;
	} else if ((address->subSwitch & 128) == 128) {
		artNet.subNet = address->subSwitch & 0x0F;
		NVSaveUInt8(EE_SUBNET,artNet.subNet);
		changed = true;
	}

	// Send PollReply when something changed
	if (changed && artNet.sendPollReplyOnChange) {
		artNet.pollReplyCount++;
		ArtNetSendPollReply();
	}
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
static void ArtNetProcessIpProgPacket(struct ArtNetIPProg *ipprog) 
{
	if (ipprog->command & BIT(7)) {
		if (ipprog->command & BIT(0)) {
			artNet.port = (ipprog->progPort[0] << 8) | ipprog->progPort[1];
			NVSaveUInt16(EE_PORT,artNet.port);
		}
		if (ipprog->command & BIT(1)) {
			ethernet.thisMask = ipprog->progSm;
			NVSaveUInt32(EE_MASK,ethernet.thisMask);
			artNet.netConfig = 1;
		}
		if (ipprog->command & BIT(3)) {
			ethernet.thisIP = ipprog->progIp;
			NVSaveUInt32(EE_IP,ethernet.thisIP);
			artNet.netConfig = 1;
		}
		if (ipprog->command & BIT(4)) {
			artNet.port = PORT_DEFAULT;
			NVSaveUInt16(EE_PORT,artNet.port);
			artNet.netConfig = 0;
		}
		NVSaveUInt8(EE_NETCONFIG,artNet.netConfig);
	}
	struct IPHeader *ip = (struct IPHeader*) &ethernet.buffer[sizeof(struct ETHHeader)];
	ArtNet_sendIpProgReply(ip->sourceIP);
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void ArtNetHandlePacket(void) 
{
  struct IPHeader *ip = (struct IPHeader*) &ethernet.buffer[sizeof(struct ETHHeader)];
	struct UDPHeader *udp = (struct UDPHeader*) &ethernet.buffer[sizeof(struct ETHHeader) + (ip->versionHeaderLength & 0x0F) * 4];
  uint8 *data = (uint8*) udp + sizeof(struct UDPHeader);	
  struct ArtNetHeader *header = (struct ArtNetHeader*) data;
 
	//check the id
	if ((header->id[0] != 'A') ||
	    (header->id[1] != 'r') ||
	    (header->id[2] != 't') ||
	    (header->id[3] != '-') ||
	    (header->id[4] != 'N') ||
	    (header->id[5] != 'e') ||
	    (header->id[6] != 't') ||
	    (header->id[7] !=  0 )) {
  	DEBUG("Wrong ArtNet header, discarded\r\n");
	  artNet.status = RC_PARSE_FAIL;
	  return;
	}

	if (header->opCode == OP_POLL) {

		DEBUG("Recv ArtNetPoll\n");
		processPollPacket((struct ArtNetPoll*) data);

	} else if (header->opCode == OP_POLLREPLY) {

		DEBUG("Recv OP_POLLREPLY\n");

	} else if (header->opCode == OP_OUTPUT) {

		DEBUG("Recv ArtNetDMX\n");
    ProcessDMXPacket((struct ArtNetDMX*) data);
		
	} else if (header->opCode == OP_ADDRESS) {

		DEBUG("Recv ArtNetAddress\n");
		processAddressPacket((struct ArtNetAddress*) data);

	} else if (header->opCode == OP_IPPROG) {

		DEBUG("Recv ArtNetIPProg\n");
		ArtNetProcessIpProgPacket((struct ArtNetIPProg*) data);

	}
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
/**
// DMX transmission
ISR (USART_TXC_vect) {
 static uint8  dmxState = BREAK;
 static uint16 curDmxCh = 0;

 if (dmxState == STOPPED) {
  if (artNet.dmxTransmitting == true) {
   dmxState = BREAK;
  }
 } else if (dmxState == BREAK) {
  UBRR = (F_CPU / (50000 * 16L) - 1);
  UDR      = 0;								//send break
  dmxState = STARTB;
 } else if (dmxState == STARTB) {
  UBRR = (F_CPU / (250000 * 16L) - 1);
  UDR      = 0;								//send start byte
  dmxState = DATA;
  curDmxCh = 0;
 } else {
  _delay_us(IBG);
  UDR      = artNet.dmxUniverse[curDmxCh++];			//send data
  if (curDmxCh == artNet.dmxChannels) {
   if (artNet.dmxTransmitting == true) {
    dmxState = BREAK; 						//new break if all ch sent
   } else {
    dmxState = STOPPED;
   } 
  }
 }
}
**/
// ----------------------------------------------------------------------------
// DMX reception
/**
ISR (USART_RXC_vect) {
 static uint8  dmxState = 0;
 static uint16 dmxFrame = 0;
 uint8 status = UCSRA; 	// status register must be read prior to UDR (because of 2 byte fifo buffer)
 uint8 byte = UDR; 		// immediately catch data from i/o register to enable reception of the next byte

 if ((byte == 0) && (status & (1<<FE))) {				// BREAK detected (Framing Error)
  dmxState = 1;
  dmxFrame = 0;
  if (artNet.dmxChannels > 0) {
   artNet.dmxInComplete = true;
  }
 } else if (dmxFrame == 0) {							// Start code test
  if ((byte == 0) && (dmxState == 1)) {					// valid SC detected
   dmxState = 2;
  }
  dmxFrame = 1;
 } else {
  if ((dmxState == 2) && (dmxFrame <= MAX_CHANNELS)) {	// addressed to us
   if (artNet.dmxUniverse[dmxFrame - 1] != byte) {
    artNet.dmxUniverse[dmxFrame - 1] = byte;
	artNet.dmxInChanged = true;
   }
   if (dmxFrame > artNet.dmxChannels) {
    artNet.dmxChannels = dmxFrame;
   }
  }
  dmxFrame++;
 }
}
**/
