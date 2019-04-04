
#include <SdFat.h>

#define OFFSET_IP 14
#define OFFSET_LEN 7

#define WAITING_CONNECTION 0
#define WAITING_NAME 1
#define READING_DATA 2
//No idea why but with a bigger buffer the program crashes randomly, it seems the "used memory" is not totally accurate and we're out of space
#define BUFFER_SIZE 150

static char buffer[BUFFER_SIZE];
static int state = 0;

inline void StartWiFi()
{
	//Start the hardware serial port at 115200 (default config for the ESP-01)
	Serial.begin(115200);

	lcd.clear();
	printtextF(PSTR("Starting WiFi..."), 0);

	Serial.setTimeout(60000);
	ClearSerial();
	SendCommand(PSTR("AT+RST"));//!!!!! Reset the ESP-01 device for a clean startup
	delay(2000);
	ClearSerial();
	SendCommand(PSTR("ATE0")); //Disable echo
	ClearSerial();
	delay(500);
	ClearSerial();

	lcd.clear();
	SendCommand(PSTR("AT+CWMODE=1")); //Station mode

	if (!CheckOK())
	{
		printtextF(PSTR("Error!"), 1);
		Serial.end();
		delay(5000);
		return;
	}

	lcd.clear();
	printtextF(PSTR("Registering..."), 0);
	SendCommand(PSTR("AT+CWJAP_CUR=\"" ACCES_POINT "\",\"" PASSWORD "\"")); //Register with the access point
	
	if (!CheckOK())
	{
		printtextF(PSTR("Error!"), 1);
		Serial.end();
		delay(5000);
		return;
	}

	printtextF(PSTR("Starting server..."), 0);
	
	SendCommand(PSTR("AT+CIPMUX=1")); //Allow multiple connections, forced by the server mode
	delay(100);
	while(Serial.available())
		ReadLine();

	SendCommand(PSTR("AT+CIPSERVER=1,9090")); //Start server on port 9090

	if (!CheckOK())
	{
		printtextF(PSTR("Error!"), 1);
		Serial.end();
		delay(5000);
		return;
	}

	SendCommand(PSTR("AT+CIPSTO=0")); //Disable timeout, we already take care of it
	
	delay(100);

	ClearSerial();
	
	SendCommand(PSTR("AT+CIFSR")); //Get IP address

	if (!ReadLine())
	{
		printtextF(PSTR("Error!"), 1);
		Serial.end();
		delay(5000);
		return;
	}

	CleanIP();
	delay(500);
	ClearSerial();
	printtextF(PSTR("Server running"), 0);
	printtext(buffer, 1); //Show IP to the user
	ServerLoop();
}

//Show an error code to the user
inline void ShowError(uint8_t ErrorCode)
{
	itoa(ErrorCode, buffer, 10);
	printtextF(PSTR("Error!"), 0);
	printtext(buffer, 1);
	delay(5000);
}

//Clear any pending data on the serial buffer
inline void ClearSerial()
{
	while (Serial.available())
		Serial.read();
}

//Read a line from the serial port
inline bool ReadLine()
{
	memset(buffer, 0, BUFFER_SIZE);
	int pos = 0;


	while (true)
	{
		int loopCount = 0;

		while (!Serial.available())//No data, wait...
		{
			loopCount++;
			delay(1);

			if (loopCount > 30000) //Timeout!
				return false;
			
		}

		while (Serial.available() && pos < BUFFER_SIZE - 1)//While something to read...
		{
			char val = (char)Serial.read(); //Store in buffer
			buffer[pos++] = val;

			if (val == '\n') //Line terminator?
			{
				buffer[pos] = '\0'; //Remove \r and place a string terminator
				return true;
			}
		}

		if (pos >= BUFFER_SIZE - 1) //Overflow!!
			return false;
	}

	return false;
}

//Wait for an OK response
inline bool CheckOK()
{
	memset(buffer, 0, BUFFER_SIZE);
	int pos = 0;

	bool found = false;

	while (true)
	{
		if (!ReadLine()) //Timeout!
		{
			ShowError(2);
			return false;
		}

		if (!strcmp_P(buffer, PSTR("OK\r\n")) || !strcmp_P(buffer, PSTR("SEND OK\r\n"))) //It's an ok?
			return true;
		else if (!strcmp_P(buffer, PSTR("ERROR\r\n")) || !strcmp_P(buffer, PSTR("FAIL\r\n")))//It's an error?
			return false;
	}

	return false;

}

