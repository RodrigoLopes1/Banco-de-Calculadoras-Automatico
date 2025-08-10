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
bool modoCadastroRFID = false; 
bool aguardandoConfirmacaoEmprestimoDuplo = false;

void setup() {
  Serial.begin(115200);
  SPI.begin(); 
  rfid.PCD_Init(); 

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  // A linha 'digitalWrite(LED_PIN, HIGH);' foi movida para depois da conexão.

  // --- INÍCIO DA CORREÇÃO ---
  // 1. Inicializa o display PRIMEIRO
  lcd.init();
  lcd.backlight();
  
  // 2. Mostra a mensagem de "Conectando..." no display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando ao");
  lcd.setCursor(0, 1);
  lcd.print("WiFi");
  // --- FIM DA CORREÇÃO ---

  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  
  // Enquanto espera, o LED azul (LED_PIN) pisca
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(LED_PIN, HIGH);
    delay(250);
    digitalWrite(LED_PIN, LOW);
    delay(250);
  }
  
  Serial.println("\nWi-Fi conectado.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Acende o LED azul permanentemente para indicar que está pronto
  digitalWrite(LED_PIN, HIGH);

  // Mostra a mensagem final de sistema pronto
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema pronto");
  lcd.setCursor(0, 1);
  lcd.print("Pressione A ou B");
}

int verificarNUSP(String nusp, String &primeiroNome) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado");
    return 0; // ERRO
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = "https://script.google.com/macros/s/AKfycbz9WPSQ2U5OE6Vnzwv7RvpDB0svIaownyCjN4iLb0s5l1-3CfxMv3O0dVyScEsRHWYanw/exec?nusp=" + nusp;
  
  http.begin(client, url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  int httpCode = http.GET();
  
  String payload = "";
  if (httpCode == 200) {
    WiFiClient* stream = http.getStreamPtr();
    while (stream->available()) {
      payload += (char)stream->read();
    }
  }
  http.end();

  if (httpCode == 200) {
    // --- NOVA LÓGICA DE VERIFICAÇÃO ---
    int posBanido = payload.indexOf("BANIDO:");
    int posOK = payload.indexOf("OK:");

    if (posBanido != -1) {
      // Se encontrou "BANIDO:", extrai o nome e retorna 2
      String temp = payload.substring(posBanido + 7);
      // (Lógica para limpar o nome, igual à anterior)
      int FimDoNome = -1;
      for(int i=0; i < temp.length(); i++){ if(temp.charAt(i) == '\n' || temp.charAt(i) == '\r'){ FimDoNome = i; break; } }
      if (FimDoNome != -1) { primeiroNome = temp.substring(0, FimDoNome); } else { primeiroNome = temp; }
      primeiroNome.trim();
      return 2; // STATUS BANIDO

    } else if (posOK != -1) {
      // Se encontrou "OK:", extrai o nome e retorna 1
      String temp = payload.substring(posOK + 3);
      // (Lógica para limpar o nome, igual à anterior)
      int FimDoNome = -1;
      for(int i=0; i < temp.length(); i++){ if(temp.charAt(i) == '\n' || temp.charAt(i) == '\r'){ FimDoNome = i; break; } }
      if (FimDoNome != -1) { primeiroNome = temp.substring(0, FimDoNome); } else { primeiroNome = temp; }
      primeiroNome.trim();
      return 1; // STATUS OK
    }
  }

  // Se nada foi encontrado ou houve erro de HTTP
  return 0; // STATUS ERRO
}


String getUserName(String uid) {
  if (uid == "04-28-6C-52-DA-61-80") return "23";
  else if (uid == "04-2C-6C-52-DA-61-80") return "26";
  //else if (uid == "04-AF-68-52-DA-61-80") return "3"; 
  else if (uid == "04-B2-67-52-DA-61-80") return "20";
  else if (uid == "04-D2-66-52-DA-61-80") return "21";
  else if (uid == "04-30-6C-52-DA-61-80") return "24";
  else if (uid == "04-51-6C-52-DA-61-80") return "3";
  else if (uid == "06-7D-6C-FA") return "1";
  else return "";
}

