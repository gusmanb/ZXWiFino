/*
*
*		ZXWiFino 1.0, a TZXDuino remake
*
*		Copyright 2019 Agustín Gimenez Bernad
*
*		Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
*		associated documentation files (the "Software"), to deal in the Software without restriction, including
*		without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*		copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*
*		The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
*		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*		OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
*		LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
*		IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*		Modification of the TZXDuino V1.9
*		Adds support for an ESP-01 WiFi module, it
*		allows to add tapes remotely.
*		Also adds support for fast-forward FAT records (navigate a lot faster when you have tons of folders).
*		Made a cleanup to the code
*
*		Developed with Visual Studio 2017 and Visual Micro for Arduino
*
*		I HATE to have to get the card out, place it on my computer, create the correct folders
*		copy the tapes and add it again to the device each time I'm testing new games, so I decided to
*		add an inexpensive ESP-01 WiFi module to send the tapes directly from my pc through WiFi.
*		Yes, I'm a lazy guy :D
*
*		To use the WiFi server press the WiFi button, to terminate the server press it again.
*		If an error happens there will be a notice with the error code and the WiFi server will terminate automatically
*
*		To fast-forward FAT records, keep pressed the up or down button and it will start to jump by ten records per second
*
*		Currently the AP configuration is stored in defines, I plan to add support to read it from the SD card.
*
*		Had to drop support for the motor and different LCD's
*		as the device is running out of memory and program space.
*		
*		Currently only support for 16x2 parallel LCD in 4 bit mode (the only one I had available :P)
*
*		I plan to re-add LCD support as compile time conditions again but need to buy them to test it.
*		Feel free to add it yourself and create a pull request if you have some available.
*
*		Use an ESP-01 module at 115200bps connected to the Arduino's hardware serial port.
*		DO NOT USE SOFTWARE SERIAL, IT WILL NOT READ THE DATA CORRECTLY AT 115200bps.
*		Also, remember that ESP-01 IS NOT 5V TOLERANT, use an adapter shield like this:https://www.aliexpress.com/item/ESP8266-ESP-01-ESP01-Serial-WiFi-Wireless-Adapter-Module-3-3V-5V-Compatible-Serial-Board-For/32893131651.html
*
*		Tested firmware:Ai-Thinker_ESP8266_AT_Firmware_DOUT_v1.5.4.1-a_20171130 
*		Found at http://acoptex.com/project/289/basics-project-021b-how-to-update-firmware-esp8266-esp-01-wi-fi-module-at-acoptexcom/
*		Beware, it must be an ESP-01 module with a 8Mb flash
*
*/

//----ORIGINAL NOTICE FROM TZXDUINO------

// ---------------------------------------------------------------------------------
// DO NOT USE CLASS-10 CARDS on this project - they're too fast to operate using SPI
// ---------------------------------------------------------------------------------
/*
 *                                    TZXduino
 *                             Written and tested by
 *                          Andrew Beer, Duncan Edwards
 *                          www.facebook.com/Arduitape/
 *
 *              Designed for TZX files for Spectrum (and more later)
 *              Load TZX files onto an SD card, and play them directly
 *              without converting to WAV first!
 *
 *              Directory system allows multiple layers,  to return to root
 *              directory ensure a file titles ROOT (no extension) or by
 *              pressing the Menu Select Button.
 *
 *              Written using info from worldofspectrum.org
 *              and TZX2WAV code by Francisco Javier Crespo
 *
 *              ***************************************************************
 *              Menu System:
 *                TODO: add ORIC and ATARI tap support, clean up code, sleep
 *
 *              V1.0
 *                Motor Control Added.
 *                High compatibility with Spectrum TZX, and Tap files
 *                and CPC CDT and TZX files.
 *
 *                V1.32 Added direct loading support of AY files using the SpecAY loader
 *                to play Z80 coded files for the AY chip on any 128K or 48K with AY
 *                expansion without the need to convert AY to TAP using FILE2TAP.EXE.
 *                Download the AY loader from http://www.specay.co.uk/download
 *                and load the LOADER.TAP AY file loader on your spectrum first then
 *                simply select any AY file and just hit play to load it. A complete
 *                set of extracted and DEMO AY files can be downloaded from
 *                http://www.worldofspectrum.org/projectay/index.htm
 *                Happy listening!
 *
 *                V1.8.1 TSX support for MSX added by Natalia Pujol
 *
 *                V1.8.2 Percentage counter and timer added by Rafael Molina Chesserot along with a reworking of the OLED1306 library.
 *                Many memory usage improvements as well as a menu for TSX Baud Rates and a refined directory controls.
 *
 *                V1.8.3 PCD8544 library changed to use less memory. Bitmaps added and Menu system reduced to a more basic level.
 *                Bug fixes of the Percentage counter and timer when using motor control/
 */