//Send a command to the ESP-01 module
inline void SendCommand(const char* text)
{
	Serial.print((__FlashStringHelper*)text);
	Serial.print((__FlashStringHelper*)PSTR("\r\n"));
}

//Send a part of a command to the ESP-01 module
inline void SendPartialCommand(const char* text)
{
	Serial.print((__FlashStringHelper*)text);
}

//Close a link
inline void CloseLink(uint8_t LinkId)
{
	SendPartialCommand(PSTR("AT+CIPCLOSE="));
	Serial.write(LinkId);
	SendPartialCommand(PSTR("\r\n"));
	delay(2000);
	ClearSerial();
}

//Main server loop
inline void ServerLoop()
{
	while (true)
	{
		//If there is data on the serial port...
		if (Serial.available())
		{
			if (!ReadLine())//If we can't read a line, error!
			{
				ShowError(7);
				Terminate();
				return;
			}

			if (!ProcessCommand())//Process the received command or die
			{
				Terminate();
				return;
			}

		}

		//If the user presses the wifi button, exit server mode
		if (digitalRead(btnWiFi) == LOW)
		{
			Terminate();

			delay(500);

			//Wait until the user releases the button
			while (digitalRead(btnWiFi) == LOW)
				delay(50);

			return;
		}
	}
}

//Close everything
inline void Terminate()
{
	//Close the entry if open
	if (entry.isOpen())
		entry.close();

	//Close link0 if we're on a transmission
	if (state != WAITING_CONNECTION)
		CloseLink(0);

	//Tell the module to disconnect from the AP
	SendCommand(PSTR("AT+CWQAP"));
	delay(1000);
	ClearSerial();

	//Close the serial port
	Serial.end();

	state = WAITING_CONNECTION;
}

//Prepare the received IP to be shown on the display
inline void CleanIP()
{
	int pos = 0;
	char current = buffer[OFFSET_IP];

	while (current != '"')
	{
		buffer[pos] = current;
		pos++;
		current = buffer[OFFSET_IP + pos];
	}

	buffer[pos] = 0;
}

//Process an incomming command
inline bool ProcessCommand()
{

	if (buffer[0] == '+')
	{
		//Process incomming remote data
		return ProcessIncommingData();
	}
	else if (!strcmp_P(&buffer[2], PSTR("CONNECT\r\n")))
	{
		//Process a new connection
		return ProcessConnection();
	}
	else if (!strcmp_P(&buffer[2], PSTR("CLOSED\r\n")))
	{
		//Process a connection close
		return ProcessDisconnected();
	}
	else if (buffer[0] == '\r' && buffer[1] == '\n')
	{
		//Discard trash generated by the ESP-01 module (some empty lines)
		return true;
	}
	else
	{
		//No idea what we have received, error!
		ShowError(8);
		return false;
	}
}

//Process an incomming connection
inline bool ProcessConnection()
{
	if (state == WAITING_CONNECTION)
	{
		if (buffer[0] == '0')
		{
			//If we're waiting for a connection and this is the link 0, accept it
			if (SendInit())
			{
				state = WAITING_NAME;
				return true;
			}
			else
			{
				CloseLink(0);
				ShowError(6);
				return true;
			}
		}
		else
		{
			//Close the connection
			CloseLink(buffer[0]);
			return true;
		}
	}
	else
	{
		//Close the connection
		CloseLink(buffer[0]);
		return true;
	}

}

//Send the ZXWiFino identifier
inline bool SendInit()
{
	SendPartialCommand(PSTR("AT+CIPSEND=0,8\r\n")); //Inform the wifi module we're going to send data

	if (!CheckOK())
		return false;

	ClearSerial(); //Discard cursor

	SendPartialCommand(PSTR("WIFIZX\r\n")); //Send the data

	if (!CheckOK())
		return false;

	return true;
}

//Send an OK message
inline bool SendOk()
{
	SendPartialCommand(PSTR("AT+CIPSEND=0,4\r\n")); //Inform the wifi module we're going to send data

	if (!CheckOK())
		return false;

	ClearSerial();

	SendPartialCommand(PSTR("OK\r\n")); //Send the data

	if (!CheckOK())
		return false;

	return true;
}

