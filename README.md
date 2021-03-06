# LoRaWAN-Arduino-Sensor-Sketches
A repository containing various Arduino sketches made by the Digital Ag Team to read different sensors for smart farming and transmit the data over LoRaWAN using Adafruit Feather M0 Microcontrollers.

Sensors Supported
1. Decagon 5TM Soil Moisture Sensor
	- http://publications.metergroup.com/Manuals/20424_5TM_Manual_Web.pdf
2. Edaphic NDVI Normalized Difference Vegetation Index Sensor
	- https://www.edaphic.com.au/products/light/ndvi-pri-sensors/
3. Meter Atmos 41 Automatic Weather Station
	- https://www.metergroup.com/environment/products/atmos-41-weather-station/
4. Meter Teros 21 Soil Water Potential Sensor
	- https://www.metergroup.com/environment/products/teros-21/
5. Adafruit Ultimate GPS Module
	- https://learn.adafruit.com/adafruit-ultimate-gps
  
Notes: 
1. Sketches require the user to update the LoRa-otaa.h file with the relevant The Things Network otaa (over-the-air-activation) EUI keys.
	- Both the Device EUI and the App EUI need to have the Least Significant Bit (LSB) first whilst the App Key has the Most Significant Bit (MSB) first.
	If these keys are being retrieved from The Things Network device page this can be switched with the buttons next to the field containing the relevant key.

Adafruit Feather M0:
1. Requires pin 6 to be connected to io1 for continuous transmission beyond the first uplink. This applies for all sketches, and if not done your device may simply sit there and continue to activate instead of joining the network.
2. The Adafruit Feather boards require you download the board information/configuration by first going to preferences and adding the below link to the Additional Board Manager URL's and separating additional URL's with a comma.
	- https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
3. After which you then go to Tools -> Boards -> Boards Manager and search for SAMD and install Arduino SAMD Boards (32-bits ARM Cortex M0+). This completes the IDE setup for the boards.
3. To upload to the feather plug it in to the computer and double press the reset button, the LED should now flash indicating the board is in upload mode.
4. Ensure you select the device in the Tools -> Port menu before selecting upload.

Libraries:
These sketches use the following libraries (which are included in the libraries folder);
- RTC - Real Time Clock
- LMiC - Lora Mac in C (by Mathiaskoojman IBM) Configured to AS-923 for Australia
- Math
- SDI-12 - Arduino Serial Digital Interface
- Cayenne LPP (Low Powered Packet) for transmitting data to the Cayenne platform
- ArduinoJson - JSON library for Arduino and embedded C++
- Adafruit Unified Sensor
- Adafruit ADT7410 Temperature Sensor
- Adafruit ADXL343 Triple Axis Accelerometer
- Adafruit GPS - Adafruit GPS library for Adafruit Ultimate GPS