// NOVA FUNÇÃO para verificar se um NUSP já tem um empréstimo ativo
String verificarEmprestimoAtivo(String nusp) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi não conectado para checagem de empréstimo");
    return "ERRO_WIFI";
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  // IMPORTANTE: Use o link do seu SEGUNDO script aqui!
  String url = "https://script.google.com/macros/s/AKfycbyxzbaYZUv-z2bGPP_TaaesGBAbyNrtJOt7bFS4OkylubIreRHLameWUG9ZNUyrCC8hwg/exec?nusp=" + nusp;
  
  http.begin(client, url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  int httpCode = http.GET();
  String payload = "";
  if (httpCode > 0) {
    payload = http.getString();
  }
  
  Serial.println("Verificando emprestimo ativo para NUSP: " + nusp);
  Serial.println("URL da verificação: " + url);
  Serial.println("HTTP Code da verificação: " + httpCode);
  Serial.println("Payload da verificação: " + payload);
  
  http.end();
  
  return payload;
}






void mostrarMensagem(String linha1, String linha2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linha1.substring(0, 16)); // Linha 1
  lcd.setCursor(0, 1);
  lcd.print(linha2.substring(0, 16)); // Linha 2
}


// Substitua sua função loop() por esta versão corrigida e mais estável:

void loop() {
  char key = keypad.getKey();

  // ESTADO 1: OCIOSO (Aguardando 'A', 'B' ou 'D')
  if (!processoIniciado && !modoDevolucao && !modoCadastroRFID) {
    if (key == 'A') {
      digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
      processoIniciado = true;
      pessoaID = "";
      mostrarMensagem("Digite o NUSP", "e pressione #");
    } else if (key == 'B') {
      digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
      modoDevolucao = true;
      esperandoCartao = true; 
      mostrarMensagem("Devolucao", "Aproxime calc.");
    } else if (key == 'D') {
      digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
      modoCadastroRFID = true;
      esperandoCartao = true; 
      mostrarMensagem("Passe a", "calculadora");
    }
  }

  // ESTADO 2: PROCESSO DE EMPRÉSTIMO ATIVO
  else if (processoIniciado) {
    
    // SUB-ESTADO 2.1: AGUARDANDO CONFIRMAÇÃO DO EMPRÉSTIMO DUPLO
    if (aguardandoConfirmacaoEmprestimoDuplo) {
      if (key) {
        digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
        if (key == 'A') {
          aguardandoConfirmacaoEmprestimoDuplo = false;
          esperandoCartao = true;
          mostrarMensagem("Confirmado!", "Aproxime calc.");
        } else if (key == 'B') {
          mostrarMensagem("Operacao", "Cancelada");
          delay(2000);
          // Reseta tudo para o estado ocioso
          processoIniciado = false;
          aguardandoConfirmacaoEmprestimoDuplo = false;
          pessoaID = "";
          mostrarMensagem("Sistema pronto", "Pressione A ou B");
        }
      }
    }

    // SUB-ESTADO 2.2: AGUARDANDO CONFIRMAÇÃO DO EMPRÉSTIMO NORMAL
    else if (aguardandoConfirmacao) {
      if (key) {
        digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
        if (key == 'A') {
          aguardandoConfirmacao = false;
          esperandoCartao = true;
          mostrarMensagem("Confirmado!", "Aproxime calc.");
        } else if (key == 'B') {
          mostrarMensagem("Operacao", "Cancelada");
          delay(2000);
          // Reseta tudo para o estado ocioso
          processoIniciado = false;
          aguardandoConfirmacao = false;
          pessoaID = "";
          mostrarMensagem("Sistema pronto", "Pressione A ou B");
        }
      }
    }

    // SUB-ESTADO 2.3: AGUARDANDO DIGITAÇÃO DO NUSP
    else if (!esperandoCartao) {
        if (key) { 
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
              
              mostrarMensagem("Verificando NUSP", "Aguarde...");
              String nomeVerificado;
              int statusNUSP = verificarNUSP(pessoaID, nomeVerificado);

              switch (statusNUSP) {
                case 1: { // CASO OK
                  mostrarMensagem("Verificando se", "ha emprestimo...");
                  String statusEmprestimo = verificarEmprestimoAtivo(pessoaID);
                  if (statusEmprestimo.indexOf("DISPONIVEL") != -1) {
                    aguardandoConfirmacao = true;
                    mostrarMensagem("Emprestar para:", nomeVerificado);
                    delay(2500);
                    mostrarMensagem("A: Confirma", "B: Cancela");
                  } 
                  else if (statusEmprestimo.indexOf("EMPRESTADO") != -1) {
                    mostrarMensagem("NUSP COM", "EMPRESTIMO");
                    digitalWrite(BUZZER_PIN, HIGH); delay(600); digitalWrite(BUZZER_PIN, LOW);
                    delay(2000);
                    aguardandoConfirmacaoEmprestimoDuplo = true;
                    mostrarMensagem("A: Continua", "B: Cancela");
                  } else {
                    mostrarMensagem("Erro ao checar", "emprestimos.");
                    delay(2000);
                    processoIniciado = false;
                    pessoaID = "";
                    mostrarMensagem("Sistema pronto", "Pressione A ou B");
                  }
                  break;
                }
                case 2: // CASO BANIDO
                  mostrarMensagem("USUARIO BANIDO", nomeVerificado);
                  digitalWrite(BUZZER_PIN, HIGH); delay(1000); digitalWrite(BUZZER_PIN, LOW);
                  delay(2500);
                  processoIniciado = false;
                  pessoaID = "";
                  mostrarMensagem("Sistema pronto", "Pressione A ou B");
                  break;
                case 0: // CASO ERRO/NÃO ENCONTRADO
                default:
                  mostrarMensagem("NUSP nao", "cadastrado");
                  digitalWrite(BUZZER_PIN, HIGH); delay(500); digitalWrite(BUZZER_PIN, LOW);
                  delay(2000);
                  processoIniciado = false;
                  pessoaID = "";
                  mostrarMensagem("Sistema pronto", "Pressione A ou B");
                  break;
              }
            }
        }
    }
  }

  // ESTADO 3: ESPERANDO PELO CARTÃO RFID
  else if (esperandoCartao && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Toca um bipe no momento exato em que o cartão é lido com sucesso.
    digitalWrite(BUZZER_PIN, HIGH); delay(150); digitalWrite(BUZZER_PIN, LOW);

    String uid = uidToString(rfid.uid.uidByte, rfid.uid.size);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    String numero = getUserName(uid);

    if (modoCadastroRFID) {
      Serial.println("--- MODO CADASTRO RFID ---");
      Serial.println("UID Lido: " + uid);
      Serial.println("--------------------------");
      mostrarMensagem("RFID Lido!", "Verifique Serial");
      delay(2500);
    } 
    else if (modoDevolucao) {
      if (numero != "") {
        mostrarMensagem("Devolvendo Calc:", numero);
        delay(1500);
        mostrarMensagem("Registrando...", "Aguarde");
        if (sendData("Calculadora-devolvida", "-", numero)) {
          mostrarMensagem("Devolucao", "Concluida!");
          digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW); delay(60);
          digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW);
          delay(2000);
        } else {
          mostrarMensagem("Erro de Rede", "Tente novamente");
          digitalWrite(BUZZER_PIN, HIGH); delay(500); digitalWrite(BUZZER_PIN, LOW);
          delay(2000);
        }
      } else {
        mostrarMensagem("Cartao nao", "cadastrado");
        digitalWrite(BUZZER_PIN, HIGH); delay(500); digitalWrite(BUZZER_PIN, LOW);
        delay(2000);
      }
    }
    else if (processoIniciado) {
      if (numero != "") {
        mostrarMensagem("Emprestando Calc:", numero);
        delay(1500);
        mostrarMensagem("Registrando...", "Aguarde");
        if (sendData("Calculadora-emprestada", pessoaID, numero)) {
          mostrarMensagem("Emprestimo", "Concluido!");
          digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW); delay(60);
          digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW);
          delay(2000);
        } else {
          mostrarMensagem("Erro de Rede", "Tente novamente");
          digitalWrite(BUZZER_PIN, HIGH); delay(500); digitalWrite(BUZZER_PIN, LOW);
          delay(2000);
        }
      } else {
        mostrarMensagem("Cartao nao", "cadastrado");
        digitalWrite(BUZZER_PIN, HIGH); delay(500); digitalWrite(BUZZER_PIN, LOW);
        delay(2000);
      }
    }

    // --- RESET TOTAL AO FINAL DE QUALQUER OPERAÇÃO RFID ---
    processoIniciado = false;
    modoDevolucao = false;
    esperandoCartao = false;
    aguardandoConfirmacao = false;
    aguardandoConfirmacaoEmprestimoDuplo = false;
    modoCadastroRFID = false;
    pessoaID = "";
    mostrarMensagem("Sistema pronto", "Pressione A ou B");
    rfid.PCD_Init();
    lastUid = uid;
  }
}

String uidToString(byte *buffer, byte bufferSize) {
  String result = "";
  for (byte i = 0; i < bufferSize; i++) {
    if (i > 0) result += " "; // Use espaço ou "-" como separador
    if (buffer[i] < 0x10) result += "0";
    result += String(buffer[i], HEX);
  }
  result.toUpperCase();
  return result;
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