//Disconnection received, what do we do?
inline bool ProcessDisconnected()
{
	if (buffer[0] == '0' && state > WAITING_CONNECTION) //We only care about disconnections when there is a transmission and the disconnected link is the zero one
	{
		if (entry.isOpen()) //Close the entry if it's open
			entry.close();

		//Wait for another connection
		state = WAITING_CONNECTION;
	}

	return true;
}

//Choose what to do based on the current state
inline bool ProcessIncommingData()
{
	switch (state)
	{
	case WAITING_CONNECTION: //Ignore data if we're waiting for link 0
		return true;
	case WAITING_NAME:
		return ProcessFilename(); //Try to read filename from the received data
	case READING_DATA:
		return ProcessDataPacket(); //Process a data packet
	}
}

//Process data when waiting for a filename
inline bool ProcessFilename() 
{
	int len = PrepareData();//Prepare the data to be processed

	if (len < 2)
	{
		ShowError(0);
		return false;
	}

	//CHeck the path
	if (!CheckFolders())
	{
		ShowError(1);
		return false;
	}

	//Open file
	if (!entry.open(buffer, O_CREAT | O_RDWR))
	{
		ShowError(2);
		return false;
	}

	//We're ready to receive data, inform to the client
	if (!SendOk())
	{
		ShowError(3);
		return false;
	}

	state = READING_DATA;

	return true;
}

//Checks if the target path exists and if not creates it
inline bool CheckFolders()
{
	int filePos = strlen(buffer);

	while (buffer[filePos] != '/')
		filePos--;

	buffer[filePos] = '\0';

	bool res = false;

	if (!sd.exists(buffer))
		res = sd.mkdir(buffer, true);
	else
		res = true;

	buffer[filePos] = '/';

	return res;
}

inline bool ProcessDataPacket()
{
	int len = PrepareData(); //Prepare the data in the buffer
	len = base64_decode(buffer, buffer, len); //Decode the data (only string data is accepted)

	if (len < 1) //No data?
	{
		ShowError(4);
		return false;
	}

	if (len == 1 && buffer[0] == '\0') //A packet with only one zero byte means "transfer finished"
	{
		entry.flush();
		entry.close();
		CloseLink(0); 
		state = WAITING_CONNECTION;
		return true;
	}

	//Write the data to the file
	entry.write(buffer, len);

	if (!SendOk())
	{
		ShowError(5);
		return false;
	}

	return true;
}

//Moves to the beginning of the buffer the received data and terminates it with \0
inline int PrepareData()
{
	int pos = OFFSET_LEN;

	while (buffer[pos] != ':' && pos < BUFFER_SIZE)
	{
		buffer[pos - OFFSET_LEN] = buffer[pos];
		pos++;
	}

	buffer[pos] = '\0';

	pos++;

	int len = atoi(buffer);

	if (len < 3)
		return -1;

	len -= 2;

	for (int buc = 0; buc < len; buc++)
		buffer[buc] = buffer[buc + pos];

	buffer[len] = '\0';

	return len ;
}

//Base64 functions used from https://github.com/adamvr/arduino-base64
inline int base64_decode(char * output, char * input, int inputLen)
{
	int i = 0, j = 0;
	int decLen = 0;
	unsigned char a3[3];
	unsigned char a4[4];


	while (inputLen--)
	{
		if (*input == '=')
		{
			break;
		}

		a4[i++] = *(input++);
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
			{
				a4[i] = b64_lookup(a4[i]);
			}

			a4_to_a3(a3, a4);

			for (i = 0; i < 3; i++)
			{
				output[decLen++] = a3[i];
			}
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 4; j++)
		{
			a4[j] = '\0';
		}

		for (j = 0; j < 4; j++)
		{
			a4[j] = b64_lookup(a4[j]);
		}

		a4_to_a3(a3, a4);

		for (j = 0; j < i - 1; j++)
		{
			output[decLen++] = a3[j];
		}
	}
	output[decLen] = '\0';
	return decLen;
}

inline void a4_to_a3(unsigned char * a3, unsigned char * a4)
{
	a3[0] = (a4[0] << 2) + ((a4[1] & 0x30) >> 4);
	a3[1] = ((a4[1] & 0xf) << 4) + ((a4[2] & 0x3c) >> 2);
	a3[2] = ((a4[2] & 0x3) << 6) + a4[3];
}

inline unsigned char b64_lookup(char c)
{
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 71;
	if (c >= '0' && c <= '9') return c + 4;
	if (c == '+') return 62;
	if (c == '/') return 63;
	return -1;
}
