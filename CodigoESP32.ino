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

String lastUid = "";


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
bool processoIniciado = false;

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
  char key = keypad.getKey();

  // Esperando apertar 'A' para iniciar o processo
  if (!processoIniciado) {
    if (key == 'A') {
      processoIniciado = true;
      pessoaID = "";
      Serial.println("Início do processo. Digite o código da pessoa (8 dígitos):");
    }
    return; // Não faz mais nada até que 'A' seja pressionado
  }

  // Coletando os 8 dígitos
  if (!esperandoCartao && key && isDigit(key)) {
    pessoaID += key;
    Serial.print(key);

    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);

    if (pessoaID.length() >= 8) {
      Serial.println("\nAproxime a calculadora (cartão RFID)...");
      esperandoCartao = true;
    }
  }

  // Esperando cartão RFID
  if (esperandoCartao) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String uid = uidToString(rfid.uid.uidByte, rfid.uid.size);
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();

      String numero = getUserName(uid);
      if (numero != "") {
        Serial.print("Cartão reconhecido: ");
        Serial.println(numero);
        sendData("Calculadora-emprestada", pessoaID, numero);
      } else {
        Serial.print("Cartão não cadastrado: ");
        Serial.println(uid);
      }

      // Resetando tudo para próxima vez
      pessoaID = "";
      esperandoCartao = false;
      processoIniciado = false;
      lastUid = uid;
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


String getUserName(String uid) {
  if (uid == "04-28-6C-52-DA-61-80") return "23";
  else if (uid == "04-2C-6C-52-DA-61-80") return "26";
  else if (uid == "04-AF-68-52-DA-61-80") return "3";
  else if (uid == "04-B2-67-52-DA-61-80") return "20";
  else if (uid == "04-D2-66-52-DA-61-80") return "21";
  else return "";
}



void sendData(String nome, String valor, String udi) {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    String url = "https://script.google.com/macros/s/" + scriptID + "/exec?nome=" + nome + "&valor=" + valor + "&uid=" + udi;

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
