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



// Time
unsigned long currentMillis = millis();  // stores the current time since startup in milliseconds

unsigned long DHTInterval = 5000;  // interval for checking the thermometer // 1 minute
unsigned long prevDHTTime;            // time of last temperature reading

unsigned long radioInterval = DHTInterval/2; // interval for transmitting data - set to half of the DHTinterval 
unsigned long prevRadioTime; // time of last transmission

int count = 1;

// Radio
RCSwitch radio = RCSwitch();
String data; // data to send 
int currentVal; // the value of the two numbers to be transmitted - helps with reconstructing on the receiver
int tempPeriod; // store location of decimal in temp data

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
  if (currentMillis - prevRadioTime > 10000 ) {
    prevRadioTime = currentMillis;

    //sends data two digits at a time
    Serial.println(tempC);
    //Integer of temp
    for (int i = 0; i < str_tempC.length(); i ++){ // loop through temp
      if (str_tempC[i] == "."){ // find the period
        tempPeriod = i; // store this value
        for (int j = 0; j < i; j ++){ // loop through everything before the period
          currentVal += str_tempC[j]; // add that to a string to be transmitted
        }
      }
    }
    radio.send(int(currentVal)); // send integer part of temp
    Serial.println(currentVal);
    Serial.println(int(currentVal));
    currentVal = 0;

    //Decimal of temp
    for (int i = tempPeriod; i < str_tempC.length(); i ++){ // loop through temp starting at the period
      currentVal += str_tempC[i]; // add to a string to be transmitted
    }
    radio.send(int(currentVal)); // send the decimal part of temp
    Serial.println(currentVal);
    Serial.println(int(currentVal));
    currentVal = 0;

    //Again but for humidity now - doesnt have decimal
    Serial.println(humidity);

    for (int i = 0; i < str_humidity.length(); i ++){ // loop through humidity
      if (str_humidity[i] == "."){ // find the period
        for (int j = 0; j < i; j ++){ // loop through everything before the period
          currentVal += str_humidity[j]; // add that to a string to be transmitted
        }
      }
    }
    radio.send(int(currentVal)); // send humidity
    Serial.println(currentVal);
    Serial.println(int(currentVal));
    currentVal = 0;
    
    //sends data one digit at a time
    // for (int i = 0; i < data.length(); i ++){
    //   radio.send(int(data[i]), 24);
    //   Serial.println(int(data[i]));
    //   delay(200);
    // }
    Serial.println("\nSent ig");
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

    // Sava data in prep for transmission
    str_tempC = String(tempC);
    str_humidity = String(humidity);

    data = str_tempC + "," + str_humidity;
    // Serial.println(data);
  }
}
