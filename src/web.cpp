#include <Arduino.h>
#include "main.h"
#include "utils.h"
#include "web.h"
#include <FS.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include "cc_interface.h"
#if defined(ESP32)
#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#endif
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266mDNS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#endif

#ifdef MANAGED_WIFI
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager/tree/feature_asyncwebserver
#endif

const char *http_username = "admin";
const char *http_password = "admin";
AsyncWebServer server(80);

uint32_t log_msgs_counter = 0;
static String log_msgs = "";
static bool is_in_run_mode = false;

void add_web_log(String msg, uint32_t channel)
{
  if (channel != 0)
  {
    int in_logs = log_msgs.indexOf(":\"" + String(channel) + "\":");
    if (in_logs != -1)
    {
      int start_pos = log_msgs.lastIndexOf("{", in_logs);
      int end_pos = log_msgs.indexOf("},", in_logs) + 2;
      log_msgs.remove(start_pos, end_pos - start_pos);
    }
  }
  else
  {
    Serial.println(msg);
  }
  log_msgs_counter++;
  log_msgs += "{\"" + String(log_msgs_counter) + "\":\"" + String(channel) + "\":\"" + msg + "\"},";
  while (log_msgs.length() > 5000)
  {
    // Remove older log messages if they where not pulled by now
    log_msgs = log_msgs.substring(log_msgs.indexOf("\"},") + 3);
  }
}

void http_get_status(AsyncWebServerRequest *request)
{
  if (!is_in_run_mode)
  {
    request->send(200, "text/plain", "9999");
    if (request->hasParam("received_log"))
    {
      int client_received_counter = request->getParam("received_log")->value().toInt();
      if (client_received_counter == 0)
        is_in_run_mode = true;
    }
  }
  else
  {
    if (request->hasParam("received_log") && request->getParam("received_log")->value() != "" && log_msgs != "")
    {
      int client_received_counter = request->getParam("received_log")->value().toInt();
      int log_main_pos = 0;
      int log_number = 0;
      do
      {
        int curr_last_log = log_msgs.indexOf("{\"", log_main_pos);
        if (curr_last_log == -1)
          break;
        int curr_last_log_end = log_msgs.indexOf("\"", curr_last_log + 2);
        if (curr_last_log_end == -1)
          break;
        int curr_last_end_of_log = log_msgs.indexOf("\"},", curr_last_log_end);
        if (curr_last_end_of_log == -1)
          break;
        log_number = log_msgs.substring(curr_last_log + 2, curr_last_log_end).toInt();
        if (log_number <= client_received_counter)
          log_main_pos = curr_last_end_of_log + 3;
      } while (log_number < client_received_counter);
      log_msgs = log_msgs.substring(log_main_pos);
    }
  }
  request->send(200, "text/plain", log_msgs);
}

void http_cc_erase(AsyncWebServerRequest *request)
{
  uint16_t device_id = cc.begin(CCDEBUG_CLK, CCDEBUG_DATA, CCDEBUG_RESET);
  add_web_log("Device id: " + array_to_hex_string((uint8_t *)&device_id, 2, true));
  add_web_log(array_to_hex_string((uint8_t *)&device_id, 2, true), 2);
  add_web_log("Chip erasing");
  cc.erase_chip();
  add_web_log("Chip erasing done");
  cc.reset_cc();
  request->send(200, "text/plain", "CC Chip erasing done");
}

void http_cc_debug_enter(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/plain");
  cc.enable_cc_debug();
  response->printf("Entered debug mode");
  add_web_log("Entered debug mode");
  request->send(response);
}

void http_cc_debug_exit(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/plain");
  cc.reset_cc();
  response->printf("Debug mode exit");
  add_web_log("Debug mode exit");
  request->send(response);
}

void http_cc_init(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/plain");
  uint16_t device_id = cc.begin(CCDEBUG_CLK, CCDEBUG_DATA, CCDEBUG_RESET);
  uint8_t status_byte = cc.send_cc_cmdS(0x34);
  response->printf("Device id: %04x Status: %02X", device_id, status_byte);
  add_web_log("Device id: " + array_to_hex_string((uint8_t *)&device_id, 2, true) + " Status: " + array_to_hex_string((uint8_t *)&status_byte, 1, true));
  add_web_log(array_to_hex_string((uint8_t *)&device_id, 2, true), 2);
  add_web_log((status_byte & 0x04) ? "Locked" : "unlocked", 3);
  request->send(response);
}

void http_cc_flash(AsyncWebServerRequest *request)
{
  String filename;
  if (request->hasParam("file"))
  {
    filename = "/" + request->getParam("file")->value();

    if (!SPIFFS.exists(filename))
    {
      add_web_log("Error file not existing");
      request->send(200, "text/plain", "Error file not existing");
      return;
    }

    File file = SPIFFS.open(filename, "rb");
    if (file == 0)
    {
      add_web_log("Error opening file");
      request->send(200, "text/plain", "Error opening file");
      return;
    }
    file.seek(0, SeekEnd);
    int file_size = file.position();
    file.seek(0, SeekSet);
    file.close();

    if (file_size == 0)
    {
      add_web_log("Error file to small: " + filename + " Len: " + String(file_size));
      request->send(200, "text/plain", "Error file to small: " + filename + " Len: " + String(file_size));
      return;
    }
    set_firmware_file(filename, file_size);

    add_web_log("OK flashing firmware now: " + filename + " Len: " + String(file_size));
    request->send(200, "text/plain", "OK flashing firmware now: " + filename + " Len: " + String(file_size));
    return;
  }
  request->send(200, "text/plain", "Wrong parameter");
}

