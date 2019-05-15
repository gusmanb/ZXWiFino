int state = 0;

inline void StartWiFi()
{
	//Read WiFi settings from SD card
	if (!CheckWiFiSettings())
	{
		lcd.clear();
		printtextF(PSTR("Missing settings.."), 0);
		delay(5000);
		return;
	}

	//Start the hardware serial port at 115200 (default config for the ESP-01)
	
	lcd.clear();
	printtextF(PSTR("Starting WiFi..."), 0);

	Serial.begin(115200);
	Serial.setTimeout(60000);

	ClearSerial();

	SendCommand(PSTR("AT+RST"));//!!!!! Reset the ESP-01 device for a clean startup
	delay(2000);
	ClearSerial();
	
	SendCommand(PSTR("ATE0")); //Disable echo
	delay(500);
	ClearSerial();

	SendCommand(PSTR("AT+CWMODE=1")); //Station mode

	if (!CheckOK())
	{
		entry.close();
		printtextF(PSTR("Error!"), 1);
		Serial.end();
		delay(5000);
		return;
	}

	/*lcd.clear();
	printtextF(PSTR("Registering..."), 0);
*/
	SendWiFiSettings();

	if (!CheckOK())
	{
		printtextF(PSTR("Error!"), 1);
		Serial.end();
		delay(5000);
		return;
	}
/*
	printtextF(PSTR("Starting server..."), 0);
	*/
	SendCommand(PSTR("AT+CIPMUX=1")); //Allow multiple connections, forced by the server mode
	delay(100);
	ClearSerial();

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
	printtext(dataBuffer, 1); //Show IP to the user
	ServerLoop();
}

//Sends the WiFi AP name and password
inline void SendWiFiSettings()
{
	SendPartialCommand(PSTR("AT+CWJAP_CUR=\""));
	int read = entry.read();

	//Read unitil we reach a new line
	while (read != '\r' && read != '\n')
	{
		Serial.write(read);
		read = entry.read();
	}

	if (read == '\r')
		entry.read();

	SendPartialCommand(PSTR("\",\""));
	
	read = entry.read();

	//Read until we reach a new line or the end of the file
	while (read != '\r' && read != '\n' && read != -1)
	{
		Serial.write(read);
		read = entry.read();
	}

	entry.close();
	
	SendPartialCommand(PSTR("\"\r\n"));
}

inline bool CheckWiFiSettings()
{
	//Get to the root directory
	sd.chdir(true);

	//Get the first file name
	entry.cwd()->rewind();
	
	//No file on the SD card!
	if (!entry.openNext(entry.cwd(), O_READ))
		return false;

	entry.getName(dataBuffer, bufferLength);

	//Loop through the entries until we find our settings or reached the end of the records
	while (strcmp_P(dataBuffer, PSTR("wifi.ini")))
	{
		entry.close();

		if (!entry.openNext(entry.cwd(), O_READ)) //No more records!
			return false;

		entry.getName(dataBuffer, bufferLength);
	}
	
	//Search for a new line, the config must contain two lines, any other data after those lines will be ignored.
	int read = entry.read();

	while(read != -1 && read != '\r' && read != '\n')
		read = entry.read();

	//Not found!!
	if (read == -1)
		return false;

	entry.seekSet(0);

	return true;
}

//Show an error code to the user
inline void ShowError(uint8_t ErrorCode)
{
	itoa(ErrorCode, dataBuffer, 10);
	printtextF(PSTR("Error!"), 0);
	printtext(dataBuffer, 1);
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
	memset(dataBuffer, 0, bufferLength);
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

		while (Serial.available() && pos < bufferLength - 1)//While something to read...
		{
			char val = (char)Serial.read(); //Store in buffer
			dataBuffer[pos++] = val;

			if (val == '\n') //Line terminator?
			{
				dataBuffer[pos] = '\0'; //Place a string terminator
				return true;
			}
		}

		if (pos >= bufferLength - 1) //Overflow!!
			return false;
	}

	return false;
}

