# ISE-211 Project

This repo contains the Arduino code (ISE-211_Project folder) which simply reads a DHT11 sensor, and sends the reading over serial to a computer.

The code for the computer is a little more complicated. First, it will load a json config file. Next, it will load the json logger and print out the average over tha last 24 hours. Finally, the main loop will run, which will read the sensor, send alerts if needed, and save the reading to the logger. The serial reader connection will automatically connect and reconnect as needed.

Additionally, the computer code will only work on a Windows computer. If you switch to using the fake serial reader class, and likely comment out a few lines, you could probably get something working. Linux support will be added only if it is deemed needed.

## Initial overview
- Per-sensor readers (async or separate thread prefered):
    - Possible support for multiple weather stations
    - Possible support for only updating 1 sensor at a time
- Process new data:
    - Debug print data
    - Automatic warnings after passing threasholds
    - Store historical data
    - Possible there will be multiple locations, which means the data collector and data processor should be kept separate
- Store data as JSON (or a proper database)
- Per sensor:
    - Dedicated class per sensor
    - Warning threasholds stored in the class
    - Reader function (or channel?)
    - Store class/similar in separate .hpp file
    - Base class definition that all sensors import and base self off of
    - Warnings and errors should be returned and always checked/read from
- Sensor types: Temperature, Humidity, Wind speed, Pressure
- Error handling everywhere
- Create a UML Class Diagram
- Things to log (to a file):
    - Not sure if this is like the stored JSON data from earlier, or if it's different
    - Collected sensor data (Timestamp, Sensor type, Sensor reading, Geographic location)
- Ability to read historical data via: Date range, Sensor type, and Location
- Dedicated functions for: Daily averages, Alerts for anomalies, and Comparing data across locations (if we use multiple locations)
- 5 minute presentation (read project details)