#include <SdFat.h>
#include <SysCall.h>
#include <sdios.h>
#include <SdFatConfig.h>
#include <MinimumSerial.h>
#include <FreeStack.h>
#include <BlockDriver.h>
#include <Wire.h>
#include <TimerOne.h>
#include <EEPROM.h>
#include "ZXWiFino.h"
//Define your access point here
#define ACCES_POINT ""
//Define your password here
#define PASSWORD ""

#define VERSION "ZXWiFino 1.0"

#include <LiquidCrystal.h>
char indicators[] = { '|', '/', '-',0 };

//Parallel (4 bit) LCD pin definitions
#define rs 14
#define en 15
#define d4 16
#define d5 17
#define d6 18
#define d7 19

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

SdFat sd;                           //Initialise Sd card 

SdFile entry;                       //SD card file

#define filenameLength    100       //Maximum length for scrolling filename

char fileName[filenameLength + 1];    //Current filename
char sfileName[13];                 //Short filename variable
char prevSubDir[3][25];
int subdir = 0;
unsigned long filesize;             // filesize used for dimensioning AY files

//Pinout, configure at will (do not use TX and RX, used by the WiFi module)

#define chipSelect    10          //Sd card chip select pin
#define btnUp         7            //Up button
#define btnDown       8            //Down button
#define btnPlay       2            //Play Button
#define btnStop       3            //Stop Button
#define btnMenu      4             //Motor Sense (connect pin to gnd to play, NC for pause
#define btnWiFi       6            //WiFi button
#define btnRewind       5             //Return to SD card root

#define scrollSpeed   200           //text scroll delay
#define scrollWait    2000          //Delay before scrolling starts

byte scrollPos = 0;                 //Stores scrolling text position
unsigned long scrollTime = millis() + scrollWait;

byte mselectState = 1;              //Motor control state 1=on 0=off
byte start = 0;                     //Currently playing flag
byte pauseOn = 0;                   //Pause state
int currentFile = 1;                //Current position in directory
int prevFile = 0;                //Current position in directory
int maxFile = 0;                    //Total number of files in directory
byte isDir = 0;                     //Is the current file a directory
unsigned long timeDiff = 0;         //button debounce

byte UP = 0;                      //Next File, down button pressed
char PlayBytes[16];

void setup()
{


	lcd.begin(16, 2);
	lcd.display();
	lcd.clear();

	//General Pin settings
	//Setup buttons with internal pullup 
	pinMode(btnPlay, INPUT_PULLUP);
	digitalWrite(btnPlay, HIGH);
	pinMode(btnStop, INPUT_PULLUP);
	digitalWrite(btnStop, HIGH);
	pinMode(btnUp, INPUT_PULLUP);
	digitalWrite(btnUp, HIGH);
	pinMode(btnDown, INPUT_PULLUP);
	digitalWrite(btnDown, HIGH);
	pinMode(btnMenu, INPUT_PULLUP);
	digitalWrite(btnMenu, HIGH);
	pinMode(btnRewind, INPUT_PULLUP);
	digitalWrite(btnRewind, HIGH);
	pinMode(btnWiFi, INPUT_PULLUP);
	digitalWrite(btnWiFi, HIGH);

	printtextF(PSTR("Starting.."), 0);
	delay(1500);

	pinMode(chipSelect, OUTPUT);      //Setup SD card chipselect pin
	if (!sd.begin(chipSelect, SPI_FULL_SPEED))
	{
		//Start SD card and check it's working
		printtextF(PSTR("No SD Card"), 0);
		return;
	}

	sd.chdir();                       //set SD to root directory
	TZXSetup();                       //Setup TZX specific options

	lcd.clear();

	getMaxFile();                     //get the total number of files in the directory
	seekFile(currentFile);            //move to the first file in the directory

}

