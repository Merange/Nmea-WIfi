#ifndef SerialNMEA_h
#define SerialNMEA_h

#include "Arduino.h"
#include "Stream.h"

//#define DEBUG
#ifdef DEBUG
//#define SIMU_SANS_CONNEXION
#ifdef SIMU_SANS_CONNEXION
	static const char * Chaines[] = { 
		"$GPRMC,165020.977,V,4054.930,N,07302.499,W,36.1,3.00,250217,,E*43\n",
		"$GPGGA,165021.977,4054.930,N,07302.499,W,0,00,,,M,,M,,*54\n",
		"$GPGLL,4054.930,N,07302.499,W,165022.977,V*37\n",
		"$GPVTG,3.00,T,,M,36.1,N,66.8,K*5F\n" };
#endif
#endif


class SerialNMEA;

typedef void (*SendUDPCallBack) (SerialNMEA * source);

class SerialNMEA {
public:
	SerialNMEA (Stream * portNMEA, SendUDPCallBack SendUDP):
		pPortNMEA (portNMEA),
		pSendUDP (SendUDP)	{
			packetBuffer[maxLen] = '\0';
		};
		
	static const byte maxLen = 100;   // longest NMEA sentence
	unsigned long int momentDernierReceptionNMEA = 0;
	unsigned long int nbTramesNMEATropLongues = 0;
	byte packetBuffer[maxLen+1];      // buffer for UDP communication
	int len = 0;                      // num chars in buffer

	SendUDPCallBack pSendUDP;

	void LitPort ();
	
private:
	Stream * pPortNMEA = NULL;
	
	bool stringComplete = false;   	  // whether the string is complete
	
	#ifdef DEBUG 
	#ifdef SIMU_SANS_CONNEXION
	unsigned int loop_count = 0;
	unsigned int idxChaines = 0;
	#endif
	#endif
};

#endif
