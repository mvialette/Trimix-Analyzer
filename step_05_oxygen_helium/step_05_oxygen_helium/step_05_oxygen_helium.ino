/**
* Arduino projet by Maxime VIALETTE
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <RunningAverage.h>

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

RunningAverage oxygenRA(10);       // moyennage O2 sur 10 valeurs
RunningAverage heliumRA(10);       // moyennage He sur 10 valeurs

const int readDelay = 3;          //ADC read delay between samples

int sensorReading;
int pourcentageValue;

float currentMillivolts = 0;              //Define ADC reading value
//const float multiplier = 0.0625F; //ADC value/bit for gain of 2

 //ADC value/bit for gain of 4
const float multiplier = 0.03125F;

float calibrationValueOfAir = 0;               //Calibration value (%/mV)
const float ppO2 = 20.9000F;
const float ppHe = 1.0F;

float percentO2;                  //% O2

float currentMillivoltsInLoop = 0;              //Define ADC reading value
float currentMillivoltsInLoopByMultiplier = 0;

/*taille du tableau de destination définie*/
char destination [ 8 ] ;

float currentMaxiPpO2Allowed = 1.6F;

bool debuggingMode = true;

float voltageOxygenCalibrationAir = 0;
float voltageOxygenCurrent = 0;
float ppO2Current = 0;

//float voltageHelium = 0;
float voltageHeliumCalibrationAir = 10;
float voltageHeliumCalibrationMaxi = 661.26;      // valeur de la tension du pont avec 100% helium
float voltageHeliumCurrent = 0;
float ppHeCurrent = 0;

void setup() {
  Serial.begin(9600);

  Serial.println(F("================================================"));
  Serial.println(F("Trimix Analyzer by Maxime VIALETTE"));
  Serial.println(F("------------------------------------------------"));

  //ADS.setGain(0);		// ± 6.144 volt (par défaut). À noter que le gain réel ici sera de « 2/3 », et non zéro, comme le laisse croire ce paramètre !
  //ADS.setGain(1);	// ± 4.096 volt
  //ADS.setGain(2);	// ± 2.048 volt, 	0.0625 mV (multiplier)
  ADS.setGain(4);	// ± 1.024 volt
  //ADS.setGain(8);	// ± 0.512 volt
  //ADS.setGain(16);	// ± 0.256 volt

  //ADS.setMode(0);	// 0 = CONTINUOUS, 1 = SINGLE (default)

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

  //display.clearDisplay();       //CLS
  //display.display();            //CLS
  delay(2000);  // Pause for 2 seconds  

  display.clearDisplay();       //CLS
  display.display();            //CLS

  oxygenCalibration();
  heliumCalibration();

  delay(2000);  // Pause for 2 seconds  

  display.clearDisplay();       //CLS
  display.display();            //CLS

  displayStaticMeasureLabels();
}

void displayStaticMeasureLabels() {
  display.setCursor(0, 0);
  display.println(F("====================="));

  display.println(F("= Oxygen = "));

  display.println(F("= Helium = "));
  
  display.println(F("=                   ="));
  display.println(F("====================="));
  
  display.display();            //CLS
  delay(2000);  // Pause for 2 seconds  
}

void oxygenCalibration() {
  
  display.clearDisplay();       //CLS
  display.display();            //CLS

  display.setTextColor(WHITE);  //WHITE for monochrome
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setCursor(0,0);             // Start at top-left corner

  display.println(F("Oxygen calibration"));
  display.display();                //CLS execute

  Serial.println(F("O2 Calibration :"));
  
  int i = 0;
  float tensionMoyenne = 0;

  //for(i = 1; i <10 or (abs (voltage - (tensionMoyenne / (i-1)))) > 0.001; i++) {
  for(i = 1; i <10; i++) {
      float measureOxygen = ADS.readADC_Differential_0_1();  //Read differental voltage between ADC pins 2 & 3
      if(measureOxygen > 0){
        oxygenRA.addValue(measureOxygen);
      }

      Serial.print(F("i =="));
      Serial.print(i);
      Serial.print(F(", measureOxygen ("));
      Serial.print(measureOxygen);
      Serial.print(F(") * multiplier ("));
      Serial.print(multiplier);
      Serial.print(F(") = "));
      Serial.print(measureOxygen * multiplier);
      Serial.println(F(" mv"));

      voltageOxygenCalibrationAir = abs(oxygenRA.getAverage()*multiplier); 
      tensionMoyenne = tensionMoyenne + voltageOxygenCalibrationAir;

      display.print(F("."));
      display.display();    

      delay(300);
  }
  
  Serial.print(F("Average =="));
  Serial.print(oxygenRA.getAverage());
  Serial.print(F(") * multiplier ("));
  Serial.print(multiplier);
  Serial.print(F(") = "));
  Serial.print(voltageOxygenCalibrationAir);
  Serial.println(F(" mv"));
}

