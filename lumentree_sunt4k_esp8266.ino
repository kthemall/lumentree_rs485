#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// --------------------------------------------------
// WLAN
// --------------------------------------------------
const char* ssid     = ""; //WLAN SSID
const char* password = ""; //WLAN Password

// --------------------------------------------------
// MQTT
// --------------------------------------------------
const char* mqtt_server = ""; //ip-adresse des mqtt servers
const int   mqtt_port   = 1883; //ip port des mqtt servers
const char* mqtt_user   = "";  //mqtt user
const char* mqtt_pass   = ""; //mqtt password

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --------------------------------------------------
// ESP8266 ESP-12F
// --------------------------------------------------
#define RX_PIN    12 
#define TX_PIN    13
#define DE_RE_PIN 4

SoftwareSerial RS485(RX_PIN, TX_PIN);

const uint8_t  SLAVE_ID    = 1;
const uint32_t MODBUS_BAUD = 9600;

// --------------------------------------------------
// CRC16 Modbus
// --------------------------------------------------
uint16_t modbusCRC(uint8_t *buf, uint16_t len)
{
  uint16_t crc = 0xFFFF;

  for (uint16_t pos = 0; pos < len; pos++)
  {
    crc ^= buf[pos];

    for (uint8_t i = 0; i < 8; i++)
    {
      if (crc & 0x0001)
      {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }

  return crc;
}

// --------------------------------------------------
// WLAN verbinden
// --------------------------------------------------
void setupWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Verbinde WLAN");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Verbunden. IP: ");
  Serial.println(WiFi.localIP());
}

// --------------------------------------------------
// MQTT verbinden
// --------------------------------------------------
void mqttReconnect()
{
  while (!mqttClient.connected())
  {
    Serial.print("MQTT verbinden...");

    if (mqttClient.connect(
          "Lumentree1Monitor",
          mqtt_user,
          mqtt_pass))
    {
      Serial.println("OK");
    }
    else
    {
      Serial.print("Fehler ");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

// --------------------------------------------------
// RS485 senden
// --------------------------------------------------
void rs485Send(uint8_t *data, uint8_t len)
{
  digitalWrite(DE_RE_PIN, HIGH);

  delayMicroseconds(100);

  RS485.write(data, len);
  RS485.flush();

  delayMicroseconds(300);

  digitalWrite(DE_RE_PIN, LOW);
}

// --------------------------------------------------
// Register lesen
// --------------------------------------------------
int32_t readRegister(uint16_t reg)
{
  while (RS485.available())
  {
    RS485.read();
  }

  uint8_t request[8];

  request[0] = SLAVE_ID;
  request[1] = 0x03;

  request[2] = highByte(reg);
  request[3] = lowByte(reg);

  request[4] = 0x00;
  request[5] = 0x01;

  uint16_t crc = modbusCRC(request, 6);

  request[6] = lowByte(crc);
  request[7] = highByte(crc);

  rs485Send(request, 8);

  uint8_t buffer[32];
  uint8_t len = 0;

  uint32_t start = millis();

  while (millis() - start < 500)
  {
    while (RS485.available())
    {
      if (len < sizeof(buffer))
      {
        buffer[len++] = RS485.read();
      }
    }
    yield();
  }

  if (len < 7)
    return INT32_MIN;

  uint16_t receivedCRC =
      buffer[len - 2] |
      ((uint16_t)buffer[len - 1] << 8);

  uint16_t calculatedCRC =
      modbusCRC(buffer, len - 2);

  if (receivedCRC != calculatedCRC)
    return INT32_MIN;

  return ((uint16_t)buffer[3] << 8) | buffer[4];
}

// --------------------------------------------------
// MQTT senden
// --------------------------------------------------
void publishValues()
{
  int16_t gridPower    = (int16_t)readRegister(59);
  int16_t batteryPower = (int16_t)readRegister(61);
  int16_t homeLoad     = (int16_t)readRegister(67);

  char payload[16];

  snprintf(payload, sizeof(payload), "%d", gridPower);
  mqttClient.publish(
      "lumentree1/grid_power",
      payload,
      true);

  snprintf(payload, sizeof(payload), "%d", batteryPower);
  mqttClient.publish(
      "lumentree1/battery_power",
      payload,
      true);

  snprintf(payload, sizeof(payload), "%d", homeLoad);
  mqttClient.publish(
      "lumentree1/home_load",
      payload,
      true);

  Serial.println();
  Serial.println("========================================");

  Serial.print("Grid Power    : ");
  Serial.print(gridPower);
  Serial.println(" W");

  Serial.print("Battery Power : ");
  Serial.print(batteryPower);
  Serial.println(" W");

  Serial.print("Home Load     : ");
  Serial.print(homeLoad);
  Serial.println(" W");

  Serial.println("========================================");
}

// --------------------------------------------------
// Setup
// --------------------------------------------------
void setup()
{
  Serial.begin(115200);

  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  RS485.begin(MODBUS_BAUD);

  delay(1000);

  setupWifi();

  mqttClient.setServer(
      mqtt_server,
      mqtt_port);

  Serial.println();
  Serial.println("========================================");
  Serial.println("LUMENTREE1 MQTT MONITOR");
  Serial.println("========================================");
}

// --------------------------------------------------
// Loop
// --------------------------------------------------
void loop()
{
  if (!mqttClient.connected())
  {
    mqttReconnect();
  }

  mqttClient.loop();

  publishValues();

  delay(5000);
}