#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "Arduino.h"

#define PWM_CHANNEL_0 0
#define PWM_TIMER_13_BIT 13
#define PWM_BASE_FREQ 2000
#define PWM_PIN 25

WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char *wifiSSID = "Cono'lin_RD";
const char *wifiPassword = "KldPo.2023";

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

void setup()
{
  Serial.begin(115200);

  setupPWM();
  setPWMDutyCycle(0);

  initTasks();

  setWifiConnection();

  String mdnsName = "smart-led-corridor";
  if (MDNS.begin(mdnsName))
  {
    Serial.println("MDNS responder je zapnuty.");
  }

  setServerResponses();

  setTimeClient();

  server.begin();
  Serial.println("HTTP server je zapnuty.");
  Serial.print("Otevři stránku: http://");
  Serial.print(mdnsName);
  Serial.println("/");
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
  server.on("/", mainHtmlMessage);

  server.on("/setPWM", []()
            {
  if (server.hasArg("pwm"))
  {
    pwmValue = (byte) server.arg("pwm").toInt();
    setPWMDutyCycle(map(pwmValue, 0, 100, 0, 8192));
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
                       "<h1>Ahoj Arduino světe!</h1>"
                       "<p>Čas od spuštění je " +
                       timeFromStart + "</p>"
                                       "<p>PWM nastaveno na: " +
                       String(pwmValue) + "%</p>"
                                          "<form action=\"/setPWM\" method=\"get\">"
                                          "<label for=\"pwm\">PWM:</label>"
                                          "<input type=\"number\" id=\"pwm\" name=\"pwm\" min=\"0\" max=\"100\">"
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
      for (int duty = 0; duty <= 8192; duty += 82)
      {
        setPWMDutyCycle(duty);
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      for (int duty = 8192; duty >= 0; duty -= 82)
      {
        setPWMDutyCycle(duty);
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      testingPWM = false;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void loop()
{
  server.handleClient();
}