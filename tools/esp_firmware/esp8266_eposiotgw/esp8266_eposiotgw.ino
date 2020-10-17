#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <time.h>

extern "C" {
    #include "user_interface.h"
    #include "c_types.h"
    #include "wpa2_enterprise.h"
}

#define IOTHOST   "iot.lisha.ufsc.br"
#define IOTCREATE "/api/create.php"
#define IOTATTACH "/api/attach.php"
#define IOTPUT    "/api/put.php"
#define IOTPORT   80

// Server RSA public key
// Extracted by: openssl x509 -pubkey -noout -in servercert.pem
//static const char pubkey[] PROGMEM = R"KEY(
//-----BEGIN PUBLIC KEY-----
//MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA11J2szu2zPx43rpdmBjo
//xS4N02eU2o8uHfOB+3j6obhVCIiOQtlriHwhHywjYWNylsKPxdUucUHDyMHnCJlS
//kHe/dPVmd9tW4/W/ZkROhWUuXkJrdHJ3fs8EKjKwIYwTTfs/wKLA3C6EfNQhOaZy
//EqtkKtEg6yy0qrJv3Y7e+1V6YnPiEdUGBcBYDNbTZSegNGlz9QB0xNRJH59j0tG4
//Elzyokrv3pv6jYgwl7e2EQU6L/YdRBT259npE+z7WqX0iNe8PETrSNjP4lsDP0PJ
//1mieNsAg55b7l+CJa5EabopI4/ncZ/xvXNsfTqDNmx1KIdgXhDqIy5ZQ61Ez8dKt
//DQIDAQAB
//-----END PUBLIC KEY-----
//)KEY";

static const long int DEFAULT_TIMEOUT = 60 * 1000;

unsigned long int init_timeout;
bool first = true;

bool check_timeout() {
    if (millis() > init_timeout) {
        return true;
    }
    return false;
}

time_t now() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    while (now < 1514764800) { // Just to avoid wrong time value
        delay(2);
        now = time(nullptr);
    }
    return now;
}

bool connect_to_wifi() {
    if(WiFi.status() == WL_CONNECTED)
        return true;
    
    const char *ssid = ""; //SSID of the network
    const char *username = ""; //identity for WPA2-Enterprise network
    const char *pass = ""; //pass of the network, or of your credential
    
    //Comment the following lines if the network is not WPA2-Enterprise
    wifi_station_set_wpa2_enterprise_auth(1);
    wifi_station_set_enterprise_identity((uint8*)username, strlen(username));
    wifi_station_set_enterprise_username((uint8*)username, strlen(username));
    wifi_station_set_enterprise_password((uint8*)pass, strlen(pass));
    wifi_station_set_enterprise_new_password((uint8*)pass, strlen(pass)); 
    WiFi.begin(ssid, 0);

    //Uncomment the following line with the network is WPA2-PSK
    //WiFi.begin(ssid, pass);
  
    init_timeout = millis() + DEFAULT_TIMEOUT;
    while(WiFi.status() != WL_CONNECTED) { 
        if(check_timeout())
            ESP.restart();
        delay(0);
    }
    if(WiFi.status() != WL_CONNECTED)
        return false;
    else
        return true;
}

int post(char * message, int message_size, bool series=false) {
    HTTPClient client;
    
    String s_route = (series == true) ? IOTATTACH : IOTPUT;
    
    client.begin(IOTHOST,(uint16_t)IOTPORT,s_route);

    int httpCode = client.POST((uint8_t *)message,(size_t)message_size);

    client.end();

    return httpCode;
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(0);
    WiFi.mode(WIFI_STA);
//    Serial.print(now());
//    Serial.print('X');
//    delay(5000);
}

void loop() {
    if((Serial.available() > 0) && connect_to_wifi()) {
        char c = Serial.read();
        if(c == 'T' || first) {
            Serial.print('X');
            Serial.print('X');
            Serial.print('X');
            Serial.print(now());
            Serial.print('X');
            Serial.print('X');
            Serial.print('X');
            first = false;
        }
        else if((c == 'S') || (c == 'R')) {
            char data[50];
            unsigned int count=0;
            unsigned int X = 0;
            while(X < 3) {
                if(Serial.available() > 0) {
                    char d = Serial.read();
                    data[count] = d;
                    count++;
                    if(d == 'X')
                        X++;
                    else
                        X = 0;
                }
            }
            if(c == 'S')
                post(data,count-3,true);
            else
                post(data,count-3);
        }
    }
}
