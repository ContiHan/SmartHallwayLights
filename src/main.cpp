#include <WifiCredentials.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include "Arduino.h"
#include "ArduinoOTA.h"

#define PWM_CHANNEL_0 0
#define PWM_TIMER_13_BIT 13
#define PWM_BASE_FREQ 2000
#define PWM_PIN 25
#define MAX_DUTY 8192
#define DUTY_STEP 82
#define PWM_DELAY 50
#define TASK_DELAY 100

#define BREATH_IN_START 22
#define BREATH_IN_END 35
#define BREATH_OUT_START 35
#define BREATH_OUT_END 22
#define BREATH_IN_TIME 2000
#define BREATH_HOLD_IN_TIME 1000
#define BREATH_OUT_TIME 2000
#define BREATH_HOLD_OUT_TIME 2500

WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char *mdnsName = "smart-led-corridor";

String indexHtml;
String styleCss;
String javascript;
String appleTouchIcon;
String favicon32x32;
String favicon16x16;
String siteWebmanifest;
String androidChrome192x192;
String adnroidChrome512x512;

byte pwmValue = 0;
bool testingPWM = false;
bool breathing = false;
unsigned long startTimestamp;

void setupPWM();
void setPWMDutyCycle(int dutyCycle);
void cyclePWMTask(void *parameter);
void breathPWMTask(void *parameter);
void initTasks();
void setWifiConnection();
void setServerResponses();
void setFaviconServerResponses();
void unknownHtmlMessage();
void setTimeClient();
String elapsedTimeHtml();
void startWebServer();
void setupMDNS();
void initArduinoOTA();
void loadFileIntoMemory(const char *fileName, String &memory);
void redirectToRoot();
void initSPIFFSAndLoadFiles();
void fadeIn(int targetValue);
void fadeOut(int targetValue);

void setup()
{
  Serial.begin(115200);

  initSPIFFSAndLoadFiles();
  setupPWM();
  initTasks();
  setWifiConnection();
  setupMDNS();
  setServerResponses();
  setTimeClient();
  startWebServer();
  initArduinoOTA();
}

void fadeIn(int targetValue)
{
  for (int i = 0; i <= targetValue; i++)
  {
    pwmValue = i;
    setPWMDutyCycle(map(pwmValue, 0, 100, 0, MAX_DUTY));
    delay(PWM_DELAY);
  }
}

void fadeOut(int targetValue)
{
  for (int i = pwmValue; i >= targetValue; i--)
  {
    pwmValue = i;
    setPWMDutyCycle(map(pwmValue, 0, 100, 0, MAX_DUTY));
    delay(PWM_DELAY);
  }
}

void initSPIFFSAndLoadFiles()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  loadFileIntoMemory("/index.html", indexHtml);
  loadFileIntoMemory("/style.css", styleCss);
  loadFileIntoMemory("/script.js", javascript);
  loadFileIntoMemory("/apple-touch-icon.png", appleTouchIcon);
  loadFileIntoMemory("/favicon-32x32.png", favicon32x32);
  loadFileIntoMemory("/favicon-16x16.png", favicon16x16);
  loadFileIntoMemory("/site.webmanifest", siteWebmanifest);
  loadFileIntoMemory("/android-chrome-192x192.png", androidChrome192x192);
  loadFileIntoMemory("/android-chrome-512x512.png", adnroidChrome512x512);
}

void loadFileIntoMemory(const char *path, String &dest)
{
  File file = SPIFFS.open(path, "r");
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  dest = file.readString();
  file.close();
}

void initArduinoOTA()
{
  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      } });

  ArduinoOTA.begin();
}

void startWebServer()
{
  server.begin();
  Serial.println("HTTP server je zapnuty.");
  Serial.print("Otevři stránku: http://");
  Serial.print(mdnsName);
  Serial.println("/");
}

void setupMDNS()
{
  if (MDNS.begin(mdnsName))
  {
    Serial.println("MDNS responder je zapnuty.");
  }
}

void setTimeClient()
{
  timeClient.begin();
  timeClient.update();
  startTimestamp = timeClient.getEpochTime();
}

void initTasks()
{
  xTaskCreate(cyclePWMTask, "PWM Cycle Task", 2048, NULL, 1, NULL);
  xTaskCreate(breathPWMTask, "Breath PWM Task", 2048, NULL, 1, NULL);
}

void setWifiConnection()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPassword);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Pripojeno k WiFi siti: ");
  Serial.println(wifiSSID);
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());
}

