#include <SPI.h>
#include <SD.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define chipSelect D8
#define sensorPin A0 //sensor pin connected to analog pin A0

String dataFile = "sensor.txt";
bool SDFail, dataIsStored = false;

char ssid[] = "<SSID>";     //  your network SSID (name)
char pass[] = "<PASSWORD>"; // your network password

int status = WL_IDLE_STATUS;
char server[] = "<WEB SERVER ADDRESS>"; // your web server e.g. test.testapp.com

float latitude, longitude, speed;
int year, month, date, hour, minute, second;
String date_str, time_str, lat_str, lng_str, speed_str, fuel_str;
int pm;

float liquid_level;
float Fuel_level;
int liquid_percentage;
int top_level = 807;
int bottom_level = 70;
float I_Display = 0;

const String vehicle_str = "AB00C1234";

WiFiClient client;
File myFile;
TinyGPSPlus gps;
SoftwareSerial ss(4, 5);

void connectWiFi();
void writeData(String newData);
void deleteData();
String readData();
void sendData();
//void sendDataSerially();

void connectWiFi()
{
    Serial.println("Attempting to connect to WPA network...");
    Serial.print("SSID: ");
    Serial.println(ssid);

    status = WiFi.begin(ssid, pass);
    if (status != WL_CONNECTED)
    {
        Serial.println("Couldn't get a wifi connection. Will retry.");
    }
    else
    {
        Serial.println("Connected to wifi");
    }
}

String readData()
{
    String data;
    myFile = SD.open(dataFile);
    if (myFile)
    {
        while (myFile.available())
        {
//            Serial.println((char)myFile.read());
            data += (char)myFile.read();
        }
//        Serial.println(data);
        return String(data);
    }
    else
    {
        Serial.println("error reading file");
        return "error opening file";
    }
     myFile.close();
}

void writeData(String data)
{

    myFile = SD.open(dataFile, FILE_WRITE);
    if (myFile)
    {
        Serial.print("Writing to file...");
        myFile.println(data);
       
        Serial.println("done.");
    }
    else
    {
        Serial.println("error writing " + dataFile);
    }
    myFile.close();
}

void deleteData()
{
    Serial.print("Deleting the file...");
    SD.remove(dataFile);
    if (!SD.exists(dataFile))
    {
        Serial.println("done.");
    }
    else
    {
        Serial.println("error deleting " + dataFile);
    }
    myFile.close();
}

void sendData(String data)
{
   HTTPClient http;
   String link = "http://" + String(server) + "/add?data='[" + data + "]'" ;
   http.begin(link);
   int httpCode = http.GET();
   if ( httpCode > 0)
   {
       Serial.print("sent data. status: ");
       Serial.println(httpCode);
       if(dataIsStored) deleteData();
   }
   else
   {
       Serial.println(http.GET());
       Serial.println("Couldn't send data.");
   }

}

//void sendDataSerially()
//{
//    String data;
//    myFile = SD.open(dataFile);
//    if (myFile)
//    {
//        Serial.println("\nStarting connection...");
//        // if you get a connection, report back via serial:
//        if (client.connect(server, 80))
//        {
//            // Make a HTTP request:
//
//            while (myFile.available())
//            {
//                myFile.read(data, 125);
//                client.println("GET /add?data='[" + data + "]' HTTP/1.0");
//                client.println();
//                Serial.println("sent data.");
//            }
//            myFile.close();
//            deleteData();
//        }
//        else
//        {
//            Serial.println("Couldn't send data.");
//        }
//    }
//    else
//    {
//        Serial.println("error opening file");
//        return "error opening file";
//    }
//}

void setup()
{
    Serial.begin(115200);

    ss.begin(9600);

    pinMode(sensorPin, INPUT);

    Serial.print("Initializing SD card...");

    SDFail = !SD.begin(chipSelect);
    if (SDFail)
    {
        Serial.println("initialization failed!");
        
    } else {
        Serial.println("initialization done.");
    }
    connectWiFi();
}

void loop(void)
{

    // get Sensor data here ---

    while (ss.available() > 0)
    {
        if (gps.encode(ss.read()))
        {
            if (gps.location.isValid())
            {
                latitude = gps.location.lat();
                lat_str = String(latitude, 6);
                longitude = gps.location.lng();
                lng_str = String(longitude, 6);
            }

            if (gps.speed.isValid())
            {

                speed = gps.speed.kmph();
                speed_str = String(speed, 6);
            }

            if (gps.date.isValid())
            {
                date_str = "\"";
                date = gps.date.day();
                month = gps.date.month();
                year = gps.date.year();

                if (date < 10)
                    date_str = '0';
                date_str += String(date);

                date_str += "-";

                if (month < 10)
                    date_str += '0';
                date_str += String(month);

                date_str += "-";

                if (year < 10)
                    date_str += '0';
                date_str += String(year);
                date_str += "\"";
            }

            if (gps.time.isValid())
            {
                time_str = "\"";
                hour = gps.time.hour();
                minute = gps.time.minute();
                second = gps.time.second();

                minute = (minute + 30);
                if (minute > 59)
                {
                    minute = minute - 60;
                    hour = hour + 1;
                }
                hour = (hour + 5);
                if (hour > 23)
                    hour = hour - 24;

                if (hour >= 12)
                    pm = 1;
                else
                    pm = 0;

                hour = hour % 12;

                if (hour < 10)
                    time_str = '0';
                time_str += String(hour);

                time_str += ":";

                if (minute < 10)
                    time_str += '0';
                time_str += String(minute);

                time_str += ":";

                if (second < 10)
                    time_str += '0';
                time_str += String(second);

                if (pm == 1)
                    time_str += "PM";
                else
                    time_str += "AM";

                time_str += "\"";
            }
        }
    }

    liquid_level = analogRead(sensorPin);
    liquid_percentage = ((liquid_level - bottom_level) / top_level) * 100;
    Fuel_level = 100 - liquid_percentage;

    fuel_str = String(Fuel_level);

    // Prepare the response
    String data = "{";
    data += "\"lat\":" + lat_str + ",";
    data += "\"lng\":" + lng_str + ",";
    data += "\"time\":" + time_str + ",";
    data += "\"date\":" + date_str + ",";
    data += "\"speed\":" + speed_str + ",";
    data += "\"fuel\":" + fuel_str + ",";
    data += "\"vehicle\":\"" + vehicle_str + "\"";
    data += "}";

    // --- --- ---

    if (WiFi.status() == WL_CONNECTED)
    {
        if(dataIsStored){
            sendData(readData());
            dataIsStored = false;
        }
        sendData(data);
    }
    else
    {
        if (!SDFail){
            writeData(data);
            dataIsStored = true;
        } else {
            Serial.println('Could not store data due to SD failure.');
        }
        
    }

    delay(1000);
}
