#include "utils.h"
#include <Arduino.h>

byte nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;
}

void hexCharacterStringToBytes(byte *byteArray, String hexString)
{
  bool oddLength = hexString.length() & 1;

  byte currentByte = 0;
  byte byteIndex = 0;

  for (byte charIndex = 0; charIndex < hexString.length(); charIndex++)
  {
    bool oddCharIndex = charIndex & 1;
    if (oddLength)
    {
      if (oddCharIndex)
      {
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
    else
    {
      if (!oddCharIndex)
      {
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
  }
}

void tohex(unsigned char *in, size_t insz, char *out, size_t outsz)
{
  unsigned char *pin = in;
  const char *hex = "0123456789ABCDEF";
  char *pout = out;
  for (; pin < in + insz; pout += 3, pin++)
  {
    pout[0] = hex[(*pin >> 4) & 0xF];
    pout[1] = hex[*pin & 0xF];
    pout[2] = ':';
    if (pout + 3 - out > outsz)
    {
      break;
    }
  }
  pout[-1] = 0;
}

String split(String s, char parser, int index)
{
    String rs = "";
    int parserCnt = 0;
    int rFromIndex = 0, rToIndex = -1;
    while (index >= parserCnt)
    {
        rFromIndex = rToIndex + 1;
        rToIndex = s.indexOf(parser, rFromIndex);
        if (index == parserCnt)
        {
            if (rToIndex == 0 || rToIndex == -1)
                return "";
            return s.substring(rFromIndex, rToIndex);
        }
        else
            parserCnt++;
    }
    return rs;
}

String array_to_hex_string(uint8_t *in_arr, size_t len, bool endian)
{
  String out_string = "";
  const char *hex = "0123456789ABCDEF";
  if(endian)
  {
  for(int i = len-1;i>=0;i--)
  {
    out_string += String(hex[(in_arr[i] >> 4) & 0xF]) + String(hex[in_arr[i] & 0xF]);
  }
  }
  else
  {
  for(int i = 0;i<len;i++)
  {
    out_string += String(hex[(in_arr[i] >> 4) & 0xF]) + String(hex[in_arr[i] & 0xF]);
  }
  }
  return out_string;
}