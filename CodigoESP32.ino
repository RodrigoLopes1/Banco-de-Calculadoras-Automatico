#include <WiFi.h>
#include <HTTPClient.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21      // SDA do RFID
#define RST_PIN 22     // RST do RFID
#define LED_PIN 2
#define BUZZER_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);

// Wi-Fi
const char* ssid = "ABCDEF";
const char* password = "12345678";

// Google Apps Script
const String scriptID = "AKfycbw6yPzpHzBGF79_2UzTs-7BHshAJwY94ejbvaRMp_kfpaJr7agKWIdLdwdd9mxnMLk";

// Keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27}; // Ajuste os pinos conforme necessário
byte colPins[COLS] = {26, 25, 33, 32}; // Ajuste os pinos conforme necessário
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String pessoaID = "";
bool esperandoCartao = false;

void setup() {
  Serial.begin(115200);
  SPI.begin(); 
  rfid.PCD_Init(); 

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWi-Fi conectado.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (!esperandoCartao) {
    char key = keypad.getKey();
    if (key) {
      if (key == 'A') {
        pessoaID = "";
        Serial.println("Digite o código da pessoa (8 dígitos):");
      } else if (isDigit(key)) {
        pessoaID += key;
        Serial.print(key);
        if (pessoaID.length() >= 8) {
          Serial.println("\nAproxime a calculadora (cartão RFID)...");
          esperandoCartao = true;
        }
      }
    }
  } else {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String uid = uidToString(rfid.uid.uidByte, rfid.uid.size);
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();

      Serial.print("Cartão lido: ");
      Serial.println(uid);
      sendData("Calculadora-emprestada", pessoaID + "," + uid);

      pessoaID = "";
      esperandoCartao = false;
    }
  }
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
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    String url = "https://script.google.com/macros/s/" + scriptID + "/exec?nome=" + nome + "&valor=" + valor;
    Serial.println("Enviando dados: " + url);
    
    http.begin(url);
    int httpCode = http.GET();
    String payload = http.getString();
    Serial.println("Resposta: " + payload);
    http.end();

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    Serial.println("Erro: WiFi não conectado");
  }
}
