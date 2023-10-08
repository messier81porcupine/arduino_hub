#include <DHT.h>
#include <DHT_U.h>
#include <RCSwitch.h>


// DHT11 Temp and Humidity Sensor
const int DHTPin = 2;
const int DHTPowerPin = 3;
DHT_Unified dht(DHTPin, DHT11);
float tempC;
float humidity;


// Time
unsigned long currentMillis = millis();  // stores the current time since startup in milliseconds

unsigned long DHTInterval = 5000;  // interval for checking the thermometer // 1 minute
unsigned long prevDHTTime;            // time of last temperature reading

int count = 1;

// Radio
RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(9600);
  dht.begin();

  mySwitch.enableTransmit(10);

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
  mySwitch.send(8, 24);
  Serial.println("Sent ig");
  delay(1000);
}
void updateDHT() {  // read humidity and temperature from DHT sensor
  if (currentMillis - prevDHTTime > DHTInterval || count == 1) {
    prevDHTTime = currentMillis;
    
    digitalWrite(DHTPowerPin, 1);
    delay(50);

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
