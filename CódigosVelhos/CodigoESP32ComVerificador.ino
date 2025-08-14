#include <WiFi.h>
#include <HTTPClient.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClientSecure.h>

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);




#define SS_PIN 15
#define RST_PIN 5    // RST do RFID
#define LED_PIN 2
#define BUZZER_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);

// Wi-Fi
const char* ssid = "ABCDEF";
const char* password = "12345678";

// Google Apps Script
const String scriptID = "AKfycby8qK4YqNirCeueu0CnYG41PLmn3DVVXBtGx08rtSmF-u8vMlRVrJTSBwbK3Pd3Eg";

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
bool modoDevolucao = false;
bool aguardandoConfirmacao = false;

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

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema pronto");
  lcd.setCursor(0, 1);
  lcd.print("Pressione A ou B");


}

bool verificarNUSP(String nusp, String &primeiroNome) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = "https://script.google.com/macros/s/AKfycby237q04YCrFc-cQV2GEgKTUEHyAQco-QoUPicmpRjSfkpgqNnWHa5mQeiDDTENJR0UHQ/exec?nusp=" + nusp;
  
  http.begin(client, url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  int httpCode = http.GET();
  
  Serial.print("HTTP Code final: ");
  Serial.println(httpCode);

  String payload = "";
  if (httpCode == 200) {
    WiFiClient* stream = http.getStreamPtr();
    while (stream->available()) {
      payload += (char)stream->read();
    }
  }

  Serial.println("Payload final: " + payload);
  http.end();

  // --- MUDANÇA FINAL E DEFINITIVA ---

  // Procura por "OK:" em QUALQUER LUGAR do payload.
  // A função indexOf retorna -1 se não encontrar.
  int pos = payload.indexOf("OK:");

  if (httpCode == 200 && pos != -1) {
    // Se encontrou "OK:", o NUSP é válido!
    
    // Pega o texto que vem depois de "OK:"
    String temp = payload.substring(pos + 3);
    
    // Agora, vamos limpar o lixo do final (como o "0")
    // Procuramos pela primeira quebra de linha ou retorno de carro
    int FimDoNome = -1;
    for(int i=0; i < temp.length(); i++){
      if(temp.charAt(i) == '\n' || temp.charAt(i) == '\r'){
        FimDoNome = i;
        break;
      }
    }

    if (FimDoNome != -1) {
      // Se achou uma quebra de linha, pega só o texto até ali
      primeiroNome = temp.substring(0, FimDoNome);
    } else {
      // Senão, usa o texto todo (por segurança)
      primeiroNome = temp;
    }
    
    primeiroNome.trim(); // Garante que não há espaços em branco
    return true;

  } else {
    // Se não encontrou "OK:", o NUSP é inválido
    return false;
  }
}









void mostrarMensagem(String linha1, String linha2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linha1.substring(0, 16)); // Linha 1
  lcd.setCursor(0, 1);
  lcd.print(linha2.substring(0, 16)); // Linha 2
}


// Substitua sua função loop() inteira por esta versão atualizada:

// Substitua sua função loop() por esta versão:

// Substitua sua função loop() por esta nova versão:

// Substitua sua função loop() por esta nova versão:

// Substitua sua função loop() inteira por esta versão final e polida:

// Substitua sua função loop() por esta versão com o fluxo corrigido:

// Substitua sua função loop() por esta versão, que re-inicializa o hardware do RFID.

// Substitua sua função loop() por esta versão com as novas mensagens no display:

