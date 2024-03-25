/*
 * init.c
 *
 *  Created on: Jan 29, 2024
 *      Author: r6djo
 */

// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <Arduino.h>
#include "init.h"
#include "wificonf.h"

// void initFS()
// {
//     if (!SPIFFS.begin())
//     {
//         Serial.println("HMI: An error has occurred while mounting SPIFFS");
//     }
//     else
//     {
//         Serial.println("HMI: SPIFFS mounted successfully");
//     }
// }

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("WiFi connected");
    Serial.print("IP address: http://");
    Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    Serial.println("Trying to Reconnect");
    WiFi.begin(SSID, PASSWORD);
}

void initWiFi()
{
    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.begin(SSID, PASSWORD);
}