void heliumCalibration() {

  display.clearDisplay();       //CLS
  display.display();            //CLS

  display.setTextColor(WHITE);  //WHITE for monochrome
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setCursor(0,0);             // Start at top-left corner

  display.println(F("Helium calibration"));
  display.display();                //CLS execute

  Serial.println(F("He Calibration :"));

  int i = 0;
  float tensionMoyenne = 0;

  //for(i = 1; i <10 or (abs (voltage - (tensionMoyenne / (i-1)))) > 0.001; i++) {
  for(i = 1; i <100; i++) {
      float measureHelium = ADS.readADC_Differential_2_3();  //Read differental voltage between ADC pins 2 & 3
      heliumRA.addValue(measureHelium);

      Serial.print(F("i =="));
      Serial.print(i);
      Serial.print(F(", measureHelium ("));
      Serial.print(measureHelium);
      Serial.print(F(") * multiplier ("));
      Serial.print(multiplier);
      Serial.print(F(") = "));
      Serial.print(measureHelium * multiplier);
      Serial.println(F(" mv"));

      voltageHeliumCalibrationAir = abs(heliumRA.getAverage()*multiplier); 
      tensionMoyenne = tensionMoyenne + voltageHeliumCalibrationAir;
      delay(300);
  }
  
  Serial.print(F("Average =="));
  Serial.print(heliumRA.getAverage());
  Serial.print(F(") * multiplier ("));
  Serial.print(multiplier);
  Serial.print(F(") = "));
  Serial.print(voltageHeliumCalibrationAir);
  Serial.println(F(" mv"));
}

void oxygenMesure() {
  float measureOxygen = ADS.readADC_Differential_0_1();  //Read differental voltage between ADC pins 2 & 3

  //oxygenRA.addValue(measureOxygen);
  if(measureOxygen > 0){
    Serial.print(F("O2 measure with value = "));
    
    Serial.print(F("measureOxygen ("));
    Serial.print(measureOxygen);
    Serial.print(F(") * multiplier ("));
    Serial.print(multiplier);
    Serial.print(F(") = "));

    voltageOxygenCurrent = abs(measureOxygen * multiplier); 

    ppO2Current = voltageOxygenCurrent * ppO2 / voltageOxygenCalibrationAir;

    Serial.print(voltageOxygenCurrent);
    Serial.print(F(" mv"));

    Serial.print(F(", versus voltageOxygenCalibrationAir ("));
    Serial.print(voltageOxygenCalibrationAir);
    Serial.print(F(" ===> "));
    Serial.print(ppO2Current);
    Serial.println(F(" %"));
  }
  
}

void heliumMesure() {
    
  float measureHelium = ADS.readADC_Differential_2_3();  //Read differental voltage between ADC pins 2 & 3
  
  Serial.print(F("He measure with value = "));
  
  Serial.print(F("measureHelium ("));
  Serial.print(measureHelium);
  Serial.print(F(") * multiplier ("));
  Serial.print(multiplier);
  Serial.print(F(") = "));

  voltageHeliumCurrent = abs(measureHelium * multiplier); 

  ppHeCurrent = voltageHeliumCurrent * ppHe / voltageHeliumCalibrationAir;

  Serial.print(voltageHeliumCurrent);
  Serial.print(F(" mv"));

  Serial.print(F(", versus voltageHeliumCalibrationAir ("));
  Serial.print(voltageHeliumCalibrationAir);
  Serial.print(F(" ===> "));
  Serial.print(ppHeCurrent);
  Serial.println(F(" %"));

  Serial.print(F(">>>>>>>>>> 100 * voltageHeliumCurrent / voltageHeliumCalibrationMaxi == "));
  Serial.print(100 * voltageHeliumCurrent / voltageHeliumCalibrationMaxi);
}

void refreshDisplay() {

  // oxygen current value
  display.setCursor(70, 8);
  display.fillRect(70, 8, 40, 8, BLACK);
  display.display();            //CLS

  display.setCursor(70, 8);
  display.print(ppO2Current);
  display.println(F(" %"));

  // helium current value

  display.setCursor(70, 20);
  display.fillRect(70, 20, 50, 8, BLACK);
  display.display();            //CLS

  display.setCursor(70, 20);
  display.print(ppHeCurrent);
  display.println(F(" %"));
  
  display.display();            //CLS
  delay(2000);  // Pause for 2 seconds
}

void loop() {
  oxygenMesure();
  heliumMesure();

  refreshDisplay();
}


