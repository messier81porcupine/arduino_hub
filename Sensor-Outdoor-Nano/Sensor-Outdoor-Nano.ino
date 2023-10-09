#include <DHT.h>
#include <DHT_U.h>
#include <RCSwitch.h>


// DHT11 Temp and Humidity Sensor
const int DHTPin = 2;
const int DHTPowerPin = 3;
DHT_Unified dht(DHTPin, DHT11);
float tempC;
float humidity;
String str_humidity; // used for data transmission
String str_tempC; // used for data transmission

// Radio
RCSwitch radio = RCSwitch();
String data; // data to send 
int currentVal; // the value of the two numbers to be transmitted - helps with reconstructing on the receiver
int tempPeriod; // store location of decimal in temp data
int loops = 40; // number of times to send the data - separated by delayMS
unsigned long delayMS = 250;

// Time
unsigned long currentMillis = millis();  // stores the current time since startup in milliseconds

unsigned long DHTInterval = 1200000;  // interval for checking the thermometer // 1 minute
unsigned long prevDHTTime;            // time of last temperature reading

unsigned long radioInterval = DHTInterval/4; // interval for transmitting data - set to a fourth of the DHTinterval 
unsigned long prevRadioTime; // time of last transmission

int count = 1;

void setup() {
  Serial.begin(9600);
  dht.begin();

  radio.enableTransmit(10);

}

void loop() {
  currentMillis = millis();
  // Prevent count from overflowing
  if (count < 0) {
    count = 2;
    
  }
  updateDHT();
  transmitData();

  count ++;
}
void transmitData(){
  if (currentMillis - prevRadioTime > radioInterval || count == 1 ) {
    prevRadioTime = currentMillis;

    for (int i = 1; i <= loops; i ++){
      
      Serial.print("\nSending... ");
      Serial.println(i);

      radio.send(111, 24); // signals the begining of temp data and end of humidity data
      radio.send(int(round(tempC)), 24);
      Serial.println(int(round(tempC)));
      radio.send(333, 24); // signals the end of temp data and beginning of humidity data
      
      delay(100);

      radio.send(333, 24); // signals the beginning of humidity data and end of temp data
      radio.send(int(round(humidity)), 24); 
      Serial.println(int(round(humidity)));
      radio.send(111, 24); // signals the end of humidity data and beginning of temp data
      
      Serial.println("\nSent");

      
    }
    
  }
}
void updateDHT() {  // read humidity and temperature from DHT sensor
  if (currentMillis - prevDHTTime > DHTInterval || count == 1) {
    prevDHTTime = currentMillis;
    
    digitalWrite(DHTPowerPin, 1);
    delay(100);

    sensors_event_t event;
    Serial.println("\nDHT");
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
    digitalWrite(DHTPowerPin, 0);


  }
}
