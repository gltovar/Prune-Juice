// Simple WAV file player example
//
// Requires the audio shield:
//   http://www.pjrc.com/store/teensy3_audio.html
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <play_sd_mp3.h>
#include <FastLED.h>
#include "ClickEncoder.h"
#include "SM.h"
#include <math.h>
#include <Entropy.h>

//pin defines
#define PIN_CLK					14
#define PIN_MOSI				7
#define PIN_NEO_PIXEL			6
#define PIN_ENCODER_A_PIN		2  //todo change
#define PIN_ENCODER_B_PIN		3  //todo change
#define PIN_ENCODER_BUTTON_PIN	4  //todo change

// number definitions
#define NUM_LEDS 12
#define ENCODER_STEPS_PER_NOTCH	4


#define MICROS_ONE_SECOND 1000000

#define concat(first, second) first second
#define DIR_MUSIC "/music/"
#define DIR_CACHE "/cache/"

#define CACHE_MUSIC	concat(DIR_CACHE,"music.txt")


// GUItool: begin automatically generated code
AudioPlaySdMp3           playMp31;       //xy=154,78
AudioOutputI2S           i2s1;           //xy=334,89
AudioConnection          patchCord1(playMp31, 0, i2s1, 0);
AudioConnection          patchCord2(playMp31, 1, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=240,153
// GUItool: end automatically generated code

ClickEncoder clickEncoder(	PIN_ENCODER_A_PIN,
							PIN_ENCODER_B_PIN,
							PIN_ENCODER_BUTTON_PIN,
							ENCODER_STEPS_PER_NOTCH);

CRGB leds[NUM_LEDS];
uint8_t currentLed = 0;

SM coreloop(stateInitialize);

bool workingInput		= false;
bool workingMusic		= false;
bool workingDisplay		= false;
bool workingRTC			= false;
bool workingSD			= false;

unsigned long microsDeltaTime = 0;
int16_t	inputPrev = 0;
bool inputActive = false;

long stateTimeCurrent = 0;
long stateDurationVolumeChange = 2 * MICROS_ONE_SECOND;


float volumeCurrent = 15.0f;
float volumeMin = 0.0f;
float volumeMax = 50.0f;
float volumeOffset = 14.0f;
float volumeScale = -7.1f;

uint16_t* musicPlayOrder = 0;
uint16_t* myArraySize = 0;


void setup() {
	Serial.begin(9600);
	delay(3000);
}

void playFile(const char *filename)
{
	Serial.print("Playing file: ");
	Serial.println(filename);

	// Start playing the file.  This sketch continues to
	// run while the file plays.
	playMp31.play(filename);

	// Simply wait for the file to finish playing.
	/*while (playMp31.isPlaying()) {
		// uncomment these lines if your audio shield
		// has the optional volume pot soldered
		//float vol = analogRead(15);
		//vol = vol / 1024;
		// sgtl5000_1.volume(vol);

#if 0	
		Serial.print("Max Usage: ");
		Serial.print(playMp31.processorUsageMax());
		Serial.print("% Audio, ");
		Serial.print(playMp31.processorUsageMaxDecoder());
		Serial.print("% Decoding max, ");

		Serial.print(playMp31.processorUsageMaxSD());
		Serial.print("% SD max, ");

		Serial.print(AudioProcessorUsageMax());
		Serial.println("% All");

		AudioProcessorUsageMaxReset();
		playMp31.processorUsageMaxReset();
		playMp31.processorUsageMaxResetDecoder();
#endif 

		delay(250);
	}*/
}

static uint8_t hue = 0;
void loop() 
{
	//Serial.println("testing");
	if (stateTimeCurrent > 0)
	{
		stateTimeCurrent -= microsDeltaTime;
	}

	unsigned long l_microsStart = micros();

	EXEC(coreloop);
	updateInput();

	unsigned long l_microsEnd = micros();

	// just in case of rollover, probably safe to use previous deltatime for one iteration;
	if (l_microsEnd > l_microsStart)
	{
		microsDeltaTime = l_microsEnd - l_microsStart;
	}
}

void updateInput()
{
	clickEncoder.service();
	int16_t l_currentInput = clickEncoder.getValue();
	if (l_currentInput != 0)
	{
		inputPrev = l_currentInput;
		inputActive = true;
	}
	else
	{
		inputPrev = 0;
		inputActive = false;
	}
}

State stateInitialize()
{
	clickEncoder.setAccelerationEnabled(false);
	workingInput = true;

	FastLED.addLeds<NEOPIXEL, PIN_NEO_PIXEL>(leds, NUM_LEDS);
	clearDisplay();
	workingDisplay = true;

	// Audio connections require memory to work.  For more
	// detailed information, see the MemoryAndCpuUsage example
	AudioMemory(5);
	sgtl5000_1.enable();
	sgtl5000_1.volume(0.5);
	workingMusic = true;

	SPI.setMOSI(PIN_MOSI);
	SPI.setSCK(PIN_CLK);
	if (!(SD.begin(10))) {
		// stop here, but print a message repetitively
		while (1) {
			Serial.println("Unable to access the SD card");
			delay(500);
			workingSD = false;
		}
	}
	workingSD = true;

	Serial.println("starting statePopulateMusic");

	coreloop.Set(statePopulateMusic);
	//coreloop.Set(stateIdle);
}

uint16_t musicTotalCount = 0;

byte tmp;

State statePopulateMusic()
{
	bool l_startIdle = false;
	char* l_currentFileName;

	File l_musicDirectory = SD.open(DIR_MUSIC, 0);

	if (SD.exists(CACHE_MUSIC))
	{
		SD.remove(CACHE_MUSIC);
	}
	

	while (true)
	{

		File l_entry = l_musicDirectory.openNextFile();
		Serial.println(l_entry.name());
		if (!l_entry)
		{
			break;
		}

		File l_musicCache = SD.open(CACHE_MUSIC, FILE_WRITE);
		if (!l_musicCache)
		{
			Serial.println("Problem creating music cache file");
		}

		l_musicCache.println(l_entry.name());
		l_musicCache.close();
		
		++musicTotalCount;
	}

	if (musicPlayOrder != 0)
	{
		delete[] musicPlayOrder;
	}

	musicPlayOrder = new uint16_t[musicTotalCount];


	uint16_t i = 0;

	for (i = 0; i <= musicTotalCount; ++i)
	{
		musicPlayOrder[i] = i;
	}

	Entropy.Initialize();
	Serial.print("Starting to draw random sequences from 1 to ");
	Serial.println(musicTotalCount);


	for (i = 0; i <= musicTotalCount; ++i)
	{
		tmp = Entropy.random(1, (musicTotalCount + 1) - i);
		Serial.print(musicPlayOrder[tmp]);
		if (i < musicTotalCount)
		{
			Serial.print(",");
		}
		swap(tmp, (musicTotalCount + 1) - i);
	}

	Serial.println("starting idle");
	playMp31.play("track001.mp3");
	coreloop.Set(stateIdle);
}

void swap(uint16_t a, uint16_t b)
{
	uint16_t t = musicPlayOrder[a];
	musicPlayOrder[a] = musicPlayOrder[b];
	musicPlayOrder[b] = t;
}

State stateIdle()
{
	/*int prevLed = currentLed;
	++currentLed;
	if (currentLed >= NUM_LEDS)
	{
		currentLed = 0;
	}
	leds[prevLed] = CHSV(hue, 255, 60);
	leds[currentLed] = CHSV(++hue, 255, 120);
	FastLED.show();
	delay(16);*/

	if (clickEncoder.getButton() == ClickEncoder::Clicked)
	{
		Serial.println("click detected");
		if (playMp31.isPlaying())
		{
			playMp31.stop();
		}
		else
		{
			playMp31.play("track001.mp3");
		}
	}

	if (inputActive)
	{
		stateTimeCurrent = stateDurationVolumeChange;
		coreloop.Set(stateVolumeChange);
	}
}

State stateVolumeChange()
{
	if (inputActive)
	{
		stateTimeCurrent = stateDurationVolumeChange;
	}

	if ((-inputPrev < 0 && volumeCurrent > volumeMin) ||
		(-inputPrev > 0 && volumeCurrent < volumeMax))
	{
		volumeCurrent += (float)-inputPrev;
		//float l_volume = fscale(volumeMin, volumeMax, 0.0f, 1.0f, volumeCurrent + volumeOffset, volumeScale);

		
		//Serial.println(l_volume);
		sgtl5000_1.volume(1.0f-(volumeCurrent / volumeMax));

		// if inpute is detected reset the current state's duration
		stateTimeCurrent = stateDurationVolumeChange;
	}

	// go though and visualize the volume
	int8_t l_startingLed = 7;	// 9'oclock
	int8_t l_totalLeds = 8;		// 3'oclock
	int l_totalVolumePool = (255 * l_totalLeds);
	float l_volumePercent = 1.0f - (volumeCurrent / volumeMax);
	float l_currentVolume = l_totalVolumePool * l_volumePercent;

	float l_ledsOn = l_currentVolume / 255; // so 3.27  is 3 full LEDs on with the 4th at 27%

	int8_t l_currentLedBrightness = 0;
	int8_t l_currentLed = l_startingLed;
	for (int8_t ledIndex = 0; ledIndex < l_totalLeds; ++ledIndex)
	{
		if (l_currentLed >= NUM_LEDS)
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
			l_currentLedBrightness = 255.0f * l_ledsOn;
			l_ledsOn = 0;
		}
		else
		{
			l_currentLedBrightness = 0;
		}

		leds[l_currentLed] = CHSV(HSVHue::HUE_GREEN, 255, l_currentLedBrightness);

		++l_currentLed;
	}

	leds[6] = CRGB::Red;
	leds[3] = CRGB::Blue;

	showDisplay();

	checkForFinishedState(stateIdle, true);
}

bool checkForFinishedState(Pstate nextState, bool shouldClearDisplay)
{
	if (stateTimeCurrent <= 0)
	{
		if (shouldClearDisplay)
		{
			clearDisplay();
		}

		coreloop.Set(nextState);
		return true;
	}

	return false;
}

void clearDisplay()
{
	int8_t l_currentLed = 0;
	while (l_currentLed < NUM_LEDS)
	{

		leds[l_currentLed] = CRGB::Black;
		++l_currentLed;
	}

	FastLED.show();
}

void showDisplay()
{
	FastLED.setBrightness(25); // just to keep it powerable by the teensy for now

	FastLED.show();
	}


// http://playground.arduino.cc/Main/Fscale
float fscale(float originalMin,
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