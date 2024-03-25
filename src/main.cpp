#include <Arduino.h>

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <GyverBME280.h>
#include "init.h"

#define LED 2

// Define MQTT topics for publishing sensor data
#define mqtt_prefix "gb_iot/3216_FAV/"
#define hello "gb_iot/3216_FAV/init"
#define pres_p "gb_iot/3216_FAV/pres"
#define pres_h "gb_iot/3216_FAV/pres_hg"
#define temp "gb_iot/3216_FAV/temp"
#define led_mode_blink "gb_iot/3216_FAV/ledblink"

// Initialize the BME280 sensor
GyverBME280 bme;

// Function prototypes
void mqtt_sub_loop();
boolean mqtt_sub_reconnect();
void handle_msg(char *topic, unsigned char *payload, unsigned int length);

// Initialize MQTT clients and WiFi
WiFiClientSecure net_client_pub;
PubSubClient mqtt_client_pub;

WiFiClientSecure net_client_sub;
PubSubClient mqtt_client_sub;

const char *host = "mqtt.iotserv.ru";
const int port = 8883;

int led_mode = 0;

void setup()
{
    pinMode(LED, OUTPUT);
    // Initialize serial communication
    Serial.begin(115200);

    // Connect to WiFi network
    initWiFi();

    // Initialize BME280 sensor
    bme.begin();

    while (!WiFi.isConnected())
    {
        Serial.print(".");
        delay(333);
    }
    Serial.println();

    // Set CA certificate, client certificate, and private key for the publisher client
    net_client_pub.setCACert(ROOT_CA);
    net_client_pub.setCertificate(PUB_CERT);
    net_client_pub.setPrivateKey(PUB_KEY);

    // Initialize the publisher MQTT client
    mqtt_client_pub.setClient(net_client_pub);
    mqtt_client_pub.setServer(host, port);
    mqtt_client_pub.connect("3216_FAV_pub");

    // Set CA certificate, client certificate, and private key for the subscriber client
    net_client_sub.setCACert(ROOT_CA);
    net_client_sub.setCertificate(sub_cert);
    net_client_sub.setPrivateKey(sub_key);

    // Initialize the subscriber MQTT client
    mqtt_client_sub.setClient(net_client_sub);
    mqtt_client_sub.setServer(host, port);
    mqtt_client_sub.setCallback(handle_msg);

    Serial.printf("MQTT Pub connect status: %s\n", mqtt_client_pub.connected() ? "Connected" : "Disconnected");
    // Publish an initialization message to the MQTT topic
    mqtt_client_pub.publish(hello, "Hello!");

    // Attempt to connect to the MQTT broker for the subscriber
    mqtt_sub_reconnect();
}

void publish_bme_data()
{
    // Initialize a boolean variable to track the success of the publishing process
    boolean status = false;

    // Read the temperature from the BME sensor and convert it to a string
    String TempVal = String(bme.readTemperature());

    // Print the temperature value to the Serial monitor
    Serial.print("Temperature: ");
    Serial.print(TempVal);
    Serial.println(" *C");

    // Create a character array to store the formatted temperature value
    char t[20];

    // Format the temperature value with zero decimal places and store it in the character array
    sprintf(t, "%0.0f", bme.readTemperature());

    // Publish the temperature value to the specified MQTT topic
    status |= !mqtt_client_pub.publish(temp, t);

    // Read the pressure from the BME sensor
    float pressure = bme.readPressure();

    // Print the pressure value to the Serial monitor
    Serial.print("Pressure: ");
    Serial.print(pressure / 100.0F);
    Serial.print(" hPa, ");
    Serial.print(pressureToMmHg(pressure));
    Serial.println(" mm Hg");

    // Format the pressure value with zero decimal places and store it in the character array
    char p[20];
    sprintf(p, "%0.0f", pressure);

    // Publish the pressure value in hPa to the specified MQTT topic
    status |= !mqtt_client_pub.publish(pres_p, p);

    // Format the pressure value with one decimal place and store it in the character array
    sprintf(p, "%0.1f", pressureToMmHg(pressure));

    // Publish the pressure value in mmHg to the specified MQTT topic
    status |= !mqtt_client_pub.publish(pres_h, p);

    // Print the status of the publishing process to the Serial monitor
    Serial.printf("MQTT Pub send topic's status: %s\n", !status ? "OK" : "Problem");
}

void mqtt_pub_loop()
{
    boolean status = mqtt_client_pub.connected();
    const uint32_t TRANSMISSION_INTERVAL_MS = 6000;
    static uint32_t last_transmission_time = 0;
    uint32_t current_time = millis();

    if (current_time - last_transmission_time > TRANSMISSION_INTERVAL_MS)
    {
        if (!status)
        {
            status = mqtt_client_pub.connect("3216_FAV_pub");
            Serial.printf("MQTT Pub connect status: %s\n", status ? "Connected" : "Disconnected");
        }
        if (status)
        {
            publish_bme_data();
            last_transmission_time = current_time;
        }
    }
}

void led_update()
{
    static unsigned long previousMillis = 0;
    const long interval = 500;
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        switch (led_mode)
        {
        case 0:
            digitalWrite(LED, LOW);
            break;
        case 1:
            digitalWrite(LED, !digitalRead(LED));
            break;
        default:

            break;
        }
    }
}

// The void loop function is where the main code runs repeatedly in a loop.
void loop()
{
    if (WiFi.isConnected())
    {
        // mqtt_pub_loop() is a function that is used to publish a value.
        mqtt_pub_loop();

        // mqtt_sub_loop() is a function that handles the MQTT loop.
        mqtt_sub_loop();
    }
    led_update();
}

// Subscription message handling function
void handle_msg(char *topic, unsigned char *payload, unsigned int length)
{
    // Add null-terminator to the payload for printing as a string
    payload[length] = '\0';

    // Print received message details in format: sub: <topic>: <payload>
    // Serial.printf("MQTT Sub rcvd: %s: %s\n", (char *)topic, (char *)payload);

    // TODO: Parse received message
    // Add code here to parse and process the received message according to the project requirements
    if (strcmp(topic, led_mode_blink) == 0)
    {
        led_mode = atoi((const char *)payload);
        Serial.printf("MQTT Sub rcvd: %s: %s\n", (char *)topic, (char *)payload);
    }
}

void mqtt_sub_loop()
{
    static uint32_t last_reconnect_attempt = 0;
    uint32_t now = millis();
    if (!mqtt_client_sub.connected() && now - last_reconnect_attempt > 15000)
    {
        last_reconnect_attempt = now;
        mqtt_sub_reconnect();
    }
    mqtt_client_sub.loop();
}

boolean mqtt_sub_reconnect()
{
    boolean status = mqtt_client_sub.connect("3216_FAV_sub");
    if (status)
    {
        Serial.println("MQTT Sub connect status: Connected");
        status = mqtt_client_sub.subscribe("gb_iot/#");
    }
    if (status)
    {
        Serial.println("MQTT Sub connect status: Subscribed");
    }
    return mqtt_client_sub.connected();
}
