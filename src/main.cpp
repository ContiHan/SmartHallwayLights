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

WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char *mdnsName = "smart-led-corridor";

byte pwmValue = 0;
bool testingPWM = false;
unsigned long startTimestamp;

void setupPWM();
void setPWMDutyCycle(int dutyCycle);
void cyclePWMTask(void *parameter);
void initTasks();
void setWifiConnection();
void setServerResponses();
void mainHtmlMessage();
void unknownHtmlMessage();
void setTimeClient();
String elapsedTimeHtml();
void startWebServer();
void setupMDNS();
void initArduinoOTA();

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  setupPWM();
  initTasks();
  setWifiConnection();
  setupMDNS();
  setServerResponses();
  setTimeClient();
  startWebServer();
  initArduinoOTA();
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
  // Nový handler pro "/test"
  server.on("/test", HTTP_GET, []()
            {
    File file = SPIFFS.open("/index.html", "r"); // Používá stejný index.html soubor
    server.streamFile(file, "text/html");
    file.close(); });

  // Nový handler pro CSS specifické pro testovací stránku
  server.on("/test/style.css", HTTP_GET, []()
            {
    File file = SPIFFS.open("/style.css", "r"); // Používá stejný style.css soubor
    server.streamFile(file, "text/css");
    file.close(); });

  server.on("/", mainHtmlMessage);

  server.on("/setPWM", []()
            {
  if (server.hasArg("pwm"))
  {
    int tempPwmValue = server.arg("pwm").toInt();
    pwmValue = (byte) constrain(tempPwmValue, 0, 100);
    setPWMDutyCycle(map(pwmValue, 0, 100, 0, MAX_DUTY));
    Serial.println("PWM nastaveno na " + String(pwmValue) + "%");
  }
  mainHtmlMessage(); });

  server.on("/testPWM", []()
            {
    testingPWM = true;
    Serial.println("Začíná test PWM cyklu");
    mainHtmlMessage(); });

  server.onNotFound(unknownHtmlMessage);
}

void mainHtmlMessage()
{
  String timeFromStart = elapsedTimeHtml();
  String htmlMessage = "<!DOCTYPE html>"
                       "<html lang=\"cs\">"
                       "<head>"
                       "<meta charset=\"UTF-8\">"
                       "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                       "<title>Smart LED Corridor</title>"
                       "<link rel=\"icon\" href=\"https://t2.gstatic.com/faviconV2?client=SOCIAL&type=FAVICON&fallback_opts=TYPE,SIZE,URL&url=http://dev.to&size=16\" type=\"image/x-icon\">"
                       "<style>"
                       "body { font-family: Arial, sans-serif; margin: 20px; padding: 20px; }"
                       "h1 { color: #333366; }"
                       "p { color: #666666; }"
                       "a { background-color: #4CAF50; color: white; padding: 10px 20px; text-decoration: none; display: inline-block; }"
                       "a:hover { background-color: #45a049; }"
                       "</style>"
                       "</head>"
                       "<body>"
                       "<h1>Chytré osvětlení chodby</h1>"
                       "<p>Čas od spuštění je " +
                       timeFromStart + "</p>"
                                       "<p>PWM nastaveno na: " +
                       String(pwmValue) + "%</p>"
                                          "<form action=\"/setPWM\" method=\"get\">"
                                          "<label for=\"pwm\">PWM:</label>"
                                          "<input type=\"number\" id=\"pwm\" name=\"pwm\" min=\"0\" max=\"100\" required>"
                                          "<input type=\"submit\" value=\"Nastavit\">"
                                          "</form>"
                                          "<p><a href=\"/testPWM\">Test PWM</a></p>"
                                          "</body>"
                                          "</html>";

  server.send(200, "text/html", htmlMessage);
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

  unsigned long days = elapsedTime / 86400;
  elapsedTime %= 86400;
  unsigned long hours = elapsedTime / 3600;
  elapsedTime %= 3600;
  unsigned long minutes = elapsedTime / 60;
  elapsedTime %= 60;
  unsigned long seconds = elapsedTime;

  return String(days) + " dní " + String(hours) + " hodin " + String(minutes) + " minut " + String(seconds) + " sekund";
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

void loop()
{
  server.handleClient();
  ArduinoOTA.handle();
}