#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "main.h"

byte nibble(char c);
void hexCharacterStringToBytes(byte *byteArray, String hexString);
void tohex(unsigned char * in, size_t insz, char * out, size_t outsz);
String split(String s, char parser, int index);
String array_to_hex_string(uint8_t *in_arr, size_t len, bool endian = false);