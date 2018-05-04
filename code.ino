
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_BME280.h>
#include <ArduinoOTA.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS D7  // DS18B20 pin
const int led = LED_BUILTIN;

int timerNew;
int timerOld;

const int updateDB = 1000*60*15; //update every 15 mins
const char* ssid = "YOUR SSID";
const char* password = "YOUR PASSWORD";

//BME sensor variables
float hRead, tReadC, tToF, pRead;
char temperatureFString[6];
char temperatureCString[6];
char tempProbeStringC[6];
char tempProbeStringF[6];
char humidityString[6];
char pressureString[7];

//DS18B20 sensor variables
float tempProbeC,tempProbeF;

//ThingSpeak variables
String myWriteAPIKey = "YOUR WRITE KEY";
const char* thingspeakServer = "api.thingspeak.com";

//start webserver, wifi client, BME280 and DS18B20 sensor in the global scope
ESP8266WebServer server (80);
WiFiClient client;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
Adafruit_BME280 bme;

//Function to push data to ThingSpeak - 4 values every update
void updateThing(float ch1Value, float ch2Value, float ch3Value, float ch4Value) {
    if (client.connect(thingspeakServer, 80)) {
      String body = "field1=" + String(ch1Value) + "&";
             body += "field2=" + String(ch2Value) + "&";
             body += "field3=" + String(ch3Value) + "&";
             body += "field4=" + String(ch4Value);

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + myWriteAPIKey + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(body.length());
      client.print("\n\n");
      client.print(body);
      client.print("\n\n");
    }

     client.stop();
}

void handleRoot() {
  //Get the weather values
  getWeather();

  //html preperation
  char htmlChar[2500];

   snprintf ( htmlChar, 2500,
      "<!DOCTYPE html>\
      <html>\
        <head>\
          <meta charset=\"utf-8\" http-equiv=\"refresh\" content=\"60\">\
          <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
          <title>ESP8266 Weather Station</title>\
          <link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\" rel=\"stylesheet\">\
          <link href=\"https://getbootstrap.com/docs/4.0/examples/sticky-footer/sticky-footer.css\" rel=\"stylesheet\">\
        </head>\
        <body style=\"min-width: 500px;\">\
          <main role=\"main\" class=\"container\">\
            <h1 class=\"mt-5\">Live Weather Station</h1>\
            <p class=\"lead\">The station is located at <strong>New York, NY</strong></p>\
            <br>\
      <table class=\"table\" style='text-align: center;'>\
        <thead>\
          <tr>\
            <th scope=\"col\">Sensor</th>\
            <th scope=\"col\">Temprature</th>\
            <th scope=\"col\">Humidity</th>\
            <th scope=\"col\">Pressure</th>\
          </tr>\
        </thead>\
        <tbody>\
          <tr>\
            <th scope=\"row\">BME280</th>\
            <td>%s &deg;F (%s &deg;C)</td>\
            <td>%s %</td>\
            <td>%s Pa</td>\
          </tr>\
          <tr>\
            <th scope=\"row\">DS18B20</th>\
            <td>%s &deg;F (%s &deg;C)</td>\
            <td>-</td>\
            <td>-</td>\
          </tr>\
        </tbody>\
      </table>\
      <br>\
          <div class='history' style='text-align: center;'>\
          <iframe width=\"450\" height=\"260\" style=\"border: 1px solid #cccccc;\" src=\"https://thingspeak.com/channels/YOUR CHANNEL NUMBER/charts/3\?bgcolor=%%23ffffff&color=%%23d62020&dynamic=true&results=500&timescale=60&title=Temprature+History&type=line\"></iframe><br>\
          <iframe width=\"450\" height=\"260\" style=\"border: 1px solid #cccccc;\" src=\"https://thingspeak.com/channels/YOUR CHANNEL NUMBER/charts/2\?bgcolor=%%23ffffff&color=%%23d62020&dynamic=true&results=500&timescale=60&title=Humidity+History&type=line\"></iframe><br>\
          <iframe width=\"450\" height=\"260\" style=\"border: 1px solid #cccccc;\" src=\"https://thingspeak.com/channels/YOUR CHANNEL NUMBER/charts/4\?bgcolor=%%23ffffff&color=%%23d62020&dynamic=true&results=500&timescale=60&title=Pressure+History&type=line\"></iframe>\
          <div>\
          </main>\
          <br>\
          <footer class=\"footer\" style=\"position: fixed; bottom: 0\">\
            <div class=\"container-fluid\" style=\"margin-left: auto; margin-right: auto; text-align: center;\">\
              <span class=\"text-muted\">2018 &#169; </span>\
            </div>\
          </footer>\
        </body>\
      </html>",
      temperatureFString,temperatureCString, humidityString, pressureString,tempProbeStringF ,tempProbeStringC
        );

  //Send HTML to browser
  server.send ( 200, "text/html", htmlChar );

  //Clear memory
  ESP.getFreeHeap();
}

//404 responses
void handleNotFound() {
  String message = "Nothing Here!!! \n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.send ( 404, "text/plain", message );
}

//Get the weather values from sensors
void getWeather() {
    //Get the values from DS18B20
    DS18B20.requestTemperatures();
    tempProbeC = DS18B20.getTempCByIndex(0);
    tempProbeF = (tempProbeC*1.8)+32;
    delay(300);

    //Get the temprature from BME280
    hRead = bme.readHumidity();
    tReadC = bme.readTemperature();
    pRead = bme.readPressure()/100.0F;
    tToF = tReadC*1.8+32.0;
    delay(300);

    //Convert all to strings
    dtostrf(tempProbeC, 5, 1, tempProbeStringC);
    dtostrf(tempProbeF, 5, 1, tempProbeStringF);
    dtostrf(tToF, 5, 1, temperatureFString);
    dtostrf(tReadC, 5, 1, temperatureCString);
    dtostrf(hRead, 5, 1, humidityString);
    dtostrf(pRead, 6, 1, pressureString);
}

void setup ( void ) {
  Serial.begin ( 115200 );
  Serial.println("Booting");

  //Initialize wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin ( ssid, password );
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  //OTA update
  ArduinoOTA.onStart([]() {
  Serial.println("Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();


//Ready with IP
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if ( MDNS.begin ( "esp8266" ) ) {
    Serial.println ( "MDNS responder started" );
  }

  server.on ( "/", handleRoot );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );


  //Start BME280 sensor
  Wire.begin(D3, D4);
  delay(1000);
  Serial.println(F("BME280 test"));
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
  }


  //Blink the LED
  pinMode ( led, OUTPUT );
  for (int i=0 ; i < 10 ; i++){
    digitalWrite ( led, 0 );
    delay(50);
    digitalWrite ( led, 1 );
    delay(50);
  }
}

void loop (void) {
  ArduinoOTA.handle();
  server.handleClient();
  timerNew = millis()/updateDB; //update every 15 mins

  if (timerNew != timerOld){
    getWeather();
    updateThing(tReadC,hRead,tempProbeC,pRead);
    timerOld = timerNew;
  }
}
