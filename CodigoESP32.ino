#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>

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

// Teclado 4x4
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {5, 4, 0, 2};  // Ajuste conforme sua conexão
byte colPins[COLS] = {14, 12, 13, 15}; // Ajuste conforme sua conexão
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String userCode = "";
bool esperandoCartao = false;

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

void loop() {
  char key = keypad.getKey();

  if (!esperandoCartao) {
    if (key == 'A') {
      Serial.println("Início do empréstimo. Digite o código do aluno:");
      userCode = "";
    } else if (key && isDigit(key)) {
      userCode += key;
      Serial.print("*"); // Feedback
      if (userCode.length() == 8) {
        Serial.println("\nCódigo recebido. Aproxime a calculadora.");
        esperandoCartao = true;
      }
    }
  } else {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String rfidCode = uidToString(rfid.uid.uidByte, rfid.uid.size);
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();

      Serial.println("Cartão lido:");
      Serial.println(rfidCode);
      sendData("Calculadora-emprestada", userCode + "-" + rfidCode);

      esperandoCartao = false;
    }
  }
}
