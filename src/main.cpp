#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define PWM_CHANNEL_0 0
#define PWM_TIMER_13_BIT 13
#define PWM_BASE_FREQ 2000
#define PWM_PIN 25

const char *wifiSSID = "Cono'lin_RD";
const char *wifiPassword = "KldPo.2023";

unsigned long blinkInterval = 1000;
unsigned long pwmValue = 0;
bool testingPWM = false;

WebServer server(80);

void setupPWM();
void setPWMDutyCycle(int dutyCycle);
void cyclePWMTask(void *parameter);
void blinkLEDTask(void *parameter);
void printTickTask(void *parameter);
void initTasks();
void setWifiConnection();
void setServerResponses();
void mainHtmlMessage();
void unknownHtmlMessage();

void setup()
{
  Serial.begin(115200);

  setupPWM();
  setPWMDutyCycle(0);

  initTasks();

  setWifiConnection();

  String mdnsName = "LED-lightning-corridor";
  if (MDNS.begin(mdnsName))
  {
    Serial.println("MDNS responder je zapnuty.");
  }

  setServerResponses();

  server.begin();
  Serial.println("HTTP server je zapnuty.");
  Serial.print("Otevři stránku: http://");
  Serial.print(mdnsName);
  Serial.println("/");
}

void initTasks()
{
  xTaskCreate(cyclePWMTask, "PWM Cycle Task", 2048, NULL, 1, NULL);
  xTaskCreate(blinkLEDTask, "Blink LED Task", 2048, NULL, 1, NULL);
  xTaskCreate(printTickTask, "Print Tick Task", 2048, NULL, 1, NULL);
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

  server.on("/blink250ms", []()
            {
    blinkInterval = 250;
    Serial.println("Spuštěno blikání po 250ms");
    mainHtmlMessage(); });

  server.on("/blink2000ms", []()
            {
    blinkInterval = 2000;
    Serial.println("Spuštěno blikání po 2000ms");
    mainHtmlMessage(); });

  server.on("/pwm0", []()
            {
    setPWMDutyCycle(0);
    pwmValue = 0;
    Serial.println("PWM na 0%");
    mainHtmlMessage(); });

  server.on("/pwm1", []()
            {
    setPWMDutyCycle(82);
    pwmValue = 1;
    Serial.println("PWM na 1%");
    mainHtmlMessage(); });

  server.on("/pwm5", []()
            {
    setPWMDutyCycle(410);
    pwmValue = 5;
    Serial.println("PWM na 5%");
    mainHtmlMessage(); });

  server.on("/pwm10", []()
            {
    setPWMDutyCycle(819);
    pwmValue = 10;
    Serial.println("PWM na 10%");
    mainHtmlMessage(); });

  server.on("/pwm15", []()
            {
    setPWMDutyCycle(1229);
    pwmValue = 15;
    Serial.println("PWM na 15%");
    mainHtmlMessage(); });

  server.on("/pwm20", []()
            {
    setPWMDutyCycle(1638);
    pwmValue = 20;
    Serial.println("PWM na 20%");
    mainHtmlMessage(); });

  server.on("/pwm30", []()
            {
    setPWMDutyCycle(2458);
    pwmValue = 30;
    Serial.println("PWM na 30%");
    mainHtmlMessage(); });

  server.on("/pwm50", []()
            {
    setPWMDutyCycle(4096);
    pwmValue = 50;
    Serial.println("PWM na 50%");
    mainHtmlMessage(); });

  server.on("/pwm100", []()
            {
    setPWMDutyCycle(8192);
    pwmValue = 100;
    Serial.println("PWM na 100%");
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
  String timeFromStartInSecond = String(millis() / 1000);
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
                       "<p>Čas od spuštění Arduina je " +
                       timeFromStartInSecond + " vteřin.</p>"
                                               "<p>Interval blikání LED: " +
                       String(blinkInterval) + "ms</p>"
                                               "<p>PWM nastaveno na: " +
                       String(pwmValue) + "%</p>"
                                          "<p><a href=\"/blink250ms\">Blikej po 250ms</a></p>"
                                          "<p><a href=\"/blink2000ms\">Blikej po 2000ms</a></p>"
                                          "<p><a href=\"/pwm0\">PWM 0%</a></p>"
                                          "<p><a href=\"/pwm1\">PWM 1%</a></p>"
                                          "<p><a href=\"/pwm5\">PWM 5%</a></p>"
                                          "<p><a href=\"/pwm10\">PWM 10%</a></p>"
                                          "<p><a href=\"/pwm15\">PWM 15%</a></p>"
                                          "<p><a href=\"/pwm20\">PWM 20%</a></p>"
                                          "<p><a href=\"/pwm30\">PWM 30%</a></p>"
                                          "<p><a href=\"/pwm50\">PWM 50%</a></p>"
                                          "<p><a href=\"/pwm100\">PWM 100%</a></p>"
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

void blinkLEDTask(void *parameter)
{
  for (;;)
  {
    Serial.println("led blink..");
    vTaskDelay(pdMS_TO_TICKS(blinkInterval)); // Čekání na další "bliknutí"
  }
}

void printTickTask(void *parameter)
{
  for (;;)
  {
    Serial.println("tick..");
    vTaskDelay(pdMS_TO_TICKS(5000)); // Opakování každých 5 sekund
  }
}

void loop()
{
  server.handleClient();
}