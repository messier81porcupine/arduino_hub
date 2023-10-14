#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <DHT_U.h>
#include <RCSwitch.h>


// LCD
const int rs = 3, e = 4, db4 = 5, db5 = 6, db6 = 7, db7 = 8;  // set pins used by the LCD
LiquidCrystal lcd(rs, e, db4, db5, db6, db7);                 // create LCD object
int minBrightness = 3;                                        // limits the mapping of ambientBrightness when setting the LCD backlight
int maxBrightness = 100;                                      // limits the mapping of ambientBrightness when setting the LCD backlight

// DHT11 Temp and Humidity Sensor
const int DHTPin = 11;
DHT_Unified dht(DHTPin, DHT11);

// Radio
RCSwitch radio = RCSwitch();
int receivedVal;         // used to store the most recently transmitted value
int receivingMode = -1;  // used to know which data is being recieved 0 = Temp; 1 = Humidity
int loops = 10;          // times to check for new data per call - separated by delayMS
unsigned long delayMS = 500;
// RTC
RTC_DS3231 rtc;  // create real time clock object
char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// Pins
const int LCDBacklightPin = 9;  // setting the LCD backlight/brightness

const int SDArtc = A4;
const int SCLrtc = A5;

const int photoTransPin = A2;

const int moistureSensorPin = A0;
const int moisturePowerPin = A1;

const int redLED = 13;

const int buttonPin = 12;

// Sensor Values - Thresholds
int ambientBrightness;  // reading from phototransistor - used to set LCD backlight

int moistureLevel;  // reading from moisture sensor
int wet = 400;      // if moistureLevel is above this value the red LED wont flash
int dry = 300;      // if moistureLevel is below this value the red LED will flash

float humidity;     // humidity reading from DHT sensor
float tempC;        // temp reading from DHT sensor
float RTCTemp;      // temp reading from RTC
float leveledTemp;  // avg of tempC + rtc temp - balances out the readings ¿use this? idk

int outHumidity = -1000;  // humidity reading recieved by the radio - outdoor from Nano Every -
int outTemp = -1000;      // temp reading recieved by the radio - outdoor from Nano Every - -1000 is the preset so that the loading animation will play

int buttonState;  // state of button on pin 12

// Time
DateTime now;                            // stores rtc current time
unsigned long currentMillis = millis();  // stores the current time since startup in milliseconds

unsigned long transInterval = 10000;  // interval for checking the phototransistor // 10 seconds
unsigned long prevTransTime;          // time of last phototrans reading

unsigned long DHTInterval = 1200000;  // interval for checking the thermometer/humidity // 20 minutes
unsigned long prevDHTTime;            // time of last temperature reading

unsigned long moistureInterval = 10800000;  // 6 hours in ms = 21600000; 3 hours in ms = 10800000
unsigned long prevTime;                     // time of last moisture sensor reading

unsigned long LEDInterval = 2000;  // time to wait before flashing the red LED when Dry // 2 seconds
unsigned long prevLEDTime;         // time of last red LED flash

unsigned long RTCoutputInterval = 20000;  // time to wait between outputing the RTC data to the serial port // 20 seconds
unsigned long prevRTCtime;                // time of last rtc output

unsigned long LCDOutputInterval = 5000;  // time to wait between updating the LCD // 5 seconds
unsigned long prevLCDoutputTime;         // time of last LCD output

unsigned long radioInterval = 30000;  // interval for receiving data // 30 seconds
unsigned long prevRadioTime;          // time of last transmission

int count = 1;  // counts how many times the sensor has been run


void setup() {
  Serial.begin(9600);
  dht.begin();

  delay(100);
  radio.enableReceive(digitalPinToInterrupt(2));
  delay(100);

  // Set Pin Modes
  pinMode(buttonPin, INPUT);
  pinMode(redLED, OUTPUT);
  pinMode(photoTransPin, INPUT);
  pinMode(moistureSensorPin, INPUT);
  pinMode(moisturePowerPin, OUTPUT);
  pinMode(LCDBacklightPin, OUTPUT);
  pinMode(DHTPin, INPUT);

  // Configure RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // Configure LCD backlight
  ambientBrightness = analogRead(photoTransPin);
  analogWrite(LCDBacklightPin, map(ambientBrightness, 0, 1023, minBrightness, 255));


  // Display Welcome Message
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Welcome");
  delay(250);

  lcd.setCursor(0, 1);
  lcd.print("Hub Prototype");
  delay(500);

  // Flash the red LED
  digitalWrite(redLED, 1);
  delay(15);
  digitalWrite(redLED, 0);
}

void loop() {
  currentMillis = millis();

  // Prevent count from overflowing
  if (count < 0) {
    count = 2;
  }

  // Force moisture sensor reading
  buttonState = digitalRead(buttonPin);
  if (buttonState == 1) {
    delay(250);
  }

  // Method Call
  setBrightness();
  readMoistureSensor();
  setMoistureLED();
  updateDHT();
  now = updateRTC();
  receiveData();

  outputLCD(now);
  count++;
}


