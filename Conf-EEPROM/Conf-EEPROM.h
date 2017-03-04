#ifndef Conf_EEPROM_h
#define Conf_EEPROM_h

#include "Arduino.h"
#include "EEPROM.h"


class ConfEEPROM {
public:
	static const int MAX_SSID_LEN = 32;
	static const int MAX_PASSWORD_LEN = 64;
	char SSID [MAX_SSID_LEN+1];
	char Password [MAX_PASSWORD_LEN+1];
	long PortUDP;
	long BaudRate;
	unsigned char ConfOk;
	
	ConfEEPROM () {
		memset (SSID, '\0', sizeof (SSID));
		memset (Password, '\0', sizeof (Password));
		PortUDP = 10110;
		BaudRate = 4800;
		ConfOk = true;
	}
	
	bool LitDeEEPROM ();
	void EcritDansEEPROM ();
	bool EstValide ();
	
};

#endif
