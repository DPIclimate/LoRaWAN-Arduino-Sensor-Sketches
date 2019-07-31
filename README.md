# LoRaWAN-Arduino-Sensor-Sketches
A repository containing various Arduino sketches made by the Digital Ag Team to read different sensors for smart farming.

Sensors Supported
  1. Decagon 5TM Soil Moisture Sensor
  
Notes: 
1. Sketches require the user to update the LoRa-otaa.h file with the relevant The Things Network otaa (over-the-air-activation) EUI keys.
	- Both the Device EUI and the App EUI need to have the Least Significant Bit (LSB) first whilst the App Key has the Most Significant Bit (MSB) first.
	If these keys are being retrieved from The Things Network device page this can be switched with the buttons next to the field containing the relevant key.

Adafruit Feather M0:
1. Requires pin 6 to be connected to io1 for continuous transmission beyond the first uplink.

Libraries:
	These sketches use the following libraries;
		- RTC - Real Time Clock
		- LMiC - Lora Mac in C (by Mathiaskoojman IBM)
		- Math