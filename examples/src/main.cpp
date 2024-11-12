#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <DHTesp.h>
#include <ConfigPortal32.h>

char*               ssid_pfix = (char*)"CaptivePortal";
String              user_config_html = ""
                    "<p><input type = 'text' name = 'meta.address' placeholder= 'InfluxDB Adress'>"
                    "<p><input type = 'text' name = 'meta.token' placeholder= 'InfluxDB Token'>"
                    "<p><input type = 'text' name = 'meta.bucket' placeholder= 'InfluxDB Bucket'>"
                    "<p><input type = 'text' name = 'meta.interval' placeholder= 'Report Interval'>";
char                yourVar[50];
unsigned long       lastDHTReadMillis = 0;
float               humidity = 0;
float               temperature = 0;
DHTesp              dht;

void readDHT22() {
    unsigned long currentMillis = millis();

    if(currentMillis - lastDHTReadMillis >= (int)(const char *)cfg["meta"]["interval"][0]) {
        lastDHTReadMillis = currentMillis;

        humidity = dht.getHumidity();              // Read humidity (percent)
        temperature = dht.getTemperature();             // Read temperature as Fahrenheit
    }
}

/*
 *  ConfigPortal library to extend and implement the WiFi connected IOT device
 *
 *  Yoonseok Hur
 *
 *  Usage Scenario:
 *  0. copy the example template in the README.md
 *  1. Modify the ssid_pfix to help distinquish your Captive Portal SSID
 *          char   ssid_pfix[];
 *  2. Modify user_config_html to guide and get the user config data through the Captive Portal
 *          String user_config_html;
 *  2. declare the user config variable before setup
 *  3. In the setup(), read the cfg["meta"]["your field"] and assign to your config variable
 *
 */

void setup() {
    Serial.begin(115200);

    loadConfig();
    // *** If no "config" is found or "config" is not "done", run configDevice ***
    if(!cfg.containsKey("config") || strcmp((const char*)cfg["config"], "done")) {
        configDevice();
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // main setup
    Serial.printf("\nIP address : "); Serial.println(WiFi.localIP());

    if (MDNS.begin("miniwifi")) {
        Serial.println("MDNS responder started");
    }    
}

void loop() {
    WiFiClient client;
    HTTPClient http;

    Serial.println((const char*)cfg["address"]);

    String urlbuf;
    char data [200];
    readDHT22();
    Serial.printf("Temperature: %.2f\n , Humidity= %.2f", temperature, humidity);

    urlbuf = "http://" + (String)(const char *)cfg["meta"]["address"] + "/write?db=" + (String)(const char *)cfg["meta"]["bucket"];
    // URL Preparation
    // line protocol data preparation
    sprintf(data, "ambient,location=room02 temperature=%.1f,humidity=%.1f", temperature, humidity);
    // http begin
    http.begin(client, urlbuf);
    // Add Authorization Token to Header
    http.addHeader("Authorization", String("Token ") + (const char*)cfg["meta"]["token"]);
    // post the line protocol data
    int httpCode = http.POST(String(data));
}