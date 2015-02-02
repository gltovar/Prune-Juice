#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>
#include <RTClib.h>
#include <Wire.h>

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
#define DISP_CLOCKPIN A0
#define DISP_DATAPIN  A1
#define DISP_LATCHPIN A2


// input pins


// systems def
Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(RESET, CS, DCS, DREQ, CARDCS);
RTC_DS1307 rtc;


int dumbInc = 0;
// 36 37 38

int numberToDisplay = 0;

bool displayWorking     = false;
bool musicPlayerWorking = false;
bool rtcWorking         = false;
bool sdWorking          = false;
bool inputWorking       = false;

void setup() 
{
  Serial.begin(9600);

  #ifdef AVR
    Wire.begin();
  #else
    Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
  #endif

  // display init
  pinMode(DISP_LATCHPIN, OUTPUT);
  pinMode(DISP_CLOCKPIN, OUTPUT);
  pinMode(DISP_DATAPIN, OUTPUT);
  displayWorking = true;

  // music init
  if (!musicPlayer.begin()) 
   {
    Serial.println("VS1053 not found");
    notifyError();
   }
   else
   {
    musicPlayerWorking = true;
   }

  // rtc init
  rtc.begin();
  if( !rtc.isrunning() )
  {
    Serial.println("RTC is NOT running!");
    notifyError();
  }
  else
  {
    musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int
    rtcWorking = true;
  }
  
  // SD init
  SD.begin(CARDCS); // initialise the SD card
  if( !loadPreferences() )
  {
    notifyError();
  }
  else
  {
    sdWorking = true;
  }

   //musicPlayer.startPlayingFile("track001.mp3");
}

void loop()
{
  ++dumbInc;

  // just a binary vidual counter
  if( dumbInc%5000 == 0)
  {
    if(++numberToDisplay >= 256)
    {
      numberToDisplay = 0; 
      updateDisplay();
    }
  }
}

void notifyError()
{
  Serial.println("something went wrong")

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

void checkInputs()
{
  // check the user input and adjust / state as needed
}

void updateCurrentState()
{

}

void updateDisplay()
{
  // an idea would be to store the last set display and only
  // if the new display is different to push it to the display

  Serial.print("number: " );
  Serial.print(numberToDisplay);
  Serial.println();

  digitalWrite(DISP_LATCHPIN, LOW);
  shiftOut(DISP_DATAPIN, DISP_CLOCKPIN, MSBFIRST, numberToDisplay);

  digitalWrite(DISP_LATCHPIN, HIGH);
}