void loop(void)
{

	if (start == 1)
		TZXLoop();
	else
		digitalWrite(outputPin, LOW);    //Keep output LOW while no file is playing.

	if ((millis() >= scrollTime) && start == 0 && (strlen(fileName) > 15))
	{
		//Filename scrolling only runs if no file is playing to prevent I2C writes 
		//conflicting with the playback Interrupt
		scrollTime = millis() + scrollSpeed;
		scrollText(fileName);
		scrollPos += 1;
		if (scrollPos > strlen(fileName))
		{
			scrollPos = 0;
			scrollTime = millis() + scrollWait;
			scrollText(fileName);
		}
	}

	if (millis() - timeDiff > 50)
	{
		// check switch every 50ms 
		timeDiff = millis();           // get current millisecond count

		if (digitalRead(btnPlay) == LOW)
		{
			//Handle Play/Pause button
			if (start == 0)
			{
				//If no file is play, start playback
				playFile();
				delay(200);
			}
			else
			{
				//If a file is playing, pause or unpause the file                  
				if (pauseOn == 0)
				{
					printtextF(PSTR("Paused"), 0);
					itoa(newpct, PlayBytes, 10);
					strcat_P(PlayBytes, PSTR("%"));
					lcd.setCursor(8, 0);
					lcd.print(PlayBytes);
					strcpy(PlayBytes, "000");

					if ((lcdsegs % 1000) < 10)
						itoa(lcdsegs % 10, PlayBytes + 2, 10);
					else
					{
						if ((lcdsegs % 1000) < 100)
							itoa(lcdsegs % 1000, PlayBytes + 1, 10);
						else
							itoa(lcdsegs % 1000, PlayBytes, 10);
					}

					lcd.setCursor(13, 0); lcd.print(PlayBytes);

					pauseOn = 1;

				}
				else
				{

					printtextF(PSTR("Playing"), 0);
					itoa(newpct, PlayBytes, 10);
					strcat_P(PlayBytes, PSTR("%"));
					lcd.setCursor(8, 0);
					lcd.print(PlayBytes);
					strcpy(PlayBytes, "000");

					if ((lcdsegs % 1000) < 10)
						itoa(lcdsegs % 10, PlayBytes + 2, 10);
					else
					{
						if ((lcdsegs % 1000) < 100)
							itoa(lcdsegs % 1000, PlayBytes + 1, 10);
						else
							itoa(lcdsegs % 1000, PlayBytes, 10);
					}

					lcd.setCursor(13, 0);
					lcd.print(PlayBytes);

					pauseOn = 0;
				}
			}

			while (digitalRead(btnPlay) == LOW)
				delay(50);
		}

		//If playing, rewind the tape
		if (digitalRead(btnRewind) == LOW && start == 1)
		{
			printtextF(PSTR("REWind ALL..    "), 0);
			delay(500);
			printtextF(PSTR("Playing         "), 0);

			currpct = 100;
			lcdsegs = 0;
			currentBit = 0;
			pass = 0;
			bytesRead = 0;                                //start of file
			currentBlockTask = READPARAM;               //First block task is to read in parameters
			currentTask = GETFILEHEADER;                  //First task: search for header
			checkForEXT(sfileName);

			while (digitalRead(btnRewind) == LOW)
				delay(50);
		}

		//Stop the playback
		if (digitalRead(btnStop) == LOW && start == 1)
		{
			stopFile();

			while (digitalRead(btnStop) == LOW)
				delay(50);

		}

		//If we are in a folder, the stop button returns to the root folder
		if (digitalRead(btnStop) == LOW && start == 0 && subdir > 0)
		{
			fileName[0] = '\0';
			prevSubDir[subdir - 1][0] = '\0';
			subdir--;

			switch (subdir)
			{
				case 1:
					sd.chdir(strcat(strcat(fileName, "/"), prevSubDir[0]), true);
					break;
				case 2:
					sd.chdir(strcat(strcat(strcat(strcat(fileName, "/"), prevSubDir[0]), "/"), prevSubDir[1]), true);
					break;
				case 3:
					sd.chdir(strcat(strcat(strcat(strcat(strcat(strcat(fileName, "/"), prevSubDir[0]), "/"), prevSubDir[1]), "/"), prevSubDir[2]), true);
					break;
				default:
					sd.chdir("/", true);
			}

			getMaxFile();
			currentFile = 1;
			seekFile(currentFile);

			while (digitalRead(btnStop) == LOW)
				delay(50);

		}

		//Change to previous record
		if (digitalRead(btnUp) == LOW && start == 0)
		{
			int timePassed = 0;
			bool tenDone = false;

			//wait a delay, if the button is mantained jump ten records (directory fast forward)
			while (digitalRead(btnUp) == LOW)
			{
				delay(50);
				timePassed += 50;

				if (timePassed >= 1000)
				{
					timePassed = 0;
					scrollTime = millis() + scrollWait;
					scrollPos = 0;
					upTenFile();
					tenDone = true;
					break;
				}
			}

			//If we have jumped ten records, keep increasing by ten while the button is not released
			if (tenDone)
			{
				while (digitalRead(btnUp) == LOW)
				{
					delay(50);
					timePassed += 50;

					if (timePassed >= 500)
					{
						timePassed = 0;
						scrollTime = millis() + scrollWait;
						scrollPos = 0;
						upTenFile();
					}
				}
			}
			else
			{
				//Move up a record
				scrollTime = millis() + scrollWait;
				scrollPos = 0;
				upFile();

				while (digitalRead(btnUp) == LOW)
					delay(50);
			}
		}

		//Change to next record
		if (digitalRead(btnDown) == LOW && start == 0)
		{
			int timePassed = 0;
			bool tenDone = false;

			//wait a delay, if the button is mantained jump ten records (directory fast forward)
			while (digitalRead(btnDown) == LOW)
			{
				delay(50);
				timePassed += 50;

				if (timePassed >= 1000)
				{
					timePassed = 0;
					scrollTime = millis() + scrollWait;
					scrollPos = 0;
					downTenFile();
					tenDone = true;
					break;
				}
			}

			//If we have jumped ten records, keep increasing by ten while the button is not released
			if (tenDone)
			{

				while (digitalRead(btnDown) == LOW)
				{
					delay(50);
					timePassed += 50;

					if (timePassed >= 500)
					{
						timePassed = 0;
						scrollTime = millis() + scrollWait;
						scrollPos = 0;
						downTenFile();
					}
				}
			}
			else
			{
				//Move down a file in the directory
				scrollTime = millis() + scrollWait;
				scrollPos = 0;
				downFile();

				while (digitalRead(btnDown) == LOW)
					delay(50);
			}

		}

		//Start wifi server
		if (digitalRead(btnWiFi) == LOW && start == 0)
		{
			StartWiFi();
			printtextF(PSTR(VERSION), 0);

			printtextF(PSTR("                "), 1);
			scrollPos = 0;
			scrollText(fileName);

			while (digitalRead(btnWiFi) == LOW)
				delay(50);

			strcpy("ROOT", fileName);
			changeDir();
		}

	}
}

