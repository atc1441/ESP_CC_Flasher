#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "web.h"
#include "utils.h"
#include "spiffs.h"
#include "cc_interface.h"
#include <FS.h>
#if defined(ESP32)
#include "SPIFFS.h"
#endif

boolean should_do_firmware_flashing = false;
int flashing_size = 0;
String firmware_filename = "";
void set_firmware_file(String firmware, int file_size)
{
  flashing_size = file_size;
  firmware_filename = firmware;
  should_do_firmware_flashing = true;
}

void flash_firmware()
{
  should_do_firmware_flashing = false;
  File file = SPIFFS.open(firmware_filename, "rb");
  if (file == 0)
    return;
  uint8_t *ptr = (uint8_t *)malloc(flashing_size);
  if (ptr == nullptr)
  {
    add_web_log("No memory to allocate the buffer needed");
    return;
  }

  file.seek(0, SeekSet);
  file.read(ptr, flashing_size);
  file.close();
  uint16_t device_id = cc.begin(CCDEBUG_CLK, CCDEBUG_DATA, CCDEBUG_RESET);
  add_web_log("Device id: " + array_to_hex_string((uint8_t *)&device_id, 2, true));
  add_web_log(array_to_hex_string((uint8_t *)&device_id, 2, true), 2);
  add_web_log("Chip erasing");
  cc.erase_chip();
  add_web_log("Chip erasing done");

  for (int retry = 3; retry > 0; retry--)
  {
    add_web_log("Flashing start");
    cc.write_code_memory(0x0000, ptr, flashing_size);
    add_web_log("Flashing done");
    add_web_log("Verify start");
    if (cc.verify_code_memory(0x0000, ptr, flashing_size) == 0)
    {
      add_web_log("Verify successful");
      break;
    }
    else
    {
      add_web_log("Verify failed");
    }
  }
  free(ptr);
  cc.reset_cc();
  add_web_log("CC reset");
}

boolean should_do_firmware_dumping = false;
int dumping_size = 0;
String firmware_dump_filename = "";

void set_firmware_dump_file(String firmware, int file_size)
{
  dumping_size = file_size;
  firmware_dump_filename = firmware;
  should_do_firmware_dumping = true;
}

void dump_firmware()
{
  should_do_firmware_dumping = false;
  if (SPIFFS.totalBytes() - SPIFFS.usedBytes() < dumping_size)
  {
    add_web_log("not enough empty flash on esp32");
    return;
  }
  File file = SPIFFS.open(firmware_dump_filename, "w");
  if (file == 0)
  {
    add_web_log("Could not create file");
    return;
  }
  uint8_t *ptr = (uint8_t *)malloc(dumping_size);
  if (ptr == nullptr)
  {
    add_web_log("No memory to allocate the buffer needed");
    return;
  }

  uint16_t device_id = cc.begin(CCDEBUG_CLK, CCDEBUG_DATA, CCDEBUG_RESET);
  add_web_log("Device id: " + array_to_hex_string((uint8_t *)&device_id, 2, true));
  add_web_log("Reading flash start");
  cc.read_code_memory(0x0000, dumping_size, ptr);
  file.write(ptr, dumping_size);
  file.close();
  free(ptr);
  cc.reset_cc();
  add_web_log("CC reset");
  add_web_log("Reading done <a href=\"" + firmware_dump_filename + "\" target=\"_blank\" download>" + firmware_dump_filename + "</a>");
}

void procent_callback(uint8_t percent)
{
  add_web_log(String(percent), 1);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\r\n\r\nWelcome to CC Flasher\r\n");
  SPIFFS.begin(true);
  init_web();
  Serial.println("\r\nSetup done\r\n");
  cc.set_callback(procent_callback);
}

void loop()
{
  if (should_do_firmware_flashing)
    flash_firmware();
  if (should_do_firmware_dumping)
    dump_firmware();
}