//Wait for an OK response
inline bool CheckOK()
{
	memset(dataBuffer, 0, bufferLength);
	int pos = 0;

	bool found = false;

	while (true)
	{
		if (!ReadLine()) //Timeout!
		{
			ShowError(2);
			return false;
		}

		if (!strcmp_P(dataBuffer, PSTR("OK\r\n")) || !strcmp_P(dataBuffer, PSTR("SEND OK\r\n"))) //It's an ok?
			return true;
		else if (!strcmp_P(dataBuffer, PSTR("ERROR\r\n")) || !strcmp_P(dataBuffer, PSTR("FAIL\r\n")))//It's an error?
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

			memset(dataBuffer, 0, bufferLength);

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
	char current = dataBuffer[OFFSET_IP];

	while (current != '"')
	{
		dataBuffer[pos] = current;
		pos++;
		current = dataBuffer[OFFSET_IP + pos];
	}

	dataBuffer[pos] = 0;
}

//Process an incomming command
inline bool ProcessCommand()
{

	if (dataBuffer[0] == '+')
	{
		//Process incomming remote data
		return ProcessIncommingData();
	}
	else if (!strcmp_P(&dataBuffer[2], PSTR("CONNECT\r\n")))
	{
		//Process a new connection
		return ProcessConnection();
	}
	else if (!strcmp_P(&dataBuffer[2], PSTR("CLOSED\r\n")))
	{
		//Process a connection close
		return ProcessDisconnected();
	}
	else if (dataBuffer[0] == '\r' && dataBuffer[1] == '\n')
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
		if (dataBuffer[0] == '0')
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
			CloseLink(dataBuffer[0]);
			return true;
		}
	}
	else
	{
		//Close the connection
		CloseLink(dataBuffer[0]);
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
	if (dataBuffer[0] == '0' && state > WAITING_CONNECTION) //We only care about disconnections when there is a transmission and the disconnected link is the zero one
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
	if (!entry.open(dataBuffer, O_CREAT | O_TRUNC | O_RDWR))
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
	int filePos = strlen(dataBuffer);

	while (dataBuffer[filePos] != '/')
		filePos--;

	dataBuffer[filePos] = '\0';

	bool res = false;

	if (!sd.exists(dataBuffer))
		res = sd.mkdir(dataBuffer, true);
	else
		res = true;

	dataBuffer[filePos] = '/';

	return res;
}

inline bool ProcessDataPacket()
{
	int len = PrepareData(); //Prepare the data in the buffer
	
	if (len < 1) //No data?
	{
		ShowError(4);
		return false;
	}

	if (len == 2 && dataBuffer[0] == '\xAA' && dataBuffer[1] == '\x02') //0xAA, 0x02 -> File terminator	   
	{
		entry.flush();
		entry.close();
		CloseLink(0); 
		state = WAITING_CONNECTION;
		return true;
	}

	len = Unescape(len); //Reconstruct the data

	//Write the data to the file
	entry.write(dataBuffer, len);

	

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

	while (dataBuffer[pos] != ':' && pos < bufferLength)
	{
		dataBuffer[pos - OFFSET_LEN] = dataBuffer[pos];
		pos++;
	}

	dataBuffer[pos] = '\0';

	pos++;

	int len = atoi(dataBuffer);

	if (len < 3)
		return -1;

	len -= 2;

	for (int buc = 0; buc < len; buc++)
		dataBuffer[buc] = dataBuffer[buc + pos];

	dataBuffer[len] = '\0';

	return len ;
}

//Unescape buffered data
inline int Unescape(int inputLen)
{
	int posInput = 0;
	int posOutput = 0;
	byte inputData;

	while (posInput < inputLen)
	{
		inputData = dataBuffer[posInput++];

		if (inputData == 0xAA)
		{
			if (dataBuffer[posInput++] == 0x00)
				dataBuffer[posOutput++] = 0x0A;
			else
				dataBuffer[posOutput++] = 0xAA;
		}
		else
			dataBuffer[posOutput++] = inputData;

	}

	return posOutput;
}
