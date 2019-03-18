#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

// Selector Pins
#define RED_PIN 16
#define GREEN_PIN 5
#define BLUE_PIN 14
#define BRIGHTNESS_PIN 12

// Pot Pin
#define POT_PIN A0

// yeelight command related
String scenePrefix = "{\"id\":1,\"method\":\"set_scene\",\"params\":[\"color\",";
String sceneSuffix = "]}\r\n";
char discoverRequest[] = "M-SEARCH * HTTP/1.1\r\nMAN: \"ssdp:discover\"\r\nST: wifi_bulb\r\n";

int status = WL_IDLE_STATUS;
char ssid[] = "YeeController"; // SSID for the controller
char pass[] = "password";      // Password for the controller

// the rgb and brighness values
byte red = 255;
byte green = 255;
byte blue = 255;
byte brightness = 100;

// multicast port for searching yeelight.
IPAddress multicastIP(239, 255, 255, 250);
unsigned int multicastPort = 1982;

// ip address and port for the bulb
IPAddress bulbIP;
unsigned int bulbPort = 55443;
unsigned int localUDPPort = 2390;
unsigned int localTCPPort = 1234;

char packetBuffer[1024]; //buffer to hold incoming packet

WiFiManager wifiManager;
WiFiUDP Udp;
WiFiClient client;
WiFiServer server(localTCPPort);

void setup()
{
    Serial.begin(115200);

    // Setting pinmodes
    pinMode(RED_PIN, INPUT);
    pinMode(GREEN_PIN, INPUT);
    pinMode(BLUE_PIN, INPUT);
    pinMode(BRIGHTNESS_PIN, INPUT);

    wifiManager.setConfigPortalTimeout(180);
    wifiManager.autoConnect(ssid, pass);

    server.begin();
    server.setNoDelay(true);

    findBulb();
    setScene();
    Serial.println("Started!");
}

void findBulb()
{
    Serial.println("Finding Yeelight.");
    Udp.begin(localUDPPort);
    Udp.beginPacketMulticast(multicastIP, multicastPort, WiFi.localIP());
    Udp.write(discoverRequest);
    Udp.endPacket();
    while (1)
    {
        int packetSize = Udp.parsePacket();
        if (packetSize)
        {
            bulbIP = Udp.remoteIP();
            int len = Udp.read(packetBuffer, packetSize);
            if (len > 0)
            {
                packetBuffer[len] = 0;
            }
            Serial.printf("Found Yeelight. Response: %s\n", packetBuffer);
            setMusicMode();
            return;
        }
    }
}

void setMusicMode()
{
    WiFiClient tclient;
    tclient.connect(bulbIP, bulbPort);
    String command = "{\"id\":1,\"method\":\"set_music\",\"params\":[1,\"" + WiFi.localIP().toString() + "\"," + String(localTCPPort) + "]}";
    tclient.print(command);
    tclient.stop();
    Serial.println(command);
    Serial.println("hererer");
    Serial.println(tclient.read());
    Serial.println("hererer");
    while (1)
    {
        delay(1000);
        client = server.available();
        if (client)
        {
            Serial.println("Got client");
            Serial.println(client.remoteIP());
            break;
        }
        else
        {
            Serial.println("No client");
        }
    }
}

void loop()
{
    if (setColors(analogRead(POT_PIN)))
    {
        // setScene();
        // setMusicScene();
    }
    delay(100);
}

// setColors sets the color or brightness variables to the given value
// depending upon which one is selected. It will return true if a request
// to change the color should be sent (i.e. if the previous value and current
// value are the same, then no the function will return false).
bool setColors(int value)
{
    if (digitalRead(BRIGHTNESS_PIN) == HIGH)
    {
        value = constrain(value, 11, 1024);
        value = value / 10.24;
        if (value != brightness)
        {
            brightness = value;
            return true;
        }
    }
    else
    {
        value = constrain(value, 4, 1023);
        value = value / 4;
        if (digitalRead(RED_PIN) == HIGH)
        {
            if (value != red)
            {
                red = value;
                return true;
            }
        }
        else if (digitalRead(GREEN_PIN) == HIGH)
        {
            if (value != green)
            {
                green = value;
                return true;
            }
        }
        else if (digitalRead(BLUE_PIN) == HIGH)
        {
            if (value != blue)
            {
                blue = value;
                return true;
            }
        }
    }
    return false;
}

// setLightColor sends a request to the bulb to change the scene
void setMusicScene()
{
    String command = getColorCommand();
    Serial.println(command);
    client.print(command);
}

// setLightColor sends a request to the bulb to change the scene
void setScene()
{
    WiFiClient client;
    client.connect(bulbIP, bulbPort);
    String command = getColorCommand();
    Serial.println(command);
    client.print(command);
    client.stop();
}

// getColorCommand converts rgb, brighness value to the scene command
String getColorCommand()
{
    return scenePrefix + String(red * 65536 + green * 256 + blue) + "," + String(brightness) + sceneSuffix;
}
