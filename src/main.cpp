#include <string>
#include <map>
#include <cstdint>

#include <ArduinoOTA.h>

#include "../lib/Relay/Relay.h"
#include "../lib/LED/LED.h"
#include "../lib/Button/Button.h"
#include "../lib/DataManager/DataManager.h"
#include "../lib/MqttManager/MqttManager.h"
#include "../lib/WifiManager/WifiManager.h"
#include "../lib/WebServer/WebServer.h"
#include "../lib/UpdateManager/UpdateManager.h"



//#################### FW DATA ####################

#define FW "electroblind"
#define FW_VERSION "0.0.1"

//#################### ======= ####################

#define DEVICE_TYPE "electrodragon"
#define LED_PIN 16
#define BUTTON1_PIN 2
#define BUTTON2_PIN 0
#define RELAY_UP_PIN 12
#define RELAY_DOWN_PIN 13


UpdateManager updateManager;
DataManager dataManager;
WifiManager wifiManager;
MqttManager mqttManager;
Relay relayUp;
Relay relayDown;
Button button1;
Button button2;
LED led;

std::string wifi_ssid = dataManager.getWifiSSID();
std::string wifi_password = dataManager.getWifiPass();
std::string ip = dataManager.getIP();
std::string mask = dataManager.getMask();
std::string gateway = dataManager.getGateway();
std::string ota = dataManager.getOta();
std::string mqtt_server = dataManager.getMqttServer();
std::string mqtt_port = dataManager.getMqttPort();
std::string mqtt_username = dataManager.getMqttUser();
std::string mqtt_password = dataManager.getMqttPass();
std::string device_name = dataManager.getDeviceName();
std::string mqtt_status = dataManager.getMqttTopic(0);
std::string mqtt_command = dataManager.getMqttTopic(1);

void blindUp()
{
    Serial.println("UP");
    relayDown.off();
    relayUp.on();
    mqttManager.publishMQTT(mqtt_status, "UP");
}

void blindDown()
{
    Serial.println("DOWN");
    relayUp.off();
    relayDown.on();
    mqttManager.publishMQTT(mqtt_status, "DOWN");
}

void blindStop()
{
    Serial.println("STOP");
    relayDown.off();
    relayUp.off();
    mqttManager.publishMQTT(mqtt_status, "STOP");
}

std::vector<std::pair<std::string, std::string>> getWebServerData()
{
    std::vector<std::pair<std::string, std::string>> webServerData;

    std::pair<std::string, std::string> generic_pair;

    generic_pair.first = "wifi_ssid";
    generic_pair.second = wifi_ssid;
    webServerData.push_back(generic_pair);

    generic_pair.first = "wifi_password";
    generic_pair.second = wifi_password;
    webServerData.push_back(generic_pair);

    generic_pair.first = "ip";
    generic_pair.second = ip;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mask";
    generic_pair.second = mask;
    webServerData.push_back(generic_pair);

    generic_pair.first = "gateway";
    generic_pair.second = gateway;
    webServerData.push_back(generic_pair);

    generic_pair.first = "ota_server";
    generic_pair.second = ota;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_server";
    generic_pair.second = mqtt_server;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_port";
    generic_pair.second = mqtt_port;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_username";
    generic_pair.second = mqtt_username;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_password";
    generic_pair.second = mqtt_password;
    webServerData.push_back(generic_pair);

    generic_pair.first = "device_name";
    generic_pair.second = device_name;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_status";
    generic_pair.second = mqtt_status;
    webServerData.push_back(generic_pair);

    generic_pair.first = "mqtt_command";
    generic_pair.second = mqtt_command;
    webServerData.push_back(generic_pair);

    generic_pair.first = "firmware_version";
    generic_pair.second = FW_VERSION;
    webServerData.push_back(generic_pair);

    generic_pair.first = "device_type";
    generic_pair.second = DEVICE_TYPE;
    webServerData.push_back(generic_pair);

    return webServerData;
}

