#pragma region LCD type

//Uncomment your LCD type
#define LCD_16x16_4bits

//#define LCD_16x16_I2C

#ifdef LCD_16x16_I2C

	#define LCD_I2C_ADDRESS 0x3C

#endif

#pragma endregion

#pragma region General definitions

#define VERSION "ZXWiFino 1.0"

//If you have memory problems, reduce this buffer, also remember to reduce it on the client code.
//The larger it is, the more efficient the data transfer will be
#define bufferLength    230       //Data buffer length, used for file name and data transfer

#define scrollSpeed   200           //text scroll delay
#define scrollWait    2000          //Delay before scrolling starts

PROGMEM const char indicators[] = { '|', '/', '-',0 };

#pragma endregion

#pragma region Pin definitions

//Pinout, configure at will (do not use TX and RX, used by the WiFi module)
#define chipSelect    10			//Sd card chip select pin
#define btnUp         7				//Up button
#define btnDown       8				//Down button
#define btnPlay       2				//Play Button
#define btnStop       3				//Stop Button
#define btnWiFi       6				//WiFi button


#ifdef LCD_16x16_4bits

//Parallel (4 bit) LCD pin definitions
#define rsPin 14
#define enPin 15
#define d4Pin 16
#define d5Pin 17
#define d6Pin 18
#define d7Pin 19

#endif



// Audio Output PIN - Set accordingly to your hardware.
#define outputPin           9              

#pragma endregion

#pragma region WiFi stuff

#define OFFSET_IP 14
#define OFFSET_LEN 7

#define WAITING_CONNECTION 0
#define WAITING_NAME 1
#define READING_DATA 2

#pragma endregion

#pragma region Tape processing stuff

PROGMEM const char TZXTape[7] = { 'Z','X','T','a','p','e','!' };
PROGMEM const char TAPcheck[7] = { 'T','A','P','t','a','p','.' };
PROGMEM const char ZX81Filename[9] = { 'T','Z','X','D','U','I','N','O',0x9D };
PROGMEM const char AYFile[8] = { 'Z','X','A','Y','E','M','U','L' };           // added additional AY file header check
PROGMEM const char TAPHdr[20] = { 0x0,0x0,0x3,'Z','X','A','Y','F','i','l','e',' ',' ',0x1A,0xB,0x0,0xC0,0x0,0x80,0x6E }; // 
//const char TAPHdr[24] = {0x13,0x0,0x0,0x3,' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',0x1A,0xB,0x0,0xC0,0x0,0x80,0x52,0x1C,0xB,0xFF};

//TZX block list - uncomment as supported
#define ID10                0x10    //Standard speed data block
#define ID11                0x11    //Turbo speed data block
#define ID12                0x12    //Pure tone
#define ID13                0x13    //Sequence of pulses of various lengths
#define ID14                0x14    //Pure data block
//#define ID15                0x15    //Direct recording block -- TBD - curious to load OTLA files using direct recording (22KHz)
//#define ID18                0x18    //CSW recording block
//#define ID19                0x19    //Generalized data block
#define ID20                0x20    //Pause (silence) ot 'Stop the tape' command
#define ID21                0x21    //Group start
#define ID22                0x22    //Group end
#define ID23                0x23    //Jump to block
#define ID24                0x24    //Loop start
#define ID25                0x25    //Loop end
#define ID26                0x26    //Call sequence
#define ID27                0x27    //Return from sequence
#define ID28                0x28    //Select block
#define ID2A                0x2A    //Stop the tape is in 48K mode
#define ID2B                0x2B    //Set signal level
#define ID30                0x30    //Text description
#define ID31                0x31    //Message block
#define ID32                0x32    //Archive info
#define ID33                0x33    //Hardware type
#define ID35                0x35    //Custom info block
#define ID4B                0x4B    //Kansas City block (MSX/BBC/Acorn/...)
#define IDPAUSE				      0x59	  //Custom Pause processing
#define ID5A                0x5A    //Glue block (90 dec, ASCII Letter 'Z')
#define AYO                 0xFB    //AY file
#define ZXO                 0xFC    //ZX80 O file
#define ZXP                 0xFD    //ZX81 P File
#define TAP                 0xFE    //Tap File Mode
#define TZX                 0x00    //Tap File Mode
#define EOF                 0xFF    //End of file


