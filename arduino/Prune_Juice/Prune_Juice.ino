#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "Adafruit_VS1053.h"
#include "RTClib.h"
#include "Adafruit_NeoPixel.h"
#include "ClickEncoder.h"
#include <math.h>

// mp3 player pins
#define CLK     9 // SPI Clock, shared with SD card
#define MISO    11 // Input data, from VS1053/SD card
#define MOSI    10 // Output data, to VS1053/SD card
#define RESET   13 // VS1053 reset pin (output)
#define CS      10 // VS1053 chip select pin (output)
#define DCS     8 // VS1053 Data/command select pin (output)
#define DREQ    0 // VS1053 Data request pin (into Arduino)
#define CARDCS  6 // Card chip select pin


// display pins
//#define DISP_CLOCKPIN A0
//#define DISP_DATAPIN  A1
//#define DISP_LATCHPIN A2
#define DISP_NEOPIXEL_PIN A2
#define LED_COUNT 12

// input pins
#define INPUT_ENCODER_A_PIN   A3
#define INPUT_ENCODER_B_PIN   A4
#define INPUT_BUTTON_PIN      A5
#define INPUT_STEPS_PER_NOTCH 4

typedef enum PruneState{
	Init = 0,
	PopulateMusic,
	Idle,
	VolumeChange,
	InvalidState
};


