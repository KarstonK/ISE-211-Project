#include "serial_reader.hpp"
#include "json_things.hpp"
#include "sensors.hpp"

#include <cstdio>
#include <windows.h>

int main() {
    // printf writes UTF-8 bytes for the degree symbol below.
    SetConsoleOutputCP(CP_UTF8);
    // Read config file
    ConfigReader cfg("config.json");
    cfg.loadConfigOrCreateDefaults();

    // Load sensor history
    SensorDataLogger logger("sensor_log.json");
    logger.loadHistory();

    // Get yesterday's average
    logger.averageLast(chrono::hours(24));
    printf("Yesterday's average: ");

    // Create ESP's serial reader
    SerialReader reader(cfg.config().port, cfg.config().baud);
    //FakeReader reader;

    // Create sensor objects
    Sensor tempSensor("Temperature", cfg.config().alarmTempLow, cfg.config().alarmTempHigh);
    Sensor humSensor("Humidity", cfg.config().alarmHumidityLow, cfg.config().alarmHumidityHigh);

    while (true) {
        // Read the sensor data
        auto result = reader.read();

        // Check if the read was successful
        if (!result) {
            printf("[ERROR] %s\n", result.error().c_str());
            continue;
        }

        // If the read was successful, print the sensor data
        auto [temperature, humidity] = result.value();
        printf("[DATA] Temperature: %.2f", temperature);
        printf(", Humidity: %.2f\n", humidity);

        // Alert if any reading is out of range
        tempSensor.test(temperature);
        humSensor.test(humidity);

        // Log the reading
        logger.addReadingAndSave(temperature, humidity);
    }
}
