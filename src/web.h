#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "main.h"
#include <FS.h>
#if defined(ESP32)
#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#endif
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

void init_web();
void add_web_log(String msg, uint32_t channel = 0);
