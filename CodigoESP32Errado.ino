#include <WiFi.h>
#include <HTTPClient.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RFID
#define SS_PIN 15
#define RST_PIN 2
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
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Estado
String pessoaID = "";
bool esperandoCartao = false;
bool modoDevolucao = false;

// Funções auxiliares
void mostrarMensagem(String linha1, String linha2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linha1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(linha2.substring(0, 16));
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

void sendData(String nome, String valor, String uid) {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    String url = "https://script.google.com/macros/s/" + scriptID + "/exec?nome=" + nome + "&valor=" + valor + "&uid=" + uid;
    Serial.println("Enviando dados: " + url);
    mostrarMensagem("Enviando dados", "...");

    http.begin(url);
    int httpCode = http.GET();
    String payload = http.getString();
    Serial.println("Resposta: " + payload);
    http.end();

    mostrarMensagem("Pronto", "Press A ou B");
  } else {
    Serial.println("Erro: WiFi não conectado");
    mostrarMensagem("WiFi nao", "conectado!");
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  mostrarMensagem("Sistema pronto", "Press A ou B");
}

void loop() {
  if (!esperandoCartao) {
    char key = keypad.getKey();
    if (key) {
      if (key == 'A') {
        pessoaID = "";
        modoDevolucao = false;
        Serial.println("Modo empréstimo. Digite o código da pessoa (8 dígitos):");
        mostrarMensagem("Emprestimo", "Digite ID (8)");
      } else if (key == 'B') {
        modoDevolucao = true;
        esperandoCartao = true;
        pessoaID = "-";
        Serial.println("Modo devolução. Aproxime a calculadora.");
        mostrarMensagem("Devolucao", "Aproxime calc.");
      } else if (!modoDevolucao && isDigit(key)) {
        if (pessoaID.length() < 8) {
          pessoaID += key;
          Serial.print(key);
          mostrarMensagem("ID: " + pessoaID, "");
        }
        if (pessoaID.length() == 8) {
          Serial.println("\nAproxime a calculadora (cartão RFID)...");
          esperandoCartao = true;
          mostrarMensagem("Aproxime", "a calculadora");
        }
      }
    }
} else {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = uidToString(rfid.uid.uidByte, rfid.uid.size);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    Serial.println("UID lido: " + uid);
    mostrarMensagem("UID lido:", uid);
    delay(2000);  // Dá tempo para ver no display

    if (modoDevolucao) {
      Serial.println("Cartão de devolução detectado: " + uid);
      mostrarMensagem("Devolucao OK", "Enviando...");
      sendData("Calculadora-devolvida", "-", uid);
    } else if (pessoaID.length() == 8) {
      String numero = getUserName(uid);
      if (numero != "") {
        Serial.print("Cartão reconhecido: ");
        Serial.println(numero);
        mostrarMensagem("Cartao OK", "Enviando...");
        sendData("Calculadora-emprestada", pessoaID, numero);
      } else {
        Serial.print("Cartão não cadastrado: ");
        Serial.println(uid);
        mostrarMensagem("Cartao nao", "cadastrado");
      }
    }

    pessoaID = "";
    esperandoCartao = false;
    modoDevolucao = false;
    lastUid = uid;
  }
}
}
