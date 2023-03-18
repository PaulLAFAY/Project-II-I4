// Dependencies
#include <Arduino.h>
#include <MKRWAN.h>
#include <Ultrasonic.h>
#include <SPI.h>
#include <MFRC522.h>

// Ultrasonic sensor
Ultrasonic ultrasonic(5, 4);

// Delay between two distance measurement's sending
#define LOOP_DELAY 20000

// LoRa modem
LoRaModem modem;
const String appEui = "A8610A32371E8208";
const String appKey = "A8610A32371E8208FFFFFFFFFFFFFFFF";
const String devAddr = "260BC256";
const String nwkSKey = "63E01DA54406B5EBE3C4862A853D2C26";
const String appSKey = "4B990AE6550B1B862C38CD800DC936D1";

// RFID reader
#define RST_PIN 6
#define SS_PIN 7
MFRC522 rfid(SS_PIN, RST_PIN);

// Flame sensor
#define flamePin 0

// Time handler
int timeStart;
int timeNow;

// Fonction prototype
void sendFrame(byte *data, const int len, const byte type);

void setup()
{
  // put your setup code here, to run once:
  pinMode(flamePin, INPUT);
  Serial.begin(115200);
  while (!Serial)
    ;
  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_DumpVersionToSerial();
  Serial.println("Scan PICC to see UID, type, and data blocks...");
  if (!modem.begin(EU868))
  {
    Serial.println("Failed to start module");
    exit(EXIT_FAILURE);
  }
  if (!modem.joinOTAA(appEui, appKey))
  {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    exit(EXIT_FAILURE);
  }
  Serial.println("Setup done");
  timeStart = millis();
}

void loop()
{
  // put your main code here, to run repeatedly:

  // Distance
  timeNow = millis();
  if (timeNow - timeStart >= LOOP_DELAY)
  {
    byte distance = (byte)(ultrasonic.read() / 2);
    sendFrame(&distance, 1, 1);
    Serial.print("distance : ");
    Serial.println(distance * 2);
    timeStart = timeNow;
  }

  // Fire
  while (digitalRead(flamePin) == LOW)
  {
    sendFrame(NULL, 0, 2);
    Serial.println("Fire");
    delay(LOOP_DELAY);
  }

  // RFID
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return;
  sendFrame(rfid.uid.uidByte, rfid.uid.size, 3);
  Serial.print("Card UID :");
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
  rfid.PICC_HaltA();
}

void sendFrame(byte *data, const int len, const byte type)
{
  byte *payload = (byte *)calloc(len + 1, sizeof(byte));
  payload[0] = type;
  for (int i = 0; i < len; i++)
    payload[i + 1] = data[i];
  modem.beginPacket();
  modem.write(payload, len + 1);
  if (modem.endPacket(true) > 0)
    Serial.print("Message sent correctly (message : ");
  else
    Serial.print("Error sending message (message : ");
  Serial.print("t=");
  Serial.print(type);
  Serial.print("   data=");
  for (int i = 0; i < len; i++)
  {
    Serial.print(data[i]);
    Serial.print(" ");
  }
  Serial.println(")");
}
