#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>

// --- RFID ---
#define SS_PIN 5
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

// --- Teclado 4x4 ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26};  // L1-L4
byte colPins[COLS] = {27, 14, 12, 13};  // C1-C4
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- WiFi ---
const char* ssid = "ABCDEF";
const char* password = "12345678";

// --- App Script ---
const String appscriptID = "AKfycbw6yPzpHzBGF79_2UzTs-7BHshAJwY94ejbvaRMp_kfpaJr7agKWIdLdwdd9mxnMLk";
const String host = "https://script.google.com/macros/s/" + appscriptID + "/exec";

// --- LED e Buzzer ---
#define LED_PIN 2
#define BUZZER 4

// --- Variáveis de controle ---
String codigoPessoa = "";
bool aguardandoCodigo = false;
bool aguardandoCartao = false;

// --- Setup ---
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  SPI.begin();  // SCK=18, MISO=19, MOSI=23
  rfid.PCD_Init();

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
}

// --- UID formatado ---
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

// --- Envio para planilha ---
void sendData(String nome, String valor) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = host + "?nome=" + nome + "&valor=" + valor;
    http.begin(url);
    int httpCode = http.GET();
    Serial.print("Resposta HTTP: ");
    Serial.println(httpCode);
    http.end();

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER, LOW);
  } else {
    Serial.println("WiFi desconectado!");
  }
}

// --- Loop principal ---
void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.print("Tecla pressionada: ");
    Serial.println(key);
    if (key == 'A') {
      codigoPessoa = "";
      aguardandoCodigo = true;
      aguardandoCartao = false;
      Serial.println("Digite o código da pessoa (8 dígitos):");
    } else if (aguardandoCodigo) {
      if (key >= '0' && key <= '9') {
        codigoPessoa += key;
        Serial.println("Código parcial: " + codigoPessoa);
        if (codigoPessoa.length() == 8) {
          Serial.println("Código completo inserido: " + codigoPessoa);
          aguardandoCodigo = false;
          aguardandoCartao = true;
          Serial.println("Aproxime a calculadora (cartão RFID)...");
        }
      } else {
        Serial.println("Somente números são válidos.");
      }
    }
  }

  // Leitura RFID
  if (aguardandoCartao && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = uidToString(rfid.uid.uidByte, rfid.uid.size);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    Serial.println("Cartão lido: " + uid);
    sendData("Calculadora-emprestada", codigoPessoa + "," + uid);
    aguardandoCartao = false;
  }
}
