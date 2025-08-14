1. Visão Geral
Este projeto é um sistema embarcado para gerenciar o empréstimo e a devolução de calculadoras de forma automatizada. 
Utilizando um microcontrolador ESP32, o sistema se conecta à internet para validar usuários e registrar todas as transações em tempo real 
em planilhas do Google Sheets, que funcionam como um banco de dados e um painel de controle visual.

A interface física para o operador é composta por um display LCD, um teclado numérico e um leitor de cartões RFID para a identificação dos equipamentos.

2. Features Principais
- Múltiplos Modos de Operação: 
- Suporte para Empréstimo 
- Devolução (via RFID) 
- Devolução Forçada (manual) 
- Cadastro de novos cartões.

Validação de Usuário em Nuvem: Conecta-se a uma planilha do Google para verificar se o NUSP (Número USP) do usuário está cadastrado e se não está na lista de banidos.
Controle de Empréstimo Ativo: Realiza uma segunda verificação para impedir que um usuário com um empréstimo ativo pegue uma segunda calculadora, com a opção de o operador autorizar a exceção.
Entrada de Dados Flexível: Permite a digitação de NUSP com tamanho variável, confirmação com a tecla '#' e correção com a tecla 'C' (backspace).
Interface de Usuário Robusta: Fornece feedback claro ao operador através de:
Display LCD: Exibe instruções, status e confirmações em cada etapa.
LEDs de Status: Um LED azul indica o status da conexão WiFi e um LED verde (opcional) indica operações bem-sucedidas.
Buzzer: Emite sons distintos para pressionamento de teclas, leitura de RFID, sucesso e erro.
Reset Global: Uma tecla de atalho (*) permite cancelar qualquer operação a qualquer momento e retornar ao menu inicial.

3. Componentes Utilizados
Hardware
Microcontrolador ESP32
Leitor RFID MFRC522 com cartões/tags
Display LCD I2C 16x2
Teclado Matricial 4x4
Buzzer passivo
LEDs para status visual
Software & Cloud
Arduino IDE com C++
Google Sheets como banco de dados e dashboard
Google Apps Script (JavaScript) para criar as APIs que conectam a ESP32 às planilhas

4. Fluxo de Operação
Inicialização
Ao ser ligado, o sistema exibe "Conectando ao WiFi..." no display e o LED azul pisca.
Após a conexão bem-sucedida, o LED azul fica aceso continuamente e o display mostra a tela inicial, aguardando um comando.
A. Modo Empréstimo (Tecla 'A')
O operador digita o NUSP do usuário. Pode usar 'C' para apagar o último dígito.
Ao terminar, pressiona '#'. O sistema mostra "Verificando..." no display.
A ESP32 faz uma requisição ao primeiro Google Script para validar o NUSP e verificar se o usuário está banido.
Se o usuário for banido ou não for encontrado, uma mensagem de erro é exibida e o sistema volta ao início.
Se o NUSP for válido, a ESP32 faz uma segunda requisição a outro script para verificar se este usuário já possui um empréstimo ativo.
Caso já possua, o sistema avisa e pergunta se o operador deseja continuar ou cancelar.
Se tudo estiver certo (usuário válido e sem empréstimos), o sistema exibe o nome do usuário e pede uma confirmação final ('A' para confirmar, 'B' para cancelar).
Após a confirmação, o display pede para aproximar a calculadora.
Ao ler o cartão RFID, o sistema exibe o número da calculadora, registra o empréstimo na planilha de logs e exibe uma mensagem de sucesso.
O sistema é totalmente resetado e volta para a tela inicial.

B. Modo Devolução (Tecla 'B')
O display pede para aproximar a calculadora.
Ao ler o cartão RFID, o sistema identifica o número da calculadora, registra a devolução na planilha de logs e exibe uma mensagem de sucesso.
O sistema volta para a tela inicial.
D. Modo Devolução Forçada (Tecla 'D')
Usado para calculadoras com RFID defeituoso.
O operador digita manualmente o número da calculadora e confirma com '#'.
O sistema registra a devolução na planilha e exibe uma mensagem de sucesso.
O sistema volta para a tela inicial.

5. Estrutura do Código
O projeto é dividido em duas partes principais:
Código Embarcado (ESP32):
Responsável por gerenciar todo o hardware (LCD, Teclado, RFID).
Implementa a máquina de estados que guia o usuário através dos diferentes fluxos.
Conecta-se ao WiFi e realiza chamadas HTTP GET para os Google Apps Scripts.
Google Apps Script (Backend na Nuvem):
Script de Verificação de NUSP: Anexado à planilha de cadastros. Recebe um NUSP, procura na planilha e retorna um dos três status: OK:, BANIDO: ou ERRO:.
Script de Verificação de Empréstimo Ativo: Anexado à planilha de status. Recebe um NUSP e retorna se o status de empréstimo é DISPONIVEL ou EMPRESTADO.
Script de Registro (Log): Recebe os dados do empréstimo/devolução (pessoaID, numero da calculadora) 
e os insere em uma nova linha na planilha de logs, junto com a data e a hora da transação.