void upFile()
{
	//move up a file in the directory
	currentFile--;
	if (currentFile < 1)
	{
		getMaxFile();
		currentFile = maxFile;
	}

	UP = 1;
	seekFile(currentFile);
}

void upTenFile()
{
	//move up a record in the directory
	currentFile -= 10;
	if (currentFile < 1)
	{
		getMaxFile();
		currentFile = maxFile;
	}

	UP = 1;
	seekFile(currentFile);
}

void downFile()
{
	//move down a record in the directory
	currentFile++;
	if (currentFile > maxFile) { currentFile = 1; }
	UP = 0;
	seekFile(currentFile);
}

void downTenFile()
{
	//move down ten records in the directory
	currentFile += 10;
	if (currentFile > maxFile) { currentFile = 1; }
	UP = 0;
	seekFile(currentFile);
}

void seekFile(int pos)
{
	//move to a set position in the directory, store the filename, and display the name on screen.
	if (UP == 1)
	{
		entry.cwd()->rewind();
		for (int i = 1; i <= currentFile - 1; i++)
		{
			entry.openNext(entry.cwd(), O_READ);
			entry.close();
		}
		prevFile = currentFile;
	}

	if (currentFile == 1)
	{
		entry.cwd()->rewind();
		entry.openNext(entry.cwd(), O_READ);
		prevFile = 1;
	}
	else
	{
		entry.openNext(entry.cwd(), O_READ);
		prevFile++;

		while (prevFile < currentFile)
		{
			entry.close();
			entry.openNext(entry.cwd(), O_READ);
			prevFile++;
		}
	}
	
	entry.getName(fileName, filenameLength);
	entry.getSFN(sfileName);
	filesize = entry.fileSize();
	ayblklen = filesize + 3;  // add 3 file header, data byte and chksum byte to file length
	if (entry.isDir() || !strcmp(sfileName, "ROOT")) { isDir = 1; }
	else { isDir = 0; }
	entry.close();

	PlayBytes[0] = '\0';

	if (isDir == 1)
		strcat_P(PlayBytes, PSTR(VERSION));
	else
		strcat_P(PlayBytes, PSTR("Select File.."));

	printtext(PlayBytes, 0);

	scrollPos = 0;
	scrollText(fileName);
}

