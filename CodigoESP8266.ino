#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 15
#define RST_PIN 16
#define LED_PIN 2
#define BUZZER 4

MFRC522 rfid(SS_PIN, RST_PIN);
WiFiClientSecure client;

const char* ssid = "ABCDEF";
const char* password = "12345678";
const char* host = "script.google.com";
const int httpsPort = 443;

String appscriptID = "AKfycbw6yPzpHzBGF79_2UzTs-7BHshAJwY94ejbvaRMp_kfpaJr7agKWIdLdwdd9mxnMLk";

String lastUid = "";

// === WiFi Setup ===
void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.println("Conectando-se ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, LOW);
    delay(250);
    digitalWrite(LED_PIN, HIGH);
    delay(250);
    Serial.print(".");
  }

  digitalWrite(LED_PIN, HIGH);
  Serial.println("\nConectado ao WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  client.setInsecure();
}

// === Converte UID para string formatada tipo "AA-BB-CC-DD"
String uidToString(byte *buffer, byte bufferSize) {
  String result = "";
  for (byte i = 0; i < bufferSize; i++) {
    if (i > 0) result += "-";
    if (buffer[i] < 0x10) result += "0";
    result += String(buffer[i], HEX);
  }
  result.toUpperCase();
  return result;
}

// === Mapeia UID para nome
String getUserName(String uid) {
  if (uid == "04-28-6C-52-DA-61-80") return "23";
  else if (uid == "04-2C-6C-52-DA-61-80") return "26";
  else if (uid == "04-AF-68-52-DA-61-80") return "3";
  else if (uid == "04-B2-67-52-DA-61-80") return "20";
  else if (uid == "04-D2-66-52-DA-61-80") return "21";
  else return "";
}

// === Envia dados para o Google Sheets
void sendData(String nome, String valor) {
  Serial.println("======================================================");
  Serial.print("Conectando a: ");
  Serial.println(host);
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(BUZZER, LOW);

  if (!client.connect(host, httpsPort)) {
    Serial.println("Falha na conexão");
    return;
  }

  String url = "/macros/s/" + appscriptID + "/exec?nome=" + nome + "&valor=" + valor;
  Serial.print("Enviando URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  String line = client.readStringUntil('\n');
  Serial.println("Resposta:");
  Serial.println(line);
  Serial.println("==========");
}

// === Loop principal
void loop() {
  // Verifica se há cartão presente
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = uidToString(rfid.uid.uidByte, rfid.uid.size);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    if (uid != lastUid) {
      String nome = getUserName(uid);
      if (nome != "") {
        Serial.print("Cartão reconhecido: ");
        Serial.println(nome);
        sendData("Calculadora-emprestada", nome);
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(BUZZER, LOW);
      } else {
        Serial.print("Cartão não cadastrado: ");
        Serial.println(uid);
      }

      lastUid = uid;
    }
  }
}
