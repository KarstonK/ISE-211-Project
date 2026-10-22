#include <DHT.h>

// DHT11 DATA pin (D12)
#define DHTPIN 15 
// Sensor type: DHT11
#define DHTTYPE DHT11

// Initialize DHT sensor object
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Initialize Serial connection
  Serial.begin(115200);
  delay(1000); // Wait for serial to start

  // Initialize the sensor
  dht.begin();
  Serial.println("Info: Started");
}

void loop() {
  // Give sensor time to reset
  delay(2000);

  // Read humidity and temperature (in C)
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check data validity
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Error: Failed to read from sensor");
    return;
  }

  // Print the results to the Serial Monitor
  Serial.print("Data: ");
  Serial.print(temperature);
  Serial.print(", ");
  Serial.println(humidity);
}