void loop() {
  char key = keypad.getKey();

  // --- PARTE 1: Menu inicial (Não muda) ---
  if (!processoIniciado && !modoDevolucao) {
    if (key == 'A' || key == 'B') {
      digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
      if (key == 'A') {
        processoIniciado = true;
        pessoaID = "";
        mostrarMensagem("Digite o NUSP", "e pressione #");
      } else { // key == 'B'
        modoDevolucao = true;
        mostrarMensagem("Devolucao", "Aproxime calc.");
      }
    }
    return;
  }

  // --- PARTE 2: Processo de empréstimo (Não muda) ---
  if (processoIniciado) {
    if (!esperandoCartao && !aguardandoConfirmacao && key) {
      if (isDigit(key) && pessoaID.length() < 10) {
        pessoaID += key;
        digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
        mostrarMensagem("NUSP:", pessoaID);
      } 
      else if (key == 'C' && pessoaID.length() > 0) {
        pessoaID.remove(pessoaID.length() - 1);
        digitalWrite(BUZZER_PIN, HIGH); delay(50); digitalWrite(BUZZER_PIN, LOW);
        mostrarMensagem("NUSP:", pessoaID);
      }
      else if (key == '#' && pessoaID.length() > 0) {
        digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW); delay(60);
        digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW);
        mostrarMensagem("Verificando...", "Aguarde");
        String nomeVerificado;
        if (verificarNUSP(pessoaID, nomeVerificado)) {
          aguardandoConfirmacao = true;
          mostrarMensagem("Emprestar para:", nomeVerificado);
          delay(2500);
          mostrarMensagem("A: Confirma", "B: Cancela");
        } else {
          mostrarMensagem("NUSP nao", "cadastrado");
          digitalWrite(BUZZER_PIN, HIGH); delay(500); digitalWrite(BUZZER_PIN, LOW);
          delay(2000);
          processoIniciado = false;
          pessoaID = "";
          mostrarMensagem("Sistema pronto", "Pressione A ou B");
        }
      }
      return;
    }

    if (aguardandoConfirmacao && key) {
      digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
      if (key == 'A') {
        aguardandoConfirmacao = false;
        esperandoCartao = true;
        mostrarMensagem("Confirmado!", "Aproxime calc.");
      } else if (key == 'B') {
        mostrarMensagem("Operacao", "Cancelada");
        delay(2000);
        processoIniciado = false;
        aguardandoConfirmacao = false;
        pessoaID = "";
        mostrarMensagem("Sistema pronto", "Pressione A ou B");
      }
      return;
    }
  }

  // --- PARTE 3: Leitura do RFID (COM AS MENSAGENS ATUALIZADAS) ---
  if ((modoDevolucao || esperandoCartao) && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = uidToString(rfid.uid.uidByte, rfid.uid.size);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    String numero = getUserName(uid); // O número da calculadora está aqui

    // Lógica de Devolução
    if (modoDevolucao) {
      if (numero != "") {
        // --- NOVA MENSAGEM ---
        mostrarMensagem("Devolvendo Calc:", numero);
        delay(1500); // Pausa para o operador ver o número
        
        mostrarMensagem("Registrando...", "Aguarde");
        if (sendData("Calculadora-devolvida", "-", numero)) {
          mostrarMensagem("Devolucao", "Concluida!");
          digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW); delay(60);
          digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW);
          delay(2000);
        } else {
          // ... (código de erro)
        }
      } else {
        // ... (código de erro)
      }
    }
    // Lógica de Empréstimo
    else if (processoIniciado && esperandoCartao) {
      if (numero != "") {
        // --- NOVA MENSAGEM ---
        mostrarMensagem("Emprestando Calc:", numero);
        delay(1500); // Pausa para o operador ver o número

        mostrarMensagem("Registrando...", "Aguarde");
        if (sendData("Calculadora-emprestada", pessoaID, numero)) {
          mostrarMensagem("Emprestimo", "Concluido!");
          digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW); delay(60);
          digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW);
          delay(2000);
        } else {
          // ... (código de erro)
        }
      } else {
        // ... (código de erro)
      }
    }

    // Reset total dos estados (não muda)
    modoDevolucao = false;
    processoIniciado = false;
    esperandoCartao = false;
    aguardandoConfirmacao = false;
    pessoaID = "";
    mostrarMensagem("Sistema pronto", "Pressione A ou B");
    
    rfid.PCD_Init();
    
    lastUid = uid;
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
  else if (uid == "04-30-6C-52-DA-61-80") return "24";
  else return "";
}













// Substitua sua função sendData por esta versão mais robusta:

bool sendData(String nome, String valor, String udi) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Erro: WiFi não conectado para envio de dados.");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  // Usa o scriptID principal (de registro), que você definiu no início do código
  String url = "https://script.google.com/macros/s/" + scriptID + "/exec?nome=" + nome + "&valor=" + valor + "&uid=" + udi;
  
  http.begin(client, url);
  // Opcional, mas bom ter: seguir redirecionamentos se houver
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  int httpCode = http.GET();
  String payload = http.getString();
  
  Serial.println("Enviando dados: " + url);
  Serial.println("Resposta do servidor de registro: " + payload);
  Serial.println("HTTP Code do registro: " + httpCode);
  
  http.end();

  // Consideramos sucesso se o código HTTP for 200 OK
  if (httpCode == 200) {
    return true;
  } else {
    return false;
  }
}
