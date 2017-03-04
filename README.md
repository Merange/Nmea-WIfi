# Nmea-WIfi
A simple bridge to broadcast on UDP NMEA sentenses from Rs422 interface

The idea is to have a low cost bridge.

I use an ESP8266 to broadcast the Nmea sentenses and a simple hardware to convert RS-422 (normal NMEA hardware standard) to 3.3V signals

I choose a Wemos Mini D1 to simplify all the cabling and I use Arduino IDE to develop the code.

My project is based on a project made for an RS232 hardware interface and an ESP-01 hardware. You can find it here http://kb7kmo.blogspot.fr/2016/03/esp8266-design-contest-entry-nmea.html

I use Software COM to get the information from the NMEA.