// systems def
Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(RESET, CS, DCS, DREQ, CARDCS);
RTC_DS1307 rtc;
ClickEncoder clickEncoder(INPUT_ENCODER_A_PIN, INPUT_ENCODER_B_PIN, INPUT_BUTTON_PIN, INPUT_STEPS_PER_NOTCH);
Adafruit_NeoPixel ring = Adafruit_NeoPixel(LED_COUNT, DISP_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
//PJDisplayManager displayManager = PJDisplayManager(12);

int dumbInc = 0;
// 36 37 38
int8_t prevInput = 0;
bool inputChanged = false;
ClickEncoder::Button prevButtonState = ClickEncoder::Open;

bool displayWorking     = false;
bool musicPlayerWorking = false;
bool rtcWorking         = false;
bool sdWorking          = false;
bool inputWorking       = false;

PruneState state = InvalidState;

unsigned long millisStart = 0;
unsigned long millisEnd = 0;
unsigned long deltaTime = 0;
long currentStateTime = 0;

float volumeCurrent = 15.0f;
float volumeMin = 0.0f;
float volumeMax = 50.0f;
float volumeOffset = 14.0f;
float volumeScale = -7.1f;

unsigned long volumeStateDurationMax = 2000000;

uint8_t timeCurrentHour;
uint8_t timeCurrentMinute;
uint8_t timeCurrentSecond;


//SM coreLoop(stateInitSystem);

void setup() 
{
  Serial.begin(9600);
  Wire.begin();
}

void loop()
{
	if(millisEnd > millisStart) // just in case of rollover, safer to use last delta
	{
		deltaTime = millisEnd - millisStart;
	}
  
	millisStart = micros();

	switch (state)
	{
	case Init:
		stateInitSystem();
		break;
	case PopulateMusic:
		statePopulateMusicList();
		break;
	case Idle:
		stateIdle();
		break;
	case VolumeChange:
		stateVolumeChange();
		break;
	}

	updateInput();
	if (currentStateTime >= 0)
	{
		Serial.println(currentStateTime);
		currentStateTime -= deltaTime;
	}
  
	//updateInput();
	//updateDisplay();

	millisEnd = micros();
}

// arduino doesn't deal with enum function types too well. 
// https://fowkc.wordpress.com/2013/12/04/how-the-arduino-ide-tries-to-be-too-helpful/
void changeState(int newState) 
{
	state = (PruneState)newState;
}

void notifyError()
{
  Serial.println("something went wrong");

  if(displayWorking)
  {
    // not working visual output
  }

  if(musicPlayerWorking)
  {
    musicPlayer.sineTest(0x44, 500); // Make a tone to indicate VS1053 is working
  }
}

bool loadPreferences()
{
  // load the save file and apply settings to systems as needed
  musicPlayer.setVolume(20,20);

  return true; // TODO: in the future there will be some checks for files on the sdcard
}

void savePreferences()
{
  //  save prefs to file, like volume brightness, colors, dst date
}

void playStartUpSequence()
{
  // a sound and visual change that all is well with the init
}

void updateInput()
{
    clickEncoder.service();
    int l_currentInput = clickEncoder.getValue();
    if( l_currentInput != prevInput)
    {
		// check if +1 or -1
		prevInput = l_currentInput;
		inputChanged = true;
    }
	else
	{
		inputChanged = false;
	}

	/*ClickEncoder::Button l_currentButtonState = clickEncoder.getButton();
	if (l_currentInput != prevButtonState)
	{
		prevButtonState = l_currentButtonState;
	}*/
}

void stateInitSystem()
{
	clickEncoder.setAccelerationEnabled(false);
	inputWorking = true;

	pinMode(DISP_NEOPIXEL_PIN, OUTPUT);
	ring.begin();
	ring.setBrightness(10);
	ring.show();

	displayWorking = true;

	for (int l_ledIndex = 0; l_ledIndex < ring.numPixels(); ++l_ledIndex)
	{
		if (ring.getPixelColor(l_ledIndex) != 0)
		{
			displayWorking = false;
			notifyError();
		}
	}

	// music init
	if (!musicPlayer.begin())
	{
		Serial.println("VS1053 not found");
		notifyError();
	}
	else
	{
		musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int
		musicPlayerWorking = true;
	}

	// rtc init
	rtc.begin();
	if (!rtc.isrunning())
	{
		Serial.println("RTC is NOT running!");
		notifyError();
	}
	else
	{
		rtcWorking = true;
		//rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
		timeCurrentHour = rtc.now().hour();
		timeCurrentMinute = rtc.now().minute();
		timeCurrentSecond = rtc.now().second();
	}

	// SD init
	SD.begin(CARDCS); // initialise the SD card
	if (!loadPreferences())
	{
		notifyError();
	}
	else
	{
		sdWorking = true;
	}

	deltaTime = 0;
	millisStart = 0;
	millisEnd = 0;
	
	changeState(PopulateMusic);
}

uint16_t musicTotalCount;

void statePopulateMusicList()
{
	bool l_startIdle = false;
	
	File l_musicDirector = SD.open("/music/", 0);

	// am going to test turning on the LEDS during boot up but if it works then i dont have to do this async
	while (true)
	{
		File l_entry = l_musicDirector.openNextFile();
		if (!l_entry)
		{
			break;
		}

		//l_musi
	}

	//if (l_startIdle)
	{
		// when finished start idle
		Serial.println("starting idle");
		changeState(Idle);
	}
}

void stateIdle()
{
	// if clicked stop/start music
	if (clickEncoder.getButton() == ClickEncoder::Clicked)
	{
		Serial.println("click detected");
		if( musicPlayer.playingMusic )
		{
			musicPlayer.stopPlaying();
		}
		else
		{
			musicPlayer.startPlayingFile("track001.mp3");
		}
	}

	if (prevInput != 0)
	{
		changeState(VolumeChange);
	}


	// redraw clock
	if (timeCurrentSecond != rtc.now().second())
	{
		clearDisplay();

		timeCurrentHour = rtc.now().hour();
		timeCurrentMinute = rtc.now().minute();
		timeCurrentSecond = rtc.now().second();

		uint8_t l_hourLed = (timeCurrentHour > 10) ? timeCurrentHour - 12 : timeCurrentHour;
		uint8_t l_minuteLed = timeCurrentMinute / 12;
		bool l_minuteShouldBeOff = timeCurrentSecond % 2 == 0;

		ring.setPixelColor(l_hourLed, 255, 0, 0);
		if (l_minuteShouldBeOff)
		{
			ring.setPixelColor(l_minuteLed, 0, 0, 255);
		}
		
		ring.show();
	}
}

void stateVolumeChange()
{
	// if rotory encoder twisted start volume menu;
	if ((-prevInput < 0 && volumeCurrent != volumeMin) || (-prevInput > 0 && volumeCurrent != volumeMax))
	{

		volumeCurrent += (float)-prevInput;
		uint8_t l_volume = fscale(volumeMin, volumeMax, 0.0f, 254.0f, volumeCurrent + volumeOffset, volumeScale);

		musicPlayer.setVolume(l_volume, l_volume);

		

		currentStateTime = volumeStateDurationMax;

		// go though and visualize the volume
		int8_t l_startingLed = 8;	// 9'oclock
		int8_t l_totalLeds = 6;		// 3'oclock
		int l_totalVolumePool = (255 * l_totalLeds);
		float l_volumePercent = 1.0f-(volumeCurrent / volumeMax);
		float l_currentVolume = l_totalVolumePool * l_volumePercent;

		float l_ledsOn = l_currentVolume / 255;   // so 3.27  is 3 full LEDs on with the 4th at 27%


		Serial.print("rotation detected: ");
		Serial.print(prevInput);
		Serial.print(", current volume: ");
		Serial.println(l_volume);
		Serial.print(", total volume poo: ");
		Serial.println(l_totalVolumePool);
		Serial.print(", volume%:  ");
		Serial.println(l_volumePercent);
		Serial.print(", volume pool amount  ");
		Serial.println(l_currentVolume);
		Serial.print(", l_ledsOn  ");
		Serial.println(l_ledsOn);
		Serial.print("------------");
		

		//float l_stepsPerLed = (float)l_totalLeds / volumeMax;
		int8_t l_currentLedBrightness = 0;
		int8_t l_currentLed = l_startingLed;
		for (int8_t ledIndex = 0; ledIndex < l_totalLeds; ++ledIndex)
		{
			if (l_currentLed >= LED_COUNT)
			{
				l_currentLed = 0;
			}

			if (l_ledsOn >= 1.0f)
			{
				l_currentLedBrightness = 255;
				l_ledsOn -= 1.0f;
			}
			else if (l_ledsOn > 0)
			{
				l_currentLedBrightness = ((float)255 * l_ledsOn);
				l_ledsOn = 0;
			}
			else
			{
				l_currentLedBrightness = 0;
			}

			ring.setPixelColor(l_currentLed, 0, l_currentLedBrightness, 0);
			++l_currentLed;
		}

		ring.show();
	}

	if (currentStateTime <= 0)
	{
		clearDisplay();

		changeState(Idle);
	}
}

void clearDisplay()
{
	for (int8_t ledIndex = 0; ledIndex < LED_COUNT; ++ledIndex)
	{
		ring.setPixelColor(ledIndex, 0, 0, 0);
	}
	ring.show();
}

// http://playground.arduino.cc/Main/Fscale
float fscale(	float originalMin,
				float originalMax, 
				float newBegin,
				float newEnd,
				float inputValue,
				float curve){

	float OriginalRange = 0;
	float NewRange = 0;
	float zeroRefCurVal = 0;
	float normalizedCurVal = 0;
	float rangedValue = 0;
	boolean invFlag = 0;


	// condition curve parameter
	// limit range

	if (curve > 10) curve = 10;
	if (curve < -10) curve = -10;

	curve = (curve * -.1); // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
	curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

	/*
	Serial.println(curve * 100, DEC);   // multply by 100 to preserve resolution
	Serial.println();
	*/

	// Check for out of range inputValues
	if (inputValue < originalMin) {
		inputValue = originalMin;
	}
	if (inputValue > originalMax) {
		inputValue = originalMax;
	}

	// Zero Refference the values
	OriginalRange = originalMax - originalMin;

	if (newEnd > newBegin){
		NewRange = newEnd - newBegin;
	}
	else
	{
		NewRange = newBegin - newEnd;
		invFlag = 1;
	}

	zeroRefCurVal = inputValue - originalMin;
	normalizedCurVal = zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

	/*
	Serial.print(OriginalRange, DEC);
	Serial.print("   ");
	Serial.print(NewRange, DEC);
	Serial.print("   ");
	Serial.println(zeroRefCurVal, DEC);
	Serial.println();
	*/

	// Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
	if (originalMin > originalMax) {
		return 0;
	}

	if (invFlag == 0){
		rangedValue = (pow(normalizedCurVal, curve) * NewRange) + newBegin;

	}
	else     // invert the ranges
	{
		rangedValue = newBegin - (pow(normalizedCurVal, curve) * NewRange);
	}

	return rangedValue;
}