//Stop playback
void stopFile()
{
	TZXStop();
	if (start == 1)
	{
		printtextF(PSTR("Stopped"), 0);
		start = 0;
	}
}

//Start playback
void playFile()
{
	if (isDir == 1)
		changeDir();
	else
	{
		if (entry.cwd()->exists(sfileName))
		{
			printtextF(PSTR("Playing         "), 0);

			scrollPos = 0;
			pauseOn = 0;
			scrollText(fileName);
			currpct = 100;
			lcdsegs = 0;
			TZXPlay(sfileName);           //Load using the short filename
			start = 1;
		}
		else
			printtextF(PSTR("No File Selected"), 1);

	}
}

//Get max files in folder
void getMaxFile()
{
	entry.cwd()->rewind();
	maxFile = 0;

	while (entry.openNext(entry.cwd(), O_READ))
	{
		entry.close();
		maxFile++;
	}

	entry.cwd()->rewind();
}

//Change to the selected folder
void changeDir()
{

	if (!strcmp(fileName, "ROOT"))
	{
		subdir = 0;
		sd.chdir(true);
	}
	else
	{
		if (subdir > 0) entry.cwd()->getName(prevSubDir[subdir - 1], filenameLength); // Antes de cambiar  
		sd.chdir(fileName, true);
		subdir++;
	}

	getMaxFile();
	currentFile = 1;
	seekFile(currentFile);
}

//Marquee text
void scrollText(char* text)
{
	if (scrollPos < 0) scrollPos = 0;
	char outtext[17];

	if (isDir)
	{
		outtext[0] = 0x3E;
		for (int i = 1; i < 16; i++)
		{
			int p = i + scrollPos - 1;
			if (p < strlen(text))
			{
				outtext[i] = text[p];
			}
			else
			{
				outtext[i] = '\0';
			}
		}
	}
	else
	{
		for (int i = 0; i < 16; i++)
		{
			int p = i + scrollPos;
			if (p < strlen(text))
			{
				outtext[i] = text[p];
			}
			else
			{
				outtext[i] = '\0';
			}
		}
	}
	outtext[16] = '\0';
	printtext(outtext, 1);

}

//Print text to screen from a PROG string. 
void printtextF(const char* text, int l)
{  
	

	lcd.setCursor(0, l);
	lcd.print(F("                    "));
	lcd.setCursor(0, l);
	lcd.print(reinterpret_cast <const __FlashStringHelper *> (text));

}

//Print text to screen. 
void printtext(char* text, int l)
{  
	lcd.setCursor(0, l);
	lcd.print(F("                    "));
	lcd.setCursor(0, l);
	lcd.print(text);
}
