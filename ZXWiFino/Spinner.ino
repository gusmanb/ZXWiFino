void lcdSpinner()
{
	if (millis() - timeDiff2 > 1000)
	{
		timeDiff2 = millis();           // get current millisecond count

		lcd.setCursor(15, 0);
		lcd.print(indicators[spinpos++]);

		if (spinpos > 3)
		{
			spinpos = 0;
		}
	}
}

void lcdTime()
{
	if (millis() - timeDiff2 > 1000)
	{
		timeDiff2 = millis();

		if (lcdsegs % 10 != 0)
		{
			itoa(lcdsegs % 10, PlayBytes, 10);
			lcd.setCursor(15, 0);
			lcd.print(PlayBytes);
		}
		else
		{
			if (lcdsegs % 100 != 0)
			{
				itoa(lcdsegs % 100, PlayBytes, 10);
				lcd.setCursor(14, 0);
				lcd.print(PlayBytes);
			}
			else
			{
				if (lcdsegs % 1000 != 0)
				{
					itoa(lcdsegs % 1000, PlayBytes, 10);
					lcd.setCursor(13, 0);
					lcd.print(PlayBytes);
				} // es 100,200,300,400,500,600,700,800,900,1100,..
				else
				{
					lcd.setCursor(13, 0);
					lcd.print("000");
				} // es 000,1000,2000,...
			}

		}

		lcdsegs++;

	}
}