void http_cc_dump(AsyncWebServerRequest *request)
{
  String filename;
  int dump_size;
  if (request->hasParam("file") && request->hasParam("size"))
  {
    filename = "/" + request->getParam("file")->value();
    dump_size = request->getParam("size")->value().toInt();

    set_firmware_dump_file(filename, dump_size);

    add_web_log("OK dumping firmware now: " + filename + " Len: " + String(dump_size));
    request->send(200, "text/plain", "OK dumping firmware now: " + filename + " Len: " + String(dump_size));
    return;
  }
  request->send(200, "text/plain", "Wrong parameter");
}

void http_cc_custom_cmd(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/plain");
  String cc_cmd;
  if (request->hasParam("custom_cc_cmd"))
  {
    cc_cmd = request->getParam("custom_cc_cmd")->value();
    if (cc_cmd.length() != 2)
    {
      response->printf("Length %i not correct", cc_cmd.length());
      request->send(response);
      return;
    }
    uint8_t cmd_in = 0;
    hexCharacterStringToBytes((uint8_t *)&cmd_in, cc_cmd);
    uint16_t cmd_answer = cc.send_cc_cmd(cmd_in);

    add_web_log("Device id: " + array_to_hex_string((uint8_t *)&cmd_answer, 2, true));
    response->printf("Answer to CMD: %04X", cmd_answer);
    request->send(response);
    return;
  }
  response->printf("Wrong parameter");
  request->send(response);
}

void http_cc_lock_byte(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/plain");
  String cc_cmd;
  if (request->hasParam("lock_cmd"))
  {
    cc_cmd = request->getParam("lock_cmd")->value();
    if (cc_cmd.length() != 2)
    {
      response->printf("Length %i not correct", cc_cmd.length());
      request->send(response);
      return;
    }
    uint8_t cmd_in = 0;
    hexCharacterStringToBytes((uint8_t *)&cmd_in, cc_cmd);
    uint16_t cmd_answer = cc.set_lock_byte(cmd_in);
    uint8_t status_byte = cc.send_cc_cmdS(0x34);
    add_web_log("Status: " + array_to_hex_string((uint8_t *)&status_byte, 1, true));
    add_web_log((status_byte & 0x04) ? "Locked" : "unlocked", 3);
    response->printf("Status: %02X", status_byte);
    request->send(response);
    return;
  }
  response->printf("Wrong parameter");
  request->send(response);
}

void init_web()
{
#ifdef STATIC_WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAME, WIFI_PWD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
#else
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("AutoConnectAP");
  if (!res)
  {
    Serial.println("Failed to connect");
    ESP.restart();
  }
#endif
  Serial.println("\r\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  // Make accessible via http://cc.local using mDNS responder
  if (!MDNS.begin("cc"))
  {
    Serial.println("Error setting up mDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);

  server.addHandler(new SPIFFSEditor(SPIFFS, http_username, http_password));

  server.on("/get_status", HTTP_ANY, http_get_status);// Put status handler at top to also be handled first as its called the most
  server.on("/custom_cc_cmd", HTTP_ANY, http_cc_custom_cmd);
  server.on("/lock_cc_cmd", HTTP_ANY, http_cc_lock_byte);
  server.on("/flash_cc", HTTP_ANY, http_cc_flash);
  server.on("/dump_cc", HTTP_ANY, http_cc_dump);
  server.on("/cc_init", HTTP_ANY, http_cc_init);
  server.on("/cc_enter_dbg", HTTP_ANY, http_cc_debug_enter);
  server.on("/cc_exit_dbg", HTTP_ANY, http_cc_debug_exit);
  server.on("/erase_cc", HTTP_ANY, http_cc_erase);

  server.on("/heap", HTTP_ANY, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(ESP.getFreeHeap())); });

  server.on("/used_flash", HTTP_ANY, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(SPIFFS.usedBytes())); });

  server.on("/free_flash", HTTP_ANY, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(SPIFFS.totalBytes() - SPIFFS.usedBytes())); });

  server.on("/size_flash", HTTP_ANY, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(SPIFFS.totalBytes())); });

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");

  server.onNotFound([](AsyncWebServerRequest *request)
                    {
                      if (request->url() == "/" || request->url() == "index.htm")
                      { // not uploaded the index.htm till now so notify the user about it
                        request->send(200, "text/html", "please use <a href=\"/edit\">/edit</a> with login defined in web.cpp to uplaod the supplied index.htm to get full useage");
                        return;
                      }
                      Serial.printf("NOT_FOUND: ");
                      if (request->method() == HTTP_GET)
                        Serial.printf("GET");
                      else if (request->method() == HTTP_POST)
                        Serial.printf("POST");
                      else if (request->method() == HTTP_DELETE)
                        Serial.printf("DELETE");
                      else if (request->method() == HTTP_PUT)
                        Serial.printf("PUT");
                      else if (request->method() == HTTP_PATCH)
                        Serial.printf("PATCH");
                      else if (request->method() == HTTP_HEAD)
                        Serial.printf("HEAD");
                      else if (request->method() == HTTP_OPTIONS)
                        Serial.printf("OPTIONS");
                      else
                        Serial.printf("UNKNOWN");
                      Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

                      if (request->contentLength())
                      {
                        Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
                        Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
                      }
                      int headers = request->headers();
                      int i;
                      for (i = 0; i < headers; i++)
                      {
                        AsyncWebHeader *h = request->getHeader(i);
                        Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
                      }
                      int params = request->params();
                      for (i = 0; i < params; i++)
                      {
                        AsyncWebParameter *p = request->getParam(i);
                        if (p->isFile())
                        {
                          Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
                        }
                        else if (p->isPost())
                        {
                          Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
                        }
                        else
                        {
                          Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
                        }
                      }
                      request->send(404); });
  server.begin();
}
