#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 21      // SDA do RFID
#define RST_PIN 22     // RST do RFID
// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);




#define SS_PIN 15
#define RST_PIN 5    // RST do RFID
#define LED_PIN 2
#define BUZZER_PIN 4

@@ -37,6 +45,7 @@ Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
String pessoaID = "";
bool esperandoCartao = false;
bool processoIniciado = false;
bool modoDevolucao = false;

void setup() {
  Serial.begin(115200);
@@ -56,23 +65,46 @@ void setup() {
  Serial.println("\nWi-Fi conectado.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema pronto");
  lcd.setCursor(0, 1);
  lcd.print("Pressione A ou B");


}
void mostrarMensagem(String linha1, String linha2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linha1.substring(0, 16)); // Linha 1
  lcd.setCursor(0, 1);
  lcd.print(linha2.substring(0, 16)); // Linha 2
}


void loop() {
  char key = keypad.getKey();

  // Esperando apertar 'A' para iniciar o processo
  if (!processoIniciado) {
  // Espera iniciar processo de empréstimo ou devolução
  if (!processoIniciado && !modoDevolucao) {
    if (key == 'A') {
      processoIniciado = true;
      pessoaID = "";
      Serial.println("Início do processo. Digite o código da pessoa (8 dígitos):");
      Serial.println("Início do empréstimo. Digite o código da pessoa (8 dígitos):");
      mostrarMensagem("Emprestimo", "Digite ID (8)");
    } else if (key == 'B') {
      modoDevolucao = true;
      Serial.println("Modo devolução. Aproxime a calculadora (cartão RFID)...");
      mostrarMensagem("Devolucao", "Aproxime calc.");
    }
    return; // Não faz mais nada até que 'A' seja pressionado
    return;
  }

  // Coletando os 8 dígitos
  if (!esperandoCartao && key && isDigit(key)) {
  // Processo de empréstimo
  if (processoIniciado && !esperandoCartao && key && isDigit(key)) {
    pessoaID += key;
    Serial.print(key);

@@ -82,37 +114,70 @@ void loop() {

    if (pessoaID.length() >= 8) {
      Serial.println("\nAproxime a calculadora (cartão RFID)...");
      mostrarMensagem("Emprestimo", "Aproxime calc.");
      esperandoCartao = true;
    }
  }

  // Esperando cartão RFID
  if (esperandoCartao) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String uid = uidToString(rfid.uid.uidByte, rfid.uid.size);
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
  // Leitura de RFID
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = uidToString(rfid.uid.uidByte, rfid.uid.size);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    String numero = getUserName(uid);

    if (modoDevolucao) {
      if (numero != "") {
        Serial.print("Calculadora devolvida: ");
        Serial.println(numero);
        mostrarMensagem("Devolucao OK", "Enviando...");
        sendData("Calculadora-devolvida", "-", numero);
      } else {
        Serial.print("Cartão não cadastrado: ");
        Serial.println(uid);
        mostrarMensagem("Cartao nao", "cadastrado");
        delay(2000);
      }

      // Resetar estados
      modoDevolucao = false;
      processoIniciado = false;
      esperandoCartao = false;
      pessoaID = "";

      String numero = getUserName(uid);
      mostrarMensagem("Sistema pronto", "Pressione A ou B");
    }

    else if (processoIniciado && esperandoCartao) {
      if (numero != "") {
        Serial.print("Cartão reconhecido: ");
        Serial.println(numero);
        mostrarMensagem("Emprestimo OK", "Enviando...");
        sendData("Calculadora-emprestada", pessoaID, numero);
      } else {
        Serial.print("Cartão não cadastrado: ");
        Serial.println(uid);
        mostrarMensagem("Cartao nao", "cadastrado");
        delay(2000);
      }

      // Resetando tudo para próxima vez
      pessoaID = "";
      esperandoCartao = false;
      // Resetar estados
      processoIniciado = false;
      lastUid = uid;
      esperandoCartao = false;
      modoDevolucao = false;
      pessoaID = "";

      mostrarMensagem("Sistema pronto", "Pressione A ou B");
    }


    lastUid = uid;
  }
}



String uidToString(byte *buffer, byte bufferSize) {
  String result = "";
  for (byte i = 0; i < bufferSize; i++) {
@@ -158,3 +223,6 @@ void sendData(String nome, String valor, String udi) {
    Serial.println("Erro: WiFi não conectado");
  }
}
