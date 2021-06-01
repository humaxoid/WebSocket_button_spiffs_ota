// Импортируем библиотеки
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <AsyncElegantOTA.h>

// Задаем сетевые настройки
const char* ssid = "***";
const char* password = "*********";
IPAddress local_IP(192, 168, 1, 68);   // Задаем статический IP-адрес:
IPAddress gateway(192, 168, 1, 102);   // Задаем IP-адрес сетевого шлюза:
IPAddress subnet(255, 255, 255, 0);    // Задаем маску сети:
IPAddress primaryDNS(8, 8, 8, 8);      // Основной ДНС (опционально)
IPAddress secondaryDNS(8, 8, 4, 4);    // Резервный ДНС (опционально)
AsyncWebServer server(80);             // Создаем сервер через 80 порт
AsyncWebSocket ws("/ws");              // Создаем объект WebSocket

// Запускаем SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Произошла ошибка при монтировании SPIFFS");
  }
  Serial.println("SPIFFS примонтировано успешно");
}

// Запускаем WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Подключаюсь к WiFi ...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    //    delay(500);
  }
  Serial.println(WiFi.localIP());
}

bool ledState1 = 0;
bool ledState2 = 0;
bool ledState3 = 0;
bool ledState4 = 0;
bool ledState5 = 0;

// Уведомляем клиентов о текущем состоянии светодиода
void notifyClients1() {ws.textAll(String(ledState1));}
void notifyClients2() {ws.textAll(String(ledState2 + 2));}
void notifyClients3() {ws.textAll(String(ledState3 + 4));}
void notifyClients4() {ws.textAll(String(ledState4 + 6));}
void notifyClients5() {ws.textAll(String(ledState5 + 8));}

/* функция обратного вызова, которая запускается всякий раз, когда мы получаем новые
  данные от клиентов по протоколу WebSocket. Если мы получаем сообщение “toggle”, мы
  переключаем значение переменной ledState. Кроме того, мы уведомляем всех клиентов,
  вызывая функцию notifyClients (). Таким образом, все клиенты получают уведомление об
  изменении и соответствующим образом обновляют интерфейс.*/

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle1") == 0) {ledState1 = !ledState1;notifyClients1();}
    if (strcmp((char*)data, "toggle2") == 0) {ledState2 = !ledState2;notifyClients2();}
    if (strcmp((char*)data, "toggle3") == 0) {ledState3 = !ledState3;notifyClients3();}
    if (strcmp((char*)data, "toggle4") == 0) {ledState4 = !ledState4;notifyClients4();}
    if (strcmp((char*)data, "toggle5") == 0) {ledState5 = !ledState5;notifyClients5();}
  }
}

// Настройка прослушивателя событий для обработки различных асинхронных шагов протокола WebSocket
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:                  // когда клиент вошел в систему
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:               // когда клиент вышел из системы
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:                     // при получении пакета данных от клиента
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:                     // в ответ на запрос ping
    case WS_EVT_ERROR:                    // при получении ошибки от клиента
      break;
  }
}

String processor(const String& var) {
  Serial.println(var);
  if (var == "STATE1") {if (ledState1) {return "ON";} else {return "OFF";}}
  if (var == "STATE2") {if (ledState2) {return "ON";} else {return "OFF";}}
  if (var == "STATE3") {if (ledState3) {return "ON";} else {return "OFF";}}
  if (var == "STATE4") {if (ledState4) {return "ON";} else {return "OFF";}}
  if (var == "STATE5") {if (ledState5) {return "ON";} else {return "OFF";}}
}

// Инициализация WebSocket
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  // Последовательный порт для отладки
  Serial.begin(115200);

  pinMode(32, OUTPUT); digitalWrite(32, LOW);
  pinMode(33, OUTPUT); digitalWrite(33, LOW);
  pinMode(25, OUTPUT); digitalWrite(25, LOW);
  pinMode(26, OUTPUT); digitalWrite(26, LOW);
  pinMode(27, OUTPUT); digitalWrite(27, LOW);

  // Настраиваем статический IP-адрес:
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Режим клиента не удалось настроить");
  }

  initSPIFFS();
  initWiFi();
  initWebSocket();

  // Начальная страница
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });
  server.serveStatic("/", SPIFFS, "/");

  // Запускаем ElegantOTA
  AsyncElegantOTA.begin(&server);

  // Запускаем сервер
  server.begin();
}

void loop() {
  AsyncElegantOTA.loop();
  ws.cleanupClients();

  digitalWrite(32, ledState1);
  digitalWrite(33, ledState2);
  digitalWrite(25, ledState3);
  digitalWrite(26, ledState4);
  digitalWrite(27, ledState5);
}
