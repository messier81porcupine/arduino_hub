#include <DHT.h>
#include <DHT_U.h>
#include <RH_ASK.h>
#include <SPI.h>

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
RH_ASK radio(2000, 11, 12);

void setup() {
  Serial.begin(9600);
  dht.begin();

    // Speed of 2000 bits per second
    // Use pin 11 for reception
    // Use pin 12 for transmission
    
    if (!radio.init()){
         Serial.println("Radio module failed to initialize");
    }
}

void loop() {
  currentMillis = millis();
  // Prevent count from overflowing
  if (count < 0) {
    count = 2;
  }
  updateDHT();
  count ++;
}
void transmitData(){
    // Create
    const char *msg = "Hello World";

    radio.send((uint8_t*)msg, strlen(msg));
    radio.waitPacketSent();
 
    delay(1000);
    Serial.println("Data Sent");
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
