#include "SerialNMEA.h"

void SerialNMEA::LitPort () {

#ifdef SIMU_SANS_CONNEXION
  loop_count ++;
  if (loop_count %4 == 0) {
  len = strlen (Chaines[idxChaines % 4]);
  memcpy (packetBuffer, Chaines [idxChaines % 4], len);
  idxChaines++;
  (*pSendUDP)(this);
  delay (1000);
  }
  return;
#endif
  bool qqChoseRecu = false;
  while (pPortNMEA->available()) {
    // get the new byte:
    packetBuffer[len] = (byte)pPortNMEA->read();
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (packetBuffer[len] == '\n') {
      stringComplete = true;
    }
    len++;

    if (len > maxLen) {
      nbTramesNMEATropLongues ++;
    }

    if (stringComplete || (len > maxLen)) {    // if we missed the newline don't oveer run the buffer
      (*pSendUDP)(this);
      len = 0;
      stringComplete = false;
    }
	yield();
    qqChoseRecu = true;       
  }
  if (qqChoseRecu) {
    momentDernierReceptionNMEA = millis ();
  }
}

