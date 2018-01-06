#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <PID_v1.h>

/* The LCD module definitions */
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
// LCD relay is plugged into port 2 on the Arduino
#define RELEY_DIGITAL 2
// LCD backlight is plugged into port 10 on the Arduino
#define LCD_BACKLIGHT_PIN 10

/* The Temperature Sensor definitions */
// Data wire is plugged into port 3 on the Arduino
#define ONE_WIRE_BUS 3
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature
DallasTemperature sensors(&oneWire);

// global variables
float shouldTemp = 18.0;
float currentTemp = 0.0;
float maxDiff = 0.3;
float lastUpdate = 0.0;
float minMilisecBeforeUpdate = 1000.0*10; //turn of / on max every 10 sec.
float earlieastTempreadTime = 0.0;
float defaultDisplayOffTime = 30000;
float minTimeTurnDisplayOff = defaultDisplayOffTime;
bool chill = true;

// Temperature defines to hold states
#define STATE_SHOW_TEMP 0
#define STATE_LAST 1
int displayState = 0;

void setup() {
  //LCD columns,rows:
  lcd.begin(16, 2);
  lcd.print("Init");
  //Begin temperature sensor
  sensors.begin();
  //set reley pin as output
  pinMode(RELEY_DIGITAL, OUTPUT);
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT );

  //Turn display on, and put a time
  //when it should be turned off
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH );
  minTimeTurnDisplayOff = millis() + defaultDisplayOffTime; 
}



bool isOn = false;
void printStatus() {
  lcd.setCursor(13, 0);
  if(isOn) {
    lcd.print("ON ");
  } else {
    lcd.print("OFF");
  }
}

void turnSwitchOn() {
  if(isOn == false) {
    if(lastUpdate == 0  || (millis() - lastUpdate > minMilisecBeforeUpdate)) {
      //OK atleast minMilisecBeforeUpdate milisec has past since last change.. 
      digitalWrite(RELEY_DIGITAL, HIGH);
      lastUpdate = millis();
      isOn = true;
    }
  }
}

void turnSwitchOff() {
  if(isOn == true) {
    if(lastUpdate == 0 || (millis() - lastUpdate > minMilisecBeforeUpdate)) {
      //OK atleast minMilisecBeforeUpdate milisec has past since last change.. 
      digitalWrite(RELEY_DIGITAL, LOW);
      lastUpdate = millis();
      isOn = false;
    }
  }
}

bool toHot() {
  if(currentTemp > shouldTemp) {
    float diff = currentTemp - shouldTemp;
    if(diff > maxDiff) {
      return true;
    }
  }
  return false;
}

bool toCold() {
  if(currentTemp < shouldTemp) {
    float diff = shouldTemp - currentTemp;
    if( diff > maxDiff) {
      return true;
    }
  }
  return false;
}

float lastRead = 0.0;
float phyRead() {
  if (earlieastTempreadTime == 0 || millis() > earlieastTempreadTime  ) {
    if(lastRead == 0 || millis() - lastRead > 1000) {
      lastRead =  millis();
      sensors.requestTemperatures();
      return sensors.getTempCByIndex(0);
    }
  }
  return currentTemp;
}
int read = 0;
float readTemp() {
  return (phyRead() + phyRead()) / 2;
}


void handleUpKey() {
  shouldTemp += 1.0;
  lastUpdate = 0;
}

void handleDownKey() {
  shouldTemp -= 1.0;
  lastUpdate = 0;
}

void handleChillSwitch() {
  chill = !chill;
  if(chill){
     shouldTemp = 15;
  }else{
     shouldTemp = 65;
  }
}

void handleLeftKey() {
  displayState--;
  if(displayState < 0) {
    displayState = STATE_LAST -1;
  }
}

void handleRightKey() {
  displayState++;
  if (displayState >= STATE_LAST) {
     displayState = 0;
  }
}

/*
 * Functions for determine keypresses on the LCD display module
 */
bool isRightKey(int lcdKeyValue) {
  return lcdKeyValue == 0;
}
bool isUpKey(int lcdKeyValue) {
  return lcdKeyValue >= 50 && lcdKeyValue < 200;
}
bool isDownKey(int lcdKeyValue) {
  return lcdKeyValue >= 200 && lcdKeyValue < 400;
}
bool isLeftKey(int lcdKeyValue) {
  return lcdKeyValue >= 400 && lcdKeyValue < 600;
}
bool isSelectKey(int lcdKeyValue) {
  return lcdKeyValue >= 600 && lcdKeyValue < 800;
}


/*
 * Function for handling the keypress on the LCD display module
 */
int pendingKey = 0;
void readLcdKeys() {
  int lcdKeyValue = analogRead (0);
  if(lcdKeyValue < 1000) {
    //Key pushed.. turn lcd on
    digitalWrite(LCD_BACKLIGHT_PIN, HIGH );
    pendingKey = lcdKeyValue;
    earlieastTempreadTime = millis() + 10000; 
  } else if(pendingKey != 0) {
    if(isUpKey(pendingKey)) {
      handleUpKey();
    } else if (isDownKey(pendingKey)) {
      handleDownKey();
    } else if (isSelectKey(pendingKey)) {
      handleChillSwitch();
    }
    if(chill) {
      //turn diplay off in 30 sec, if we are fermenting
      minTimeTurnDisplayOff = millis() + defaultDisplayOffTime;
    } else {
      //Keep the LCD on
      minTimeTurnDisplayOff = 0;
    }
    pendingKey = 0;
  }
}

float printedShouldTemp = 0.0;
float printedCurrentTemp = 0.0;
void printShowTempDisplay() {
  if(millis() > minTimeTurnDisplayOff && minTimeTurnDisplayOff > 0) {
     //turn display off
     digitalWrite(LCD_BACKLIGHT_PIN, LOW );
     return;
  }
  if(shouldTemp != printedShouldTemp || printedCurrentTemp != currentTemp) {
    printedShouldTemp = shouldTemp;
    printedCurrentTemp = currentTemp;
    // set the cursor to column 0, line 0
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Should:");
    lcd.print(shouldTemp);
    // set the cursor to column 0, line 1
    lcd.setCursor(0, 1);
    lcd.print("Is:");
    lcd.print(currentTemp);
    lcd.print(" ");
    if(chill){
      lcd.print("Ferm");
    } else {
      lcd.print("Mash");
    }
  }
}
void printDisplay() {
  switch(displayState) {
    case STATE_SHOW_TEMP: {
      printShowTempDisplay();
      break;
    }
    //TODO: Add more displays to show here, 
  }
}

void loop() {
  //Read temp from sensor
  currentTemp = readTemp();
  printDisplay();
  if(toHot()) {
    if(chill) {
      turnSwitchOn();
    } else {
      turnSwitchOff();
    }
  } else if(toCold()) {
    if(chill) {
      turnSwitchOff();
    } else {
      turnSwitchOn();
    }
  } else {
    turnSwitchOff();
  }
  printStatus();
  readLcdKeys();
}
