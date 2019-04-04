# ZXWiFino

A modified TZXDuino with WiFi support and other goodies

		Developed with Visual Studio 2017 and Visual Micro for Arduino

		I HATE to have to get the card out, place it on my computer, create the correct folders
		copy the tapes and add it again to the device each time I'm testing new games, so I decided to
		add an inexpensive ESP-01 WiFi module to send the tapes directly from my pc through WiFi.
		Yes, I'm a lazy guy :D

		To use the WiFi server press the WiFi button, to terminate the server press it again.
		If an error happens there will be a notice with the error code and the WiFi server will terminate automatically

		To fast-forward FAT records, keep pressed the up or down button and it will start to jump by ten records per second

		Currently the AP configuration is stored in defines, I plan to add support to read it from the SD card.

		Had to drop support for the motor and different LCD's
		as the device is running out of memory and program space.
		
		Currently only support for 16x2 parallel LCD in 4 bit mode (the only one I had available :P)

		I plan to re-add LCD support as compile time conditions again but need to buy them to test it.
		Feel free to add it yourself and create a pull request if you have some available.

		Use an ESP-01 module at 115200bps connected to the Arduino's hardware serial port.
		DO NOT USE SOFTWARE SERIAL, IT WILL NOT READ THE DATA CORRECTLY AT 115200bps.
		Also, remember that ESP-01 IS NOT 5V TOLERANT, use an adapter shield like this:https://www.aliexpress.com/item/ESP8266-ESP-01-ESP01-Serial-WiFi-Wireless-Adapter-Module-3-3V-5V-Compatible-Serial-Board-For/32893131651.html

		Tested firmware:Ai-Thinker_ESP8266_AT_Firmware_DOUT_v1.5.4.1-a_20171130 
		Found at http://acoptex.com/project/289/basics-project-021b-how-to-update-firmware-esp8266-esp-01-wi-fi-module-at-acoptexcom/
		Beware, it must be an ESP-01 module with a 8Mb flash