// Methods
void receiveData() {
  if (currentMillis - prevRadioTime > radioInterval || count == 1 || buttonState == 1) {
    prevRadioTime = currentMillis;

    if (count == 1) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Searching");
    }

    for (int i = 0; i < loops - 1; i++) {
      if (radio.available()) {
        receivedVal = radio.getReceivedValue();
        // Serial.print("\nReceived: ");
        // Serial.println(receivedVal);
      }
      if (count == 1) {
        lcd.setCursor(i + 9, 0);
        lcd.print(".");
      }
      // set receiving mode based on what data is received
      if (receivedVal == 111) {
        receivingMode = 0;  // temp
      } else if (receivedVal == 333) {
        receivingMode = 1;         // humidity
      } else {                     // if not 111(tmp) or 333(humid) then it must be normal data
        if (receivingMode == 0) {  // save that data into the appropriate variable based on the most recent recieved code
          outTemp = receivedVal;
        } else if (receivingMode == 1) {
          outHumidity = receivedVal;
        }
      }
      Serial.println("\nRecieved\nDHT - Outdoor");
      Serial.print(outTemp);
      Serial.println("C");
      Serial.print(outHumidity);
      Serial.println("%");

      radio.resetAvailable();
      delay(delayMS);
    }
  }
}
void setBrightness() {  // read phototransistor and set brightness level accordingly
  if (currentMillis - prevTransTime > transInterval || count == 1 || buttonState == 1) {
    if (buttonState != 1) {
      prevTransTime = currentMillis;
    }

    ambientBrightness = analogRead(photoTransPin);
    Serial.print("\nAmbient Brightness: ");
    Serial.println(ambientBrightness);
    analogWrite(LCDBacklightPin, map(ambientBrightness, 0, 90, minBrightness, maxBrightness));  // set the LCD backlight to a mapped value from ambientBrightness - limited by minBrightness
  }
}
void readMoistureSensor() {  // read moisture sensor and display it on lcd
  if (currentMillis - prevTime > moistureInterval || count == 1 || buttonState == 1) {
    if (buttonState != 1) {
      prevTime = currentMillis;
    }

    digitalWrite(moisturePowerPin, 1);  // power on the moisture sensor
    delay(100);

    moistureLevel = analogRead(moistureSensorPin);  // read moisture sensor
    Serial.print("\nMoisture: ");
    Serial.println(moistureLevel);

  } else {
    digitalWrite(moisturePowerPin, 0);  // if it isnt time to read the sensor make sure it is off
  }
}
void setMoistureLED() {  // set red LED according to moisture sensor value
  if (moistureLevel > wet) {
    digitalWrite(redLED, 0);
  } else if (moistureLevel < dry) {
    digitalWrite(redLED, 0);
    if (currentMillis - prevLEDTime > LEDInterval) {
      prevLEDTime = currentMillis;
      digitalWrite(redLED, 1);
      delay(50);
    }

  } else {
    digitalWrite(redLED, 0);
  }
}
void updateDHT() {  // read humidity and temperature from DHT sensor
  if (currentMillis - prevDHTTime > DHTInterval || count == 1 || buttonState == 1) {
    if (buttonState != 1) {
      prevDHTTime = currentMillis;
    }
    sensors_event_t event;
    Serial.println("\nDHT - Indoor");
    // Temperature C
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println("Error Reading DHT Temp");
    } else {
      tempC = event.temperature;
      Serial.print(tempC);
      Serial.println("C");
    }
    // Humidity
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println("Error Reading DHT Humidity");
    } else {
      humidity = event.relative_humidity;
      Serial.print(humidity);
      Serial.println("%");
    }
    Serial.print("\nRTC Temp: ");
    RTCTemp = rtc.getTemperature();
    Serial.print(RTCTemp);

    leveledTemp = (tempC + RTCTemp) / 2;
    Serial.print("\nLeveled Temp: ");
    Serial.println(leveledTemp);
  }
}
DateTime updateRTC() {  // update now variable; also rtc temp - print rtc data to serial
  DateTime now = rtc.now();

  if (currentMillis - prevRTCtime > RTCoutputInterval || count == 1 || buttonState == 1) {
    prevRTCtime = currentMillis;
    Serial.print("\n");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
  }
  return now;
}
void outputLCD(DateTime now) {  // output all data onto LCD

  if (currentMillis - prevLCDoutputTime > LCDOutputInterval || count == 1 || buttonState == 1) {
    prevLCDoutputTime = currentMillis;
    lcd.clear();
    lcd.setCursor(0, 0);

    // Time
    if (now.hour() < 10) {
      lcd.print(0);
    }
    lcd.print(now.hour());
    lcd.print(":");
    if (now.minute() < 10) {
      lcd.print(0);
    }
    lcd.print(now.minute());

    // Temp IN
    lcd.setCursor(12, 0);
    lcd.print(round(leveledTemp));
    lcd.print("C");

    // Temp OUT
    lcd.setCursor(7, 0);
    if (outTemp == -1000) {
      lcd.print("-");
    } else {
      lcd.print(round(outTemp));
    }

    lcd.print("C");

    // Moisture
    lcd.setCursor(0, 1);
    lcd.print(moistureLevel);
    if (moistureLevel < dry) {
      lcd.print(" D");
    }

    // Ambient Brightness
    // lcd.setCursor(7, 1);
    // lcd.print(ambientBrightness);

    // Humidity IN
    lcd.setCursor(12, 1);
    lcd.print(round(humidity));
    lcd.print("%");

    // Humidity OUT
    lcd.setCursor(7, 1);
    if (outHumidity == -1000) {
      lcd.print("-");
    } else {
      lcd.print(round(outHumidity));
    }

    lcd.print("%");
  }
}