void setServerResponses()
{
  server.on("/", []()
            { server.send(200, "text/html; charset=UTF-8", indexHtml); });

  server.on("/style.css", []()
            { server.send(200, "text/css", styleCss); });

  server.on("/script.js", []()
            { server.send(200, "application/javascript", javascript); });

  setFaviconServerResponses();

  server.on("/time-since-startup", []()
            { server.send(200, "text/plain", elapsedTimeHtml()); });

  server.on("/pwm-value", []()
            { server.send(200, "text/plain", String(pwmValue)); });

  server.on("/led-on", []()
            {
    fadeIn(28);
    Serial.println("PWM nastaveno na " + String(pwmValue) + "%");
    redirectToRoot(); });

  server.on("/led-off", []()
            {
    fadeOut(0);
    Serial.println("PWM nastaveno na " + String(pwmValue) + "%");
    redirectToRoot(); });

  server.on("/setPWM", []()
            {
  if (server.hasArg("pwm"))
  {
    int tempPwmValue = server.arg("pwm").toInt();
    pwmValue = (byte) constrain(tempPwmValue, 0, 100);
    setPWMDutyCycle(map(pwmValue, 0, 100, 0, MAX_DUTY));
    Serial.println("PWM nastaveno na " + String(pwmValue) + "%");
  }
  redirectToRoot(); });

  server.on("/breath-on", []()
            {
  breathing = true;
  Serial.println("Breathing mode turned on");
  redirectToRoot(); });

  server.on("/breath-off", []()
            {
  breathing = false;
  Serial.println("Breathing mode turned off");
  redirectToRoot(); });

  server.on("/testPWM", []()
            {
    testingPWM = true;
    Serial.println("Začíná test PWM cyklu");
    redirectToRoot(); });

  server.onNotFound(unknownHtmlMessage);
}

void setFaviconServerResponses()
{
  server.on("/apple-touch-icon.png", []()
            { server.send(200, "image/png", appleTouchIcon); });

  server.on("/favicon-32x32.png", []()
            { server.send(200, "image/png", favicon32x32); });

  server.on("/favicon-16x16.png", []()
            { server.send(200, "image/png", favicon16x16); });

  server.on("/android-chrome-192x192.png", []()
            { server.send(200, "image/png", androidChrome192x192); });

  server.on("/android-chrome-512x512.png", []()
            { server.send(200, "image/png", adnroidChrome512x512); });

  server.on("/site.webmanifest", []()
            { server.send(200, "application/manifest+json", siteWebmanifest); });
}

void redirectToRoot()
{
  server.sendHeader("Location", "/");
  server.send(303);
}

void unknownHtmlMessage()
{
  String message = "Neexistujici odkaz\n\n"
                   "URI: " +
                   server.uri() + "\n"
                                  "Metoda: " +
                   ((server.method() == HTTP_GET) ? "GET" : "POST") + "\n"
                                                                      "Argumenty: " +
                   String(server.args()) + "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

String elapsedTimeHtml()
{
  timeClient.update();
  unsigned long currentTimestamp = timeClient.getEpochTime();
  unsigned long elapsedTime = currentTimestamp - startTimestamp;

  unsigned long years = elapsedTime / (86400 * 365);
  elapsedTime %= (86400 * 365);
  unsigned long days = elapsedTime / 86400;
  elapsedTime %= 86400;
  unsigned long hours = elapsedTime / 3600;
  elapsedTime %= 3600;
  unsigned long minutes = elapsedTime / 60;
  elapsedTime %= 60;
  unsigned long seconds = elapsedTime;

  char buffer[20];
  sprintf(buffer, "%d.%03d.%02d:%02d:%02d", years, days, hours, minutes, seconds);
  return String(buffer);
}

void setupPWM()
{
  ledcSetup(PWM_CHANNEL_0, PWM_BASE_FREQ, PWM_TIMER_13_BIT);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL_0);
  setPWMDutyCycle(0);
}

void setPWMDutyCycle(int dutyCycle)
{
  ledcWrite(PWM_CHANNEL_0, dutyCycle);
}

void cyclePWMTask(void *parameter)
{
  for (;;)
  {
    if (testingPWM)
    {
      for (int duty = 0; duty <= MAX_DUTY; duty += DUTY_STEP)
      {
        setPWMDutyCycle(duty);
        vTaskDelay(pdMS_TO_TICKS(PWM_DELAY));
      }
      for (int duty = MAX_DUTY; duty >= 0; duty -= DUTY_STEP)
      {
        setPWMDutyCycle(duty);
        vTaskDelay(pdMS_TO_TICKS(PWM_DELAY));
      }
      testingPWM = false;
    }
    vTaskDelay(pdMS_TO_TICKS(TASK_DELAY));
  }
}

void breathPWMTask(void *parameter)
{
  for (;;)
  {
    if (breathing)
    {
      // Breath in
      for (int duty = BREATH_IN_START; duty <= BREATH_IN_END; duty++)
      {
        int scaledDuty = map(duty, 0, 100, 0, MAX_DUTY);
        setPWMDutyCycle(scaledDuty);
        vTaskDelay(pdMS_TO_TICKS(BREATH_IN_TIME / (BREATH_IN_END - BREATH_IN_START)));
      }
      // Hold breath
      vTaskDelay(pdMS_TO_TICKS(BREATH_HOLD_IN_TIME));
      // Breath out
      for (int duty = BREATH_OUT_START; duty >= BREATH_OUT_END; duty--)
      {
        int scaledDuty = map(duty, 0, 100, 0, MAX_DUTY);
        setPWMDutyCycle(scaledDuty);
        vTaskDelay(pdMS_TO_TICKS(BREATH_OUT_TIME / (BREATH_OUT_START - BREATH_OUT_END)));
      }
      // Hold breath
      vTaskDelay(pdMS_TO_TICKS(BREATH_HOLD_OUT_TIME - TASK_DELAY));
    }
    else
    {
      setPWMDutyCycle(map(pwmValue, 0, 100, 0, MAX_DUTY));
    }
    vTaskDelay(pdMS_TO_TICKS(TASK_DELAY));
  }
}

void loop()
{
  server.handleClient();
  ArduinoOTA.handle();
}