void webServerSubmitCallback(std::map<std::string, std::string> inputFieldsContent)
{
    //Save config to dataManager
    Serial.println("webServerSubmitCallback");

    dataManager.setWifiSSID(inputFieldsContent["wifi_ssid"]);
    dataManager.setWifiPass(inputFieldsContent["wifi_password"]);
    dataManager.setIP(inputFieldsContent["ip"]);
    dataManager.setMask(inputFieldsContent["mask"]);
    dataManager.setGateway(inputFieldsContent["gateway"]);
    dataManager.setOta(inputFieldsContent["ota_server"]);
    dataManager.setMqttServer(inputFieldsContent["mqtt_server"]);
    dataManager.setMqttPort(inputFieldsContent["mqtt_port"]);
    dataManager.setMqttUser(inputFieldsContent["mqtt_username"]);
    dataManager.setMqttPass(inputFieldsContent["mqtt_password"]);
    dataManager.setDeviceName(inputFieldsContent["device_name"]);
    dataManager.setMqttTopic(0, inputFieldsContent["mqtt_status"]);
    dataManager.setMqttTopic(1, inputFieldsContent["mqtt_command"]);

    ESP.restart(); // Restart device with new config
}

void MQTTcallback(char* topic, byte* payload, unsigned int length)
{
    Serial.print("Message arrived from topic [");
    Serial.print(topic);
    Serial.println("] ");

    //ALWAYS DO THIS: Set end of payload string to length
    payload[length] = '\0'; //Do not delete

    std::string topicString(topic);
    std::string payloadString((char *)payload);

    if (topicString == mqtt_command)
    {
        if (payloadString == "UP")
        {
            blindUp();
        }
        else if (payloadString == "DOWN")
        {
            blindDown();
        }
        else if (payloadString == "STOP")
        {
            blindStop();
        }
        else
        {
            Serial.print("MQTT payload unknown: ");
            Serial.println(payloadString.c_str());
        }
    }
    else
    {
        Serial.print("MQTT topic unknown:");
        Serial.println(topicString.c_str());
    }
}

void button2_longlongPress()
{
    Serial.println("button2.longlongPress()");

    if(wifiManager.apModeEnabled())
    {
        WebServer::getInstance().stop();
        wifiManager.destroyApWifi();

        ESP.restart();
    }
    else
    {
        mqttManager.stopConnection();
        wifiManager.createApWifi();
        WebServer::getInstance().start();
    }
}

void setup()
{
    // Init serial comm
    Serial.begin(115200);

    // Configure Relay
    relayUp.setup(RELAY_UP_PIN, RELAY_HIGH_LVL);
    relayDown.setup(RELAY_DOWN_PIN, RELAY_HIGH_LVL);

    // Configure Buttons
    button1.setup(BUTTON1_PIN);
    button1.setShortPressCallback(blindStop);

    button2.setup(BUTTON2_PIN, PULLDOWN);
    button2.setShortPressCallback(blindUp);
    button2.setShortPressCallback(blindDown);
    button2.setLongLongPressCallback(button2_longlongPress);

    // Configure LED
    led.setup(LED_PIN, LED_LOW_LVL);
    led.on();
    delay(300);
    led.off();

    // Configure Wifi
    wifiManager.setup(wifi_ssid, wifi_password, ip, mask, gateway, DEVICE_TYPE);
    wifiManager.connectStaWifi();

    // Configure MQTT
    mqttManager.setup(mqtt_server, mqtt_port.c_str(), mqtt_username, mqtt_password);
    mqttManager.setDeviceData(device_name, DEVICE_TYPE, ip, FW, FW_VERSION);
    mqttManager.addStatusTopic(mqtt_status);
    mqttManager.addSubscribeTopic(mqtt_command);
    mqttManager.setCallback(MQTTcallback);
    mqttManager.startConnection();

    //Configure WebServer
    WebServer::getInstance().setup("/index.html.gz", webServerSubmitCallback);
    WebServer::getInstance().setData(getWebServerData());

    // OTA setup
    ArduinoOTA.setHostname(device_name.c_str());
    ArduinoOTA.begin();

    // UpdateManager setup
    updateManager.setup(ota, FW, FW_VERSION, DEVICE_TYPE);
}

void loop()
{
    // Process Buttons events
    button1.loop();
    button2.loop();

    // Check Wifi status
    wifiManager.loop();

    // Check MQTT status and OTA Updates
    if (wifiManager.connected())
    {
        mqttManager.loop();
        updateManager.loop();
        ArduinoOTA.handle();
    }

    // Handle WebServer connections
    if(wifiManager.apModeEnabled())
    {
        WebServer::getInstance().loop();
    }

    // LED Status
    if (mqttManager.connected())
    {
        led.on();
    }
    else if(wifiManager.apModeEnabled())
    {
        led.blink(1000);
    }
    else
    {
        led.off();
    }
}