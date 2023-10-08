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
int currentVal; // the value of the current index to be transmitted - needed to transcode commas and periods

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

    Serial.println(data);
    
    for (int i = 0; i < data.length(); i ++){
      radio.send(int(data[i]), 24);
      Serial.println(int(data[i]));
      delay(200);
    }
    Serial.println("Sent ig");
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
