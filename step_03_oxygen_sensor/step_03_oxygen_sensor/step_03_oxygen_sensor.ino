#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ADS (Analog & Digital Systems) is for 15 bits resolution (0 to 32676)
// Without an ADS1115, Arduino Nano only have 10 bits resolution (0 to 1023)
#include <ADS1X15.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// The address of SSD1306 display is 0x3C and cas display 124 * 64
#define SCREEN_ADDRESS 0x3C  // 0x3C == 124 * 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

ADS1115 ADS(0x48);
// GND	0x48 => adresse par défaut
// VDD	0x49
// SDA	0x4A
// SCL	0x4B

const int readDelay = 3;          //ADC read delay between samples

int sensorReading;
int pourcentageValue;

float currentMillivolts = 0;              //Define ADC reading value
const float multiplier = 0.0625F; //ADC value/bit for gain of 2
float calibrationValueOfAir = 0;               //Calibration value (%/mV)
const float ppO2 = 20.9000F;

float percentO2;                  //% O2

float currentMillivoltsInLoop = 0;              //Define ADC reading value
float currentMillivoltsInLoopByMultiplier = 0;

/*taille du tableau de destination définie*/
char destination [ 8 ] ;

float currentMaxiPpO2Allowed = 1.6F;

bool debuggingMode = false;

void setup() {
  Serial.begin(9600);

  Serial.println(F("================================================"));
  Serial.println(F("Trimix Analyzer by Maxime VIALETTE"));
  Serial.println(F("------------------------------------------------"));

  //ADS.setGain(0);		// ± 6.144 volt (par défaut). À noter que le gain réel ici sera de « 2/3 », et non zéro, comme le laisse croire ce paramètre !
  //ADS.setGain(1);	// ± 4.096 volt
  ADS.setGain(2);	// ± 2.048 volt, 	0.0625 mV (multiplier)
  //ADS.setGain(4);	// ± 1.024 volt
  //ADS.setGain(8);	// ± 0.512 volt
  //ADS.setGain(16);	// ± 0.256 volt

  ADS.setMode(1);	// 0 = CONTINUOUS, 1 = SINGLE (default)

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  if(debuggingMode) {
    Serial.print(F("ADS1X15_LIB_VERSION: "));
    Serial.println(ADS1X15_LIB_VERSION);
  }

  display.clearDisplay();       //CLS
  display.display();            //CLS

  display.setTextColor(WHITE);  //WHITE for monochrome
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setCursor(0,0);             // Start at top-left corner

  display.println(F("====================="));
  display.println(F("=  Trimix Analyzer  ="));
  display.println(F("=                   ="));
  display.println(F("=   Enjoy Diving    ="));
  display.println(F("=                   ="));
  display.println(F("=  Maxime VIALETTE  ="));
  display.println(F("=                   ="));
  display.println(F("====================="));
  display.display();                //CLS execute

  display.clearDisplay();       //CLS
  display.display();            //CLS
  delay(2000);  // Pause for 2 seconds

  display.setCursor(0, 0);
  display.println(F("====================="));
  display.println(F("=  Nitrox Analyzer  ="));
  display.println(F("=                   ="));
  display.println(F("=    CALIBRATION    ="));
  display.println(F("=                   ="));
  display.println(F("=     PENDING ...   ="));
  display.println(F("=                   ="));
  display.println(F("====================="));
  display.display();                //CLS execute
  delay(2000);  // Pause for 2 seconds

  if(debuggingMode) {
    // settings up the ADS 1115 module
    Serial.print(F("coeficient = "));
    Serial.print(multiplier);
    Serial.println(" float");
  }

  float millivolts = ADS.readADC_Differential_0_1();  //Read differental voltage between ADC pins 2 & 3

  /*
  Serial.println(F(""));
  Serial.print(F("Calibration with value = "));
  Serial.print(millivolts);
  Serial.println(F(" mv without multiplier"));
  Serial.println(F(""));
   //Compute calibrationValue
    //currentMillivolts = currentMillivolts * multiplier;
  Serial.print(F("millivolts ("));
  Serial.print(millivolts);
  Serial.print(F(") * multiplier ("));
  Serial.print(multiplier);
  Serial.print(F(") = "));
  Serial.print(millivolts * multiplier);
  Serial.println(F(" mv"));
  */

  calibrationValueOfAir = ppO2 / (millivolts * multiplier); // determine witch calValue (coefficient to apply air PpO2 = 20.9% O2)

  Serial.println(F("Calibration Oxy"));
  Serial.print(F("PpO2 ("));
  Serial.print(ppO2);
  Serial.print(F(") / (millivolts without multiplier ("));
  Serial.print(millivolts);
  Serial.print(F(") * multiplier ("));
  Serial.print(multiplier);
  Serial.print(F(")) == "));
  Serial.print(calibrationValueOfAir);
  Serial.println(F(" mv"));  
  Serial.println(F(""));
  Serial.println(F("End of cablibration"));
  
  display.clearDisplay();       //CLS
  display.display();            //CLS
  delay(200);  // Pause

  /*
  display.println(F("O2 calibration end"));
  display.print(F("Nitrox Nx 21 %"));
  display.display();                //CLS execute
  delay(2000);  // Pause for 2 seconds

  */

  display.setCursor(0, 0);
  display.println(F("====================="));
  display.println(F("=  Nitrox Analyzer  ="));
  display.println(F("=                   ="));
  display.println(F("=    CALIBRATION    ="));
  display.println(F("=                   ="));
  display.println(F("=      ENDING !     ="));
  display.println(F("=                   ="));
  display.println(F("====================="));
  display.display();                //CLS execute
  delay(2000);  // Pause for 2 seconds

  display.clearDisplay();       //CLS
  display.display();            //CLS dc       
  delay(200);  // Pause

  display.setCursor(0, 0);
  display.println(F("====================="));
  display.println(F("=  Nitrox Analyzer  ="));
  display.println(F("=                   ="));
  display.println(F("=                   ="));
  display.println(F("=   Nitrox Nx 21 %  ="));
  display.println(F("=                   ="));
  display.println(F("=                   ="));
  display.println(F("====================="));
  display.display();                //CLS execute
  delay(2000);  // Pause for 2 seconds
      
  //RA.addValue(millivolts);
  //delay(readDelay);
  /*

  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(F("---Trimix Analyzer---"));

  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.println(F(""));
  display.println(F("by"));
  display.println(F(""));
  //display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(F("Maxime VIALETTE"));
  */

  //display.clearDisplay();       //CLS
  //display.display();            //CLS
}

void loop() {
}


