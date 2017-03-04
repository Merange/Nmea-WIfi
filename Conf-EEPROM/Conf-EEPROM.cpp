#include "Conf-EEPROM.h"

bool ConfEEPROM::LitDeEEPROM () {
	byte* p = (byte*)(void*)this;
    unsigned int i;
    for (i = 0; i < sizeof(*this); i++) {
	  *p++ = EEPROM.read(i);
	}
	
	return EstValide ();
}

void ConfEEPROM::EcritDansEEPROM () {
	const byte* p = (const byte*)(const void*)this;
    unsigned int i;
    for (i = 0; i < sizeof(*this); i++){
		EEPROM.write(i, *p++);
	}	
    EEPROM.commit ();   
}

bool ConfEEPROM::EstValide () {
	if (ConfOk != 1) {
		ConfOk = 0;
	}
	
	ConfOk &= SSID [MAX_SSID_LEN] == '\0';
	ConfOk &= Password [MAX_PASSWORD_LEN] == '\0';
	if ((PortUDP <= 0) || (PortUDP >= 65535)) {
		ConfOk = false;
		PortUDP = 10110;
	}
	
	switch (BaudRate) {
		case 4800:
		case 9600:
		case 19200:
		case 38400:
		case 115200:
			break;
		default: 
			ConfOk = false;
			BaudRate = 4800;
			break;
	}

	// Pour être sûr de ne pas avoir de problème de chaîne trop longue
	SSID [MAX_SSID_LEN] = '\0';
	Password [MAX_PASSWORD_LEN] = '\0';

	#ifdef DEBUG
	Serial.println (String ("Configuration testee \nSSID:") + SSID);
	Serial.println (String ("Password:") + Password);
	Serial.println (String ("Port UDP:") + PortUDP);
	Serial.println (String ("BaudRate: ") + BaudRate);
	#endif
	return (ConfOk);
}
