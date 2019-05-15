void clearBufferCAS()
{

	for (int i = 0; i <= buffsize; i++)
	{
		wbuffer[i][0] = 2;
		wbuffer[i][1] = 2;
	}
}

void CASPlay()
{
	
	out = LOW;
	bytesRead = 0;
	currentType = typeNothing;
	currentTask = lookHeader;
	fileStage = 0;
	clearBufferCAS();
	isStopped = false;

	Timer1.stop();                              //Stop timer interrupt
	Timer1.initialize(period);
	Timer1.attachInterrupt(casWave);

}

void CASStop()
{
	Timer1.stop();
	isStopped = true;
	start = 0;
	entry.close();
	seekFile(currentFile);
	bytesRead = 0;
	casduino = 0;

}

void casWave()
{
	if (isStopped == 0)
	{
		switch (wbuffer[pos][working])
		{
		case 0:
			if (pass == 0 | pass == 1)
			{
				if (out == LOW)     
					WRITE_LOW;
				else  
					WRITE_HIGH;
			}
			else
			{
				if (out == LOW)     
					WRITE_HIGH;
				else  
					WRITE_LOW;
			}
			break;

		case 1:

			if (pass == 0 | pass == 2)
			{
				if (out == LOW)    
					WRITE_LOW;
				else  
					WRITE_HIGH;
			}
			else
			{
				if (out == LOW)     
					WRITE_HIGH;
				else  
					WRITE_LOW;
			}
			
			break;

		case 2:

			WRITE_LOW;
			break;
		}

		pass = pass + 1;
		if (pass == 4)
		{
			pass = 0;
			pos += 1;
			if (pos > buffsize)
			{
				pos = 0;
				working ^= 1;
				morebuff = HIGH;
			}
		}
	}
	else
		WRITE_LOW;

}

void writeByte(byte b)
{

	bits[0] = 0;
	for (int i = 1; i < 9; i++)
	{
		if (b & 1)
		{
			bits[i] = 1;
		}
		else bits[i] = 0;
		b = b >> 1;
	}
	bits[9] = 1;
	bits[10] = 1;

}

void writeSilence()
{
	for (int i = 0; i < 11; i++)
	{
		bits[i] = 2;
	}
}

void casWriteHeader()
{
	for (int i = 0; i < 11; i++)
	{
		bits[i] = 1;
	}
}

void casProcess()
{
	byte r = 0;
	if (currentType == typeEOF)
	{
		if (!count == 0)
		{
			writeSilence();
			count += -1;
		}
		else stopFile();
		return;
	}
	if (currentTask == lookHeader || currentTask == wData)
	{
		if ((r = readfile(8, bytesRead)) == 8)
		{
			if (!memcmp_P(dataBuffer, HEADER, 8))
			{
				if (fileStage == 0)
				{
					currentTask = lookType;
				}
				else
				{
					currentTask = wSilence;
					count = SHORT_SILENCE * scale;
				}
				if (currentType == typeNothing) fileStage = 1;
				bytesRead += 8;
			}

		}
		else if (r == 0)
		{
			currentType = typeEOF;
			currentTask = wClose;
			count = LONG_SILENCE * scale;
		}

	}
	if (currentTask == lookType)
	{
		if ((r = readfile(10, bytesRead)) == 10)
		{
			if (!memcmp_P(dataBuffer, ASCII, 10))
			{
				currentType = typeAscii;
				currentTask = wSilence;
				count = LONG_SILENCE * scale;
				fileStage = 1;
			}
			else if (!memcmp_P(dataBuffer, BINF, 10))
			{
				currentType = typeBinf;
				currentTask = wSilence;
				count = LONG_SILENCE * scale;
				fileStage = 1;
			}
			else if (!memcmp_P(dataBuffer, BASIC, 10))
			{
				currentType = typeBasic;
				currentTask = wSilence;
				count = LONG_SILENCE * scale;
				fileStage = 1;
			}
			else
			{
				currentType = typeUnknown;
				currentTask = wSilence;
				count = LONG_SILENCE * scale;
				fileStage = 1;
			}
		}
		else
		{
			currentType = typeUnknown;
			currentTask = wSilence;
			count = LONG_SILENCE * scale;
			fileStage = 1;
		}
	}
	if (currentTask == wSilence)
	{
		if (!count == 0)
		{
			writeSilence();
			count += -1;
		}
		else
		{
			currentTask = wHeader;
			if (fileStage == 1)
			{
				//count=LONG_HEADER*scale;
				count = LONG_HEADER * scale;
				fileStage += 1;
			}
			else
			{
				count = SHORT_HEADER * scale;
				//count=SHORT_HEADER;
				if (currentType == typeAscii)
				{
					fileStage += 1;
				}
				else
				{
					fileStage = 0;
				}
			}
		}
	}
	if (currentTask == wHeader)
	{
		if (!count == 0)
		{
			casWriteHeader();
			count += -1;
		}
		else
		{
			currentTask = wData;
			return;
		}
	}
	if (currentTask == wData)
	{
		writeByte(dataBuffer[0]);
		if (dataBuffer[0] == 0x1a && currentType == typeAscii)
		{
			fileStage = 0;
		}
	}
	if (currentTask == lookHeader || currentTask == lookType || currentTask == wData) bytesRead += 1;

}

void casduinoLoop()
{
	noInterrupts();
	copybuff = morebuff;
	morebuff = LOW;
	isStopped = pauseOn;
	interrupts();

	if (copybuff == HIGH)
	{
		btemppos = 0;
		copybuff = LOW;
	}
	if (btemppos <= buffsize)
	{

		casProcess();
		//      noInterrupts();
		for (int t = 0; t < 11; t++)
		{
			if (btemppos <= buffsize)
			{
				wbuffer[btemppos][working ^ 1] = bits[t];
				btemppos += 1;
			}
		}
		//      interrupts();

	}
	else
	{
		//lcdSpinner();
		if (pauseOn == 0)
		{
			lcdTime();
			lcdPercent();
		}
	}
}

int readfile(byte bytes, unsigned long p)
{

	int i = 0;
	int t = 0;
	if (entry.seekSet(p))
	{
		i = entry.read(dataBuffer, bytes);
	}
	return i;
}