//TZX File Tasks
#define GETFILEHEADER         0
#define GETID                 1
#define PROCESSID             2
#define GETAYHEADER           3

//TZX ID Tasks
#define READPARAM             0
#define PILOT                 1
#define SYNC1                 2
#define SYNC2                 3
#define DATA                  4
#define PAUSE                 5

//Buffer size
#define buffsize              64

//Spectrum Standards
#define PILOTLENGTH           619
#define SYNCFIRST             191
#define SYNCSECOND            210
#define ZEROPULSE             244
#define ONEPULSE              489
#define PILOTNUMBERL          8063
#define PILOTNUMBERH          3223
#define PAUSELENGTH           1000   

//ZX81 Standards
#define ZX80PULSE                 160
#define ZX80BITGAP                1442

//ZX81 Pulse Patterns - Zero Bit  - HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, GAP
//                    - One Bit   - HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, GAP

// AY Header offset start
#define HDRSTART              0

//Keep track of which ID, Task, and Block Task we're dealing with
byte currentID = 0;
byte currentTask = 0;
byte currentBlockTask = 0;

//Temporarily store for a pulse period before loading it into the buffer.
word currentPeriod = 1;

//ISR Variables
volatile byte pos = 0;
volatile word wbuffer[buffsize + 1][2];
volatile byte morebuff = HIGH;
volatile byte workingBuffer = 0;
volatile byte isStopped = false;
volatile byte pinState = LOW;
volatile byte isPauseBlock = false;
volatile byte wasPauseBlock = false;
volatile byte intError = false;

//Main Variables
byte AYPASS = 0;
byte hdrptr = 0;
byte blkchksum = 0;
word ayblklen = 0;
byte btemppos = 0;
byte copybuff = LOW;
unsigned long bytesRead = 0;
unsigned long bytesToRead = 0;
word pilotPulses = 0;
word pilotLength = 0;
word sync1Length = 0;
word sync2Length = 0;
word zeroPulse = 0;
word onePulse = 0;
byte usedBitsInLastByte = 8;
word loopCount = 0;
byte seqPulses = 0;
unsigned long loopStart = 0;
word pauseLength = 0;
word temppause = 0;
byte outByte = 0;
word outWord = 0;
unsigned long outLong = 0;
byte count = 128;
volatile byte currentBit = 0;
volatile byte currentByte = 0;
volatile byte currentChar = 0;
byte pass = 0;
byte EndOfFile = false;

byte currpct = 100;
byte newpct = 0;
byte spinpos = 0;
unsigned long timeDiff2 = 0;
unsigned int lcdsegs = 0;
int TSXspeedup = 1;
int BAUDRATE = 1200;
byte tzxduino = 0;

#pragma endregion

#pragma region CAS processing stuff

#define WRITE_LOW digitalWrite(outputPin, LOW)
#define WRITE_HIGH digitalWrite(outputPin, HIGH)

#define SHORT_SILENCE       122
#define LONG_SILENCE        SHORT_SILENCE*2

#define SHORT_HEADER        800
#define LONG_HEADER         800


/* Header Definitions */
PROGMEM const byte HEADER[8] = { 0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74 };
PROGMEM const byte ASCII[10] = { 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA };
PROGMEM const byte BINF[10] = { 0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0, 0xD0 };
PROGMEM const byte BASIC[10] = { 0xD3, 0xD3, 0xD3, 0xD3, 0xD3, 0xD3, 0xD3, 0xD3, 0xD3, 0xD3 };

#define lookHeader    0     //looking for header/data
#define lookType      1     //looking for file type
#define wHeader       2     //Write Header
#define wSilence      3     //Write Silence
#define wData         4     //Write Data
#define wClose        5     //Write closing silence

#define typeNothing   0
#define typeAscii     1
#define typeBinf      2
#define typeBasic     3
#define typeUnknown   4
#define typeEOF 5
#define period 208
#define scale 4

byte casduino = 0;
volatile byte working = 0;
byte out = LOW;
byte bits[11];
byte currentType = typeNothing;
byte fileStage = 0;
byte lastByte;


#pragma endregion