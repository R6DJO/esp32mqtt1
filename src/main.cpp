#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <PubSubClient.h>
#include <GyverBME280.h> // Подключение библиотеки

#include "secret.h"

// топики
#define pres_p "gb_iot/3216_FAV/Pres_P(BMP280)"
#define pres_h "gb_iot/3216_FAV/Pres_H(BMP280)"
#define temp   "gb_iot/3216_FAV/Temp(BMP280)"
// #define humi "gb_iot/3216_FAV"

GyverBME280 bme;

void mqtt_loop();
int mqtt_sub_reconnect();
void handle_msg(char *topic, unsigned char *payload, unsigned int length);

WiFiClientSecure net_client_pub;
PubSubClient mqtt_client_pub;

WiFiClientSecure net_client_sub;
PubSubClient mqtt_client_sub;

const char *host = "mqtt.iotserv.ru";
const int port = 8883;

void setup()
{
    Serial.begin(115200);
    WiFi.begin(W_ssid, W_pwd);

    while (WiFi.status() != wl_status_t::WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nConnected");

    bme.begin();

    net_client_pub.setCACert(rootCA);
    net_client_pub.setCertificate(pub_cert);
    net_client_pub.setPrivateKey(pub_key);

    mqtt_client_pub.setClient(net_client_pub);
    mqtt_client_pub.setServer(host, port);

    mqtt_client_pub.connect("test_1148_kad");

    net_client_sub.setCACert(rootCA);
    net_client_sub.setCertificate(sub_cert);
    net_client_sub.setPrivateKey(sub_key);

    mqtt_client_sub.setClient(net_client_sub);
    mqtt_client_sub.setServer(host, port);

    mqtt_client_sub.setCallback(handle_msg);

    Serial.print("mqtt connect status: ");
    Serial.println(mqtt_client_pub.connected());

    mqtt_sub_reconnect();

    mqtt_client_pub.publish("gb_iot/3216_FAV/init", "Hello!");
}

void pub_bme()
{
    String TempVal = String(bme.readTemperature());
    Serial.print("Temperature: ");
    Serial.print(TempVal);
    Serial.println(" *C");
    char t[20];
    sprintf(t, "%0.1f", bme.readTemperature());
    mqtt_client_pub.publish(temp, t, strlen(t));

    float pressure = bme.readPressure(); // Читаем давление в [Па]
    Serial.print("Pressure: ");
    Serial.print(pressure / 100.0F); // Выводим давление в [гПа]
    Serial.print(" hPa , ");
    Serial.print(pressureToMmHg(pressure)); // Выводим давление в [мм рт. столба]
    Serial.println(" mm Hg");
    char p[20];
    sprintf(p, "%0.1f", pressure);
    mqtt_client_pub.publish(pres_p, p, strlen(p));
    sprintf(p, "%0.1f", pressureToMmHg(pressure));
    mqtt_client_pub.publish(pres_h, p, strlen(p));
}

// Publish the current value to the MQTT topic
void publish_value()
{
    static uint32_t last_transmission_time = 0;
    static int value = 0;
    if (millis() - last_transmission_time > 6000)
    {
        char buffer[6] = {0};
        snprintf(buffer, sizeof(buffer), "%d", value);

        mqtt_client_pub.publish("gb_iot/3216_FAV/val", buffer);
        pub_bme();

        last_transmission_time = millis();
        value++;
    }
}

void loop()
{
    // Put your main code here, to run repeatedly:
    publish_value();
    mqtt_loop();
}

void handle_msg(char *topic, unsigned char *payload, unsigned int length)
{
    {
        payload[length] = '\0';
        Serial.printf("sub: %s: %s\n", (char *)topic, (char *)payload);
        // Serial.print(": ");
        // Serial.println((char *)payload);
    }
}

void mqtt_loop()
{
    static uint32_t last_reconnect_attempt = 0;
    if (!mqtt_client_sub.connected())
    {
        uint32_t now = millis();
        if (now - last_reconnect_attempt > 5000)
        {
            last_reconnect_attempt = now;
            // Attempt to reconnect
            if (mqtt_sub_reconnect())
            {
                last_reconnect_attempt = 0;
            }
        }
    }
    else
    {
        mqtt_client_sub.loop();
    }
}

int mqtt_sub_reconnect()
{
    if (mqtt_client_sub.connect("test_adk_sub"))
    {
        Serial.println("Sub connected");
        // mqtt_client_sub.subscribe("gb_iot/3216_FAV/led_mode");
        mqtt_client_sub.subscribe("gb_iot/#");
    }
    return mqtt_client_sub.connected();
}
