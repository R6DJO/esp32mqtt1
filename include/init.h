/*
 * init.h
 *
 *  Created on: Jan 29, 2024
 *      Author: r6djo
 */

#ifndef INC_INIT_H_
#define INC_INIT_H_

#include <Arduino.h>
#include "secret.h"
#include <WiFi.h>
// #include <SPIFFS.h>

void initFS(void);
void initWiFi(void);

#endif /* INC_INIT_H_ */