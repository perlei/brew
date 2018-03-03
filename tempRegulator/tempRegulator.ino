#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PID_v1.h>

#define PWM_PIN 3

/* The LCD module definitions */
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
// LCD backlight is plugged into port 10 on the Arduino
#define LCD_BACKLIGHT_PIN 10

/* The Temperature Sensor definitions */
// Data wire is plugged into port 12 on the Arduino
#define ONE_WIRE_BUS 12
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature
DallasTemperature sensors(&oneWire);

// global variables
double shouldTemp = 18.0;
double currentTemp = 0.0;
float minMilisecBeforeUpdate = 1000.0*10; //turn of / on max every 10 sec.
float earlieastTempreadTime = 0.0;
float defaultDisplayOffTime = 30000;
float minTimeTurnDisplayOff = defaultDisplayOffTime;
bool ferm = true;

// pid variables

double kP = 3.0, kI = 1.0, kD = 3.0;
double Output;
PID myPID(&currentTemp, &Output, &shouldTemp, kP, kI, kD, DIRECT);

// Temperature defines to hold states
#define STATE_SHOW_TEMP 0
#define STATE_SWITCH_MODE 1
#define STATE_SHOW_KP 2
#define STATE_SHOW_KI 3
#define STATE_SHOW_KD 4
#define STATE_LAST 5

int displayState = 0;

void setup() {
  Serial.begin(9600);
  //LCD columns,rows:
  lcd.begin(16, 2);
  lcd.print("Init");
  //Begin temperature sensor
  sensors.begin();
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT );
  pinMode(PWM_PIN, OUTPUT );

  //Turn display on, and put a time
  //when it should be turned off
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH );
  minTimeTurnDisplayOff = millis() + defaultDisplayOffTime; 
  //Turn pid on
  myPID.SetMode(AUTOMATIC);
}

float lastRead = 0.0;
float phyRead() {
  if (earlieastTempreadTime == 0 || millis() > earlieastTempreadTime  ) {
    if(lastRead == 0 || millis() - lastRead > 1000) {
      lastRead =  millis();
      sensors.requestTemperatures();
      float temp = sensors.getTempCByIndex(0);
      Serial.println(temp);
      return temp;
    }
  }
  return currentTemp;
}
int read = 0;
float readTemp() {
  return (phyRead() + phyRead()) / 2;
}

void handleChillSwitch() {
  ferm = !ferm;
  if(ferm){
     shouldTemp = 15;
  }else{
     shouldTemp = 65;
  }
}


void handleUpKey() {
  switch(displayState)
  {
    case STATE_SHOW_TEMP:
    {
        shouldTemp += 1.0;
        break;
    }
    case STATE_SWITCH_MODE:
    {
        handleChillSwitch();
        break;
    }
    case STATE_SHOW_KP:
    {
        kP += 0.1;
        myPID.SetTunings(kP, kI, kD);
        break;
    }
    case STATE_SHOW_KI:
    {
        kI += 0.05;
        myPID.SetTunings(kP, kI, kD);
        break;
    }
    case STATE_SHOW_KD:
    {
        kD += 0.1;
        myPID.SetTunings(kP, kI, kD);
        break;
    }
  }
}

void handleDownKey() {
  switch(displayState)
  {
    case STATE_SHOW_TEMP:
    {
        shouldTemp -= 1.0;
        break;
    }
    case STATE_SWITCH_MODE:
    {
        handleChillSwitch();
        break;
    }
    case STATE_SHOW_KP:
    {
        kP -= 0.1;
        if(kP < 0)
        {
            kP = 0;
        }
        myPID.SetTunings(kP, kI, kD);
        break;
    }
    case STATE_SHOW_KI:
    {
        kI -= 0.05;
        if(kI < 0)
        {
            kI = 0;
        }
        myPID.SetTunings(kP, kI, kD);
        break;
    }
    case STATE_SHOW_KD:
    {
        kD -= 0.1;
        if(kD < 0)
        {
            kD = 0;
        }
        myPID.SetTunings(kP, kI, kD);
        break;
    }
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
    } else if (isLeftKey(pendingKey)) {
      handleLeftKey();   
    } else if(isRightKey(pendingKey)) {
      handleRightKey();
    }
    if(ferm) {
      //turn diplay off in 30 sec, if we are fermenting
      minTimeTurnDisplayOff = millis() + defaultDisplayOffTime;
    } else {
      //Keep the LCD on
      minTimeTurnDisplayOff = 0;
    }
    pendingKey = 0;
  }
}

void printShowTempDisplay() {
  // set the cursor to column 0, line 0
  lcd.setCursor(0, 0);
  lcd.print("Should:");
  lcd.print(shouldTemp);
  lcd.print("   ");
  // set the cursor to column 0, line 1
  lcd.setCursor(0, 1);
  lcd.print("Is:");
  lcd.print(currentTemp);
  lcd.print(" ");
  if(ferm){
    lcd.print("Ferm");
  } else {
    lcd.print("Mash");
  }
}

void printShowSwitchModeDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("Switch mode:");
  lcd.setCursor(0, 1);
  if(ferm) {
    lcd.print("Ferm");   
  } else {
    lcd.print("Mash");
  }
}

void printBlankLine() {
    lcd.print("                ");
}
void printShowChangeKpDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("Switch PID(kP):");
  lcd.setCursor(0, 1);
  lcd.print(kP);
}

void printShowChangeKiDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("Switch PID(kI):");
  lcd.setCursor(0, 1);
  lcd.print(kI);
}

void printShowChangeKdDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("Switch PID(kD):");
  lcd.setCursor(0, 1);
  lcd.print(kD);
}

bool checkTimeToSetDisplayOff() {
  if(millis() > minTimeTurnDisplayOff && minTimeTurnDisplayOff > 0) {
     //turn display off
     digitalWrite(LCD_BACKLIGHT_PIN, LOW );
     return true;
  }
  return false;
}

int lastPrintDisplay = -1;
void printDisplay() {
  if(checkTimeToSetDisplayOff()) {
    return;
  }
  if(lastPrintDisplay != displayState)
  {
    lcd.clear();
  }
  switch(displayState) {
    case STATE_SHOW_TEMP: {
      printShowTempDisplay();
      break;
    }
    case STATE_SWITCH_MODE: {
      printShowSwitchModeDisplay();
      break;
    }
    case STATE_SHOW_KP: {
      printShowChangeKpDisplay();
      break;
    }
    case STATE_SHOW_KI: {
      printShowChangeKiDisplay();
      break;
    }
    case STATE_SHOW_KD: {
      printShowChangeKdDisplay();
      break;
    }
  }
  lastPrintDisplay = displayState;
}

void handleOutputFermation() {
  if(currentTemp > shouldTemp) {
    analogWrite(PWM_PIN, 255);
  } else {
    analogWrite(PWM_PIN, 0);
  }
}

void handleOutputMash() {
  analogWrite(PWM_PIN, Output);
  Serial.println(Output);
}

void handleOuput() {
  if(ferm) {
    handleOutputFermation();
  } else {
    handleOutputMash();
  }
}

void loop() {
  //Read temp from sensor
  currentTemp = readTemp();
  //Make the PID compute
  myPID.Compute();  
  handleOuput();
  printDisplay();
  readLcdKeys();
}
