#include <Arduino.h>
#include "init.h"
#include <WiFiClientSecure.h>

#include <PubSubClient.h>
#include <GyverBME280.h> // Подключение библиотеки

#include "secret.h"

// Define MQTT topics for publishing sensor data
#define pres_p "gb_iot/3216_FAV/pres"
#define pres_h "gb_iot/3216_FAV/pres_hg"
#define temp   "gb_iot/3216_FAV/temp"

// Initialize the BME280 sensor
GyverBME280 bme;

// Function prototypes
void mqtt_loop();
int mqtt_sub_reconnect();
void handle_msg(char *topic, unsigned char *payload, unsigned int length);

// Initialize MQTT clients and WiFi
WiFiClientSecure net_client_pub;
PubSubClient mqtt_client_pub;

WiFiClientSecure net_client_sub;
PubSubClient mqtt_client_sub;

const char *host = "mqtt.iotserv.ru";
const int port = 8883;

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);

    // Connect to WiFi network
    initWiFi();
    // WiFi.begin(W_ssid, W_pwd);

    // // Wait for WiFi connection
    // while (WiFi.status() != wl_status_t::WL_CONNECTED)
    // {
    //     Serial.print(".");
    //     delay(1000);
    // }
    // Serial.println("\nConnected");

    // Initialize BME280 sensor
    bme.begin();

    // Set CA certificate, client certificate, and private key for the publisher client
    net_client_pub.setCACert(rootCA);
    net_client_pub.setCertificate(pub_cert);
    net_client_pub.setPrivateKey(pub_key);

    // Initialize the publisher MQTT client
    mqtt_client_pub.setClient(net_client_pub);
    mqtt_client_pub.setServer(host, port);
    mqtt_client_pub.connect("test_1148_kad");

    // Set CA certificate, client certificate, and private key for the subscriber client
    net_client_sub.setCACert(rootCA);
    net_client_sub.setCertificate(sub_cert);
    net_client_sub.setPrivateKey(sub_key);

    // Initialize the subscriber MQTT client
    mqtt_client_sub.setClient(net_client_sub);
    mqtt_client_sub.setServer(host, port);
    mqtt_client_sub.setCallback(handle_msg);

    Serial.print("MQTT connect status: ");
    Serial.println(mqtt_client_pub.connected());

    // Attempt to connect to the MQTT broker for the subscriber
    mqtt_sub_reconnect();

    // Publish an initialization message to the MQTT topic
    mqtt_client_pub.publish("gb_iot/3216_FAV/init", "Hello!");
}

void publish_bme_data()
{
    // Read the temperature from the BME280 sensor and convert it to a String
    boolean status = false;
    String TempVal = String(bme.readTemperature());

    // Print the temperature value in degrees Celsius
    Serial.print("Temperature: ");
    Serial.print(TempVal);
    Serial.println(" *C");

    // Convert the temperature value to a character array with one decimal place
    char t[20];
    sprintf(t, "%0.0f", bme.readTemperature());

    // Publish the temperature value to the MQTT topic 'temp'
    status |= ~(mqtt_client_pub.publish(temp, t, strlen(t)));

    // Read the pressure from the BME280 sensor in Pascal
    float pressure = bme.readPressure();

    // Print the pressure value in hectopascals
    Serial.print("Pressure: ");
    Serial.print(pressure / 100.0F); // Convert from Pascal to hectopascals
    Serial.print(" hPa , ");

    // Print the pressure value in millimeters of mercury
    Serial.print(pressureToMmHg(pressure));
    Serial.println(" mm Hg");

    // Convert the pressure value in Pascal to a character array with one decimal place
    char p[20];
    sprintf(p, "%0.0f", pressure);

    // Publish the pressure value in Pascal to the MQTT topic 'pres_p'
    status |= ~(mqtt_client_pub.publish(pres_p, p, strlen(p)));

    // Convert the pressure value in millimeters of mercury to a character array with one decimal place
    sprintf(p, "%0.1f", pressureToMmHg(pressure));

    // Publish the pressure value in millimeters of mercury to the MQTT topic 'pres_h'
    status |= ~(mqtt_client_pub.publish(pres_h, p, strlen(p)));
    Serial.print("Pub: status ");
    Serial.println(status);
}

// Publish the current value to the MQTT topic
const uint32_t TRANSMISSION_INTERVAL_MS = 6000;

void publish_bme_values()
{
    static uint32_t last_transmission_time = 0;
    uint32_t current_time = millis();

    if (current_time - last_transmission_time > TRANSMISSION_INTERVAL_MS)
    {
        if (!mqtt_client_pub.connected())
        {
            mqtt_client_pub.connect("test_1148_kad");
            Serial.print("MQTT connect status: ");
            Serial.println(mqtt_client_pub.connected());
        }
        else
        {
            publish_bme_data();
        }
        last_transmission_time = current_time;
    }
}

// The void loop function is where the main code runs repeatedly in a loop.
void loop()
{
    // Put your main code here, to run repeatedly:

    // publish_value() is a function that is used to publish a value.
    publish_bme_values();

    // mqtt_loop() is a function that handles the MQTT loop.
    mqtt_loop();
}

// Subscription message handling function
void handle_msg(char *topic, unsigned char *payload, unsigned int length)
{
    // Add null-terminator to the payload for printing as a string
    payload[length] = '\0';

    // Print received message details in format: sub: <topic>: <payload>
    Serial.printf("Sub: %s: %s\n", (char *)topic, (char *)payload);

    // TODO: Parse received message
    // Add code here to parse and process the received message according to the project requirements
}

// Function for handling MQTT loop operations
void mqtt_loop()
{
    // Initialize a static variable to store the last reconnect attempt time
    static uint32_t last_reconnect_attempt = 0;

    // Check if the MQTT client is not connected
    if (!mqtt_client_sub.connected())
    {
        // Get the current time in milliseconds
        uint32_t now = millis();

        // Check if the difference between the current time and the last reconnect attempt time is greater than 5000 milliseconds
        if (now - last_reconnect_attempt > 5000)
        {
            // Update the last reconnect attempt time
            last_reconnect_attempt = now;

            // Attempt to reconnect the MQTT client
            if (mqtt_sub_reconnect())
            {
                // If the reconnection is successful, reset the last reconnect attempt time
                last_reconnect_attempt = 0;
            }
        }
    }
    else
    {
        // If the MQTT client is connected, process incoming messages and maintain the connection
        mqtt_client_sub.loop();
        ;
    }
}

// Function name: mqtt_sub_reconnect
// Description: This function is used to reconnect the MQTT subscriber.
// It first attempts to connect to the MQTT broker with the client ID "test_adk_sub".
// If the connection is successful, it prints "Sub connected" to the serial monitor.
// Then, it subscribes to the topic "gb_iot/#", which means it will receive messages from all topics under "gb_iot/".
// Returns: A boolean value indicating whether the MQTT client is currently connected.
int mqtt_sub_reconnect()
{
    // Attempt to connect to the MQTT broker with the client ID "test_adk_sub"
    if (mqtt_client_sub.connect("test_adk_sub"))
    {
        // If the connection is successful, print "Sub connected" to the serial monitor
        Serial.println("Sub: connected");
        // Subscribe to the topic "gb_iot/#"
        mqtt_client_sub.subscribe("gb_iot/#");
    }
    // Return whether the MQTT client is currently connected
    return mqtt_client_sub.connected();
}
