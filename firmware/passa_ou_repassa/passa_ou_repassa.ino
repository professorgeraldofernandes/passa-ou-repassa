/*
  Projeto: Passa ou Repassa
  Arquivo: firmware/passa_ou_repassa/passa_ou_repassa.ino
  Versão: 0.3.0
  Plataforma: Arduino Mega 2560
  Linguagem: C++ / Arduino
  Display: LCD 20x4 I2C

  Descrição:
  Controle eletrônico do jogo Passa ou Repassa com duas equipes,
  travamento da primeira equipe que pressionar, sinalização por LEDs,
  buzzer, reset de rodada, pontuação manual e placar no display LCD 20x4.

  Alterações da versão 0.3.0:
  - Chave de pontuação Azul movida para D28.
  - Chave de pontuação Verde movida para D25.
  - Equipe Vermelha renomeada para Verde.
  - Adicionado bloqueio de pontuação para evitar múltiplos pontos em um único clique.
  - Buzzer no D8 com sons de inicialização, rodada, pontuação, erro e reset.

  Autor: Geraldo Fernandes
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>

// =====================================================
// CONFIGURAÇÕES DO LCD
// =====================================================

// Endereços comuns: 0x27 ou 0x3F.
// Caso o display acenda, mas não mostre caracteres, teste trocar para 0x3F.
constexpr uint8_t LCD_ADDR = 0x27;
constexpr uint8_t LCD_COLS = 20;
constexpr uint8_t LCD_ROWS = 4;

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// =====================================================
// CONFIGURAÇÕES GERAIS
// =====================================================

constexpr int PONTO_PADRAO = 1;
constexpr int PLACAR_MIN = -99;
constexpr int PLACAR_MAX = 999;

// Aumentado para reduzir efeitos de repique mecânico.
constexpr unsigned long DEBOUNCE_MS = 80;

constexpr unsigned long LCD_REFRESH_MS = 250;
constexpr unsigned long RESET_ZERA_PLACAR_MS = 5000;

// Bloqueio adicional para evitar que um único clique some/subtraia mais de um ponto.
constexpr unsigned long BLOQUEIO_PONTUACAO_MS = 450;

// true  = buzzer passivo, usa tone().
// false = buzzer ativo 5 V, usa HIGH/LOW.
constexpr bool BUZZER_PASSIVO = true;

// =====================================================
// MAPEAMENTO DE PINOS - ARDUINO MEGA 2560
// =====================================================

constexpr uint8_t PINO_BOTAO_AZUL = 22;
constexpr uint8_t PINO_BOTAO_VERDE = 23;
constexpr uint8_t PINO_BOTAO_RESET = 24;

// Chave de pontuação A/V em duas entradas digitais independentes.
constexpr uint8_t PINO_CHAVE_PONTUACAO_AZUL = 28;   // D28
constexpr uint8_t PINO_BOTAO_MAIS_PONTOS = 26;      // D26
constexpr uint8_t PINO_BOTAO_MENOS_PONTOS = 27;     // D27
constexpr uint8_t PINO_CHAVE_PONTUACAO_VERDE = 25;  // D25

constexpr uint8_t PINO_LED_AZUL = 30;
constexpr uint8_t PINO_LED_VERDE = 31;
constexpr uint8_t PINO_LED_PRONTO = 32;
constexpr uint8_t PINO_BUZZER = 8;                  // D8

// LCD I2C no Arduino Mega 2560:
// SDA = pino 20
// SCL = pino 21

// =====================================================
// TIPOS E ESTADOS
// =====================================================

enum Equipe : uint8_t {
  EQUIPE_NENHUMA = 0,
  EQUIPE_AZUL = 1,
  EQUIPE_VERDE = 2
};

struct NotaBuzzer {
  uint16_t frequenciaHz;
  uint16_t duracaoMs;
  uint16_t pausaMs;
};

int placarAzul = 0;
int placarVerde = 0;

Equipe equipeTravada = EQUIPE_NENHUMA;

bool lcdPrecisaAtualizar = true;
unsigned long ultimaAtualizacaoLCD = 0;

const NotaBuzzer* sequenciaAtual = nullptr;
uint8_t totalNotasSequencia = 0;
uint8_t indiceNotaSequencia = 0;
bool buzzerLigado = false;
bool buzzerEmPausa = false;
unsigned long proximaTrocaBuzzer = 0;

bool resetLongoExecutado = false;
unsigned long ultimaAcaoPontuacao = 0;

char mensagemTemporaria[21] = "";
unsigned long mensagemTemporariaAte = 0;

// =====================================================
// SONS DO JOGO
// =====================================================

const NotaBuzzer SOM_INICIALIZACAO[] = {
  {900, 90, 30},
  {1300, 90, 30},
  {1800, 130, 0}
};

const NotaBuzzer SOM_TRAVOU_RODADA[] = {
  {1600, 100, 60},
  {1600, 100, 60},
  {2200, 160, 0}
};

const NotaBuzzer SOM_PONTO_ADICIONADO[] = {
  {1800, 80, 40},
  {2400, 110, 0}
};

const NotaBuzzer SOM_PONTO_REMOVIDO[] = {
  {900, 90, 40},
  {500, 130, 0}
};

const NotaBuzzer SOM_RESET[] = {
  {1200, 80, 40},
  {800, 120, 0}
};

const NotaBuzzer SOM_ERRO[] = {
  {300, 180, 0}
};

const NotaBuzzer SOM_EMPATE[] = {
  {700, 100, 40},
  {700, 100, 40},
  {700, 100, 0}
};

// =====================================================
// DECLARAÇÕES ANTECIPADAS
// =====================================================

void imprimirStatusSerial();
void atualizarSaidas();
void mostrarMensagemTemporaria(const char* texto, unsigned long duracaoMs);
void solicitarAtualizacaoLCD();

// =====================================================
// CLASSE DE BOTÃO COM DEBOUNCE
// =====================================================

class BotaoDebounce {
public:
  void begin(uint8_t pino) {
    pin = pino;
    pinMode(pin, INPUT_PULLUP);

    const bool leituraInicial = digitalRead(pin);
    estadoEstavel = leituraInicial;
    ultimaLeitura = leituraInicial;
    ultimaMudancaLeitura = millis();

    eventoPressionado = false;
    eventoSolto = false;
    inicioPressionado = 0;
  }

  void update() {
    const unsigned long agora = millis();
    const bool leituraAtual = digitalRead(pin);

    if (leituraAtual != ultimaLeitura) {
      ultimaLeitura = leituraAtual;
      ultimaMudancaLeitura = agora;
    }

    if ((agora - ultimaMudancaLeitura) >= DEBOUNCE_MS && leituraAtual != estadoEstavel) {
      estadoEstavel = leituraAtual;

      if (isPressed()) {
        eventoPressionado = true;
        inicioPressionado = agora;
      } else {
        eventoSolto = true;
      }
    }
  }

  bool pressed() {
    if (eventoPressionado) {
      eventoPressionado = false;
      return true;
    }
    return false;
  }

  bool released() {
    if (eventoSolto) {
      eventoSolto = false;
      return true;
    }
    return false;
  }

  bool isPressed() const {
    // INPUT_PULLUP: pressionado = LOW.
    return estadoEstavel == LOW;
  }

  unsigned long pressedFor() const {
    if (!isPressed()) {
      return 0;
    }
    return millis() - inicioPressionado;
  }

private:
  uint8_t pin = 255;
  bool estadoEstavel = HIGH;
  bool ultimaLeitura = HIGH;
  unsigned long ultimaMudancaLeitura = 0;
  unsigned long inicioPressionado = 0;
  bool eventoPressionado = false;
  bool eventoSolto = false;
};

BotaoDebounce botaoAzul;
BotaoDebounce botaoVerde;
BotaoDebounce botaoReset;
BotaoDebounce botaoMaisPontos;
BotaoDebounce botaoMenosPontos;

// =====================================================
// FUNÇÕES DO BUZZER
// =====================================================

void buzzerOn(uint16_t frequenciaHz) {
  buzzerLigado = true;

  if (BUZZER_PASSIVO) {
    tone(PINO_BUZZER, frequenciaHz);
  } else {
    digitalWrite(PINO_BUZZER, HIGH);
  }
}

void buzzerOff() {
  buzzerLigado = false;

  if (BUZZER_PASSIVO) {
    noTone(PINO_BUZZER);
  }

  digitalWrite(PINO_BUZZER, LOW);
}

void iniciarSom(const NotaBuzzer* sequencia, uint8_t totalNotas) {
  if (sequencia == nullptr || totalNotas == 0) {
    return;
  }

  sequenciaAtual = sequencia;
  totalNotasSequencia = totalNotas;
  indiceNotaSequencia = 0;
  buzzerEmPausa = false;

  buzzerOn(sequenciaAtual[indiceNotaSequencia].frequenciaHz);
  proximaTrocaBuzzer = millis() + sequenciaAtual[indiceNotaSequencia].duracaoMs;
}

void pararSom() {
  buzzerOff();
  sequenciaAtual = nullptr;
  totalNotasSequencia = 0;
  indiceNotaSequencia = 0;
  buzzerEmPausa = false;
  proximaTrocaBuzzer = 0;
}

void atualizarBuzzer() {
  if (sequenciaAtual == nullptr) {
    return;
  }

  const unsigned long agora = millis();

  if (agora < proximaTrocaBuzzer) {
    return;
  }

  if (!buzzerEmPausa) {
    buzzerOff();

    const uint16_t pausaAtual = sequenciaAtual[indiceNotaSequencia].pausaMs;

    if (pausaAtual > 0) {
      buzzerEmPausa = true;
      proximaTrocaBuzzer = agora + pausaAtual;
      return;
    }
  }

  indiceNotaSequencia++;

  if (indiceNotaSequencia >= totalNotasSequencia) {
    pararSom();
    return;
  }

  buzzerEmPausa = false;
  buzzerOn(sequenciaAtual[indiceNotaSequencia].frequenciaHz);
  proximaTrocaBuzzer = agora + sequenciaAtual[indiceNotaSequencia].duracaoMs;
}

// =====================================================
// FUNÇÕES DO LCD
// =====================================================

void imprimirLinhaLCD(uint8_t linha, const char* texto) {
  char buffer[21];

  uint8_t i = 0;
  while (i < 20 && texto[i] != '\0') {
    buffer[i] = texto[i];
    i++;
  }

  while (i < 20) {
    buffer[i] = ' ';
    i++;
  }

  buffer[20] = '\0';

  lcd.setCursor(0, linha);
  lcd.print(buffer);
}

void mostrarMensagemTemporaria(const char* texto, unsigned long duracaoMs) {
  strncpy(mensagemTemporaria, texto, 20);
  mensagemTemporaria[20] = '\0';
  mensagemTemporariaAte = millis() + duracaoMs;
  lcdPrecisaAtualizar = true;
}

void solicitarAtualizacaoLCD() {
  lcdPrecisaAtualizar = true;
}

void desenharLCD() {
  char linha[21];

  imprimirLinhaLCD(0, "PASSA OU REPASSA");

  snprintf(linha, sizeof(linha), "AZUL:%3d VERDE:%3d", placarAzul, placarVerde);
  imprimirLinhaLCD(1, linha);

  if (millis() < mensagemTemporariaAte) {
    imprimirLinhaLCD(2, mensagemTemporaria);
  } else if (equipeTravada == EQUIPE_AZUL) {
    imprimirLinhaLCD(2, "TRAVOU: AZUL");
  } else if (equipeTravada == EQUIPE_VERDE) {
    imprimirLinhaLCD(2, "TRAVOU: VERDE");
  } else {
    imprimirLinhaLCD(2, "AGUARDANDO...");
  }

  if (equipeTravada == EQUIPE_NENHUMA) {
    imprimirLinhaLCD(3, "Aperte Azul/Verde");
  } else {
    imprimirLinhaLCD(3, "Reset libera rodada");
  }

  lcdPrecisaAtualizar = false;
  ultimaAtualizacaoLCD = millis();
}

// =====================================================
// FUNÇÕES DO JOGO
// =====================================================

const char* nomeEquipe(Equipe equipe) {
  switch (equipe) {
    case EQUIPE_AZUL:
      return "AZUL";

    case EQUIPE_VERDE:
      return "VERDE";

    default:
      return "NENHUMA";
  }
}

int limitarPlacar(int valor) {
  if (valor < PLACAR_MIN) {
    return PLACAR_MIN;
  }

  if (valor > PLACAR_MAX) {
    return PLACAR_MAX;
  }

  return valor;
}

Equipe equipeSelecionadaParaPontuar() {
  const bool azulSelecionado = digitalRead(PINO_CHAVE_PONTUACAO_AZUL) == LOW;
  const bool verdeSelecionado = digitalRead(PINO_CHAVE_PONTUACAO_VERDE) == LOW;

  if (azulSelecionado && !verdeSelecionado) {
    return EQUIPE_AZUL;
  }

  if (verdeSelecionado && !azulSelecionado) {
    return EQUIPE_VERDE;
  }

  // Atalho operacional:
  // Se nenhuma chave estiver selecionada, pontua a equipe que travou a rodada.
  if (!azulSelecionado && !verdeSelecionado && equipeTravada != EQUIPE_NENHUMA) {
    return equipeTravada;
  }

  return EQUIPE_NENHUMA;
}

void atualizarSaidas() {
  digitalWrite(PINO_LED_AZUL, equipeTravada == EQUIPE_AZUL ? HIGH : LOW);
  digitalWrite(PINO_LED_VERDE, equipeTravada == EQUIPE_VERDE ? HIGH : LOW);
}

void atualizarLedPronto() {
  const unsigned long agora = millis();

  if (equipeTravada == EQUIPE_NENHUMA) {
    digitalWrite(PINO_LED_PRONTO, HIGH);
  } else {
    // Pisca quando a rodada estiver travada.
    digitalWrite(PINO_LED_PRONTO, ((agora / 300) % 2) ? HIGH : LOW);
  }
}

void resetarRodada() {
  equipeTravada = EQUIPE_NENHUMA;
  pararSom();
  atualizarSaidas();
  iniciarSom(SOM_RESET, sizeof(SOM_RESET) / sizeof(SOM_RESET[0]));
  mostrarMensagemTemporaria("RODADA LIBERADA", 1000);

  Serial.println(F("[RESET] Rodada liberada."));
}

void zerarPlacar() {
  placarAzul = 0;
  placarVerde = 0;
  mostrarMensagemTemporaria("PLACAR ZERADO", 1200);

  Serial.println(F("[PLACAR] Placar zerado."));
}

void zerarPlacarERodada() {
  equipeTravada = EQUIPE_NENHUMA;
  pararSom();
  atualizarSaidas();
  zerarPlacar();
  iniciarSom(SOM_RESET, sizeof(SOM_RESET) / sizeof(SOM_RESET[0]));
}

void travarRodada(Equipe equipe) {
  if (equipeTravada != EQUIPE_NENHUMA) {
    return;
  }

  equipeTravada = equipe;
  atualizarSaidas();
  iniciarSom(SOM_TRAVOU_RODADA, sizeof(SOM_TRAVOU_RODADA) / sizeof(SOM_TRAVOU_RODADA[0]));
  solicitarAtualizacaoLCD();

  Serial.print(F("[RODADA] Travou equipe: "));
  Serial.println(nomeEquipe(equipe));
}

void alterarPlacar(Equipe equipe, int delta) {
  if (equipe == EQUIPE_AZUL) {
    placarAzul = limitarPlacar(placarAzul + delta);
  } else if (equipe == EQUIPE_VERDE) {
    placarVerde = limitarPlacar(placarVerde + delta);
  } else {
    iniciarSom(SOM_ERRO, sizeof(SOM_ERRO) / sizeof(SOM_ERRO[0]));
    mostrarMensagemTemporaria("SELECIONE EQUIPE", 1200);
    Serial.println(F("[ERRO] Nenhuma equipe selecionada para pontuar."));
    return;
  }

  if (delta > 0) {
    iniciarSom(SOM_PONTO_ADICIONADO, sizeof(SOM_PONTO_ADICIONADO) / sizeof(SOM_PONTO_ADICIONADO[0]));
  } else {
    iniciarSom(SOM_PONTO_REMOVIDO, sizeof(SOM_PONTO_REMOVIDO) / sizeof(SOM_PONTO_REMOVIDO[0]));
  }

  solicitarAtualizacaoLCD();

  Serial.print(F("[PLACAR] "));
  Serial.print(nomeEquipe(equipe));
  Serial.print(delta > 0 ? F(" +") : F(" "));
  Serial.println(delta);

  imprimirStatusSerial();
}

// =====================================================
// MONITOR SERIAL
// =====================================================

void imprimirAjudaSerial() {
  Serial.println();
  Serial.println(F("=== PASSA OU REPASSA - COMANDOS SERIAL ==="));
  Serial.println(F("a  -> simula botao da equipe AZUL"));
  Serial.println(F("v  -> simula botao da equipe VERDE"));
  Serial.println(F("r  -> reset/libera rodada"));
  Serial.println(F("1  -> AZUL +1"));
  Serial.println(F("2  -> AZUL -1"));
  Serial.println(F("3  -> VERDE +1"));
  Serial.println(F("4  -> VERDE -1"));
  Serial.println(F("c  -> zera placar"));
  Serial.println(F("s  -> mostra status"));
  Serial.println(F("?  -> ajuda"));
  Serial.println();
}

void imprimirStatusSerial() {
  Serial.print(F("[STATUS] Azul: "));
  Serial.print(placarAzul);
  Serial.print(F(" | Verde: "));
  Serial.print(placarVerde);
  Serial.print(F(" | Rodada: "));
  Serial.println(nomeEquipe(equipeTravada));
}

void processarSerial() {
  while (Serial.available() > 0) {
    const char comando = Serial.read();

    switch (comando) {
      case 'a':
      case 'A':
        travarRodada(EQUIPE_AZUL);
        break;

      case 'v':
      case 'V':
        travarRodada(EQUIPE_VERDE);
        break;

      case 'r':
      case 'R':
        resetarRodada();
        break;

      case '1':
        alterarPlacar(EQUIPE_AZUL, PONTO_PADRAO);
        break;

      case '2':
        alterarPlacar(EQUIPE_AZUL, -PONTO_PADRAO);
        break;

      case '3':
        alterarPlacar(EQUIPE_VERDE, PONTO_PADRAO);
        break;

      case '4':
        alterarPlacar(EQUIPE_VERDE, -PONTO_PADRAO);
        break;

      case 'c':
      case 'C':
        zerarPlacarERodada();
        break;

      case 's':
      case 'S':
        imprimirStatusSerial();
        break;

      case '?':
        imprimirAjudaSerial();
        break;

      default:
        break;
    }
  }
}

// =====================================================
// LEITURA E PROCESSAMENTO DOS BOTÕES
// =====================================================

void atualizarBotoes() {
  botaoAzul.update();
  botaoVerde.update();
  botaoReset.update();
  botaoMaisPontos.update();
  botaoMenosPontos.update();
}

void processarBotoesResposta() {
  const bool azulPressionado = botaoAzul.pressed();
  const bool verdePressionado = botaoVerde.pressed();

  if (equipeTravada != EQUIPE_NENHUMA) {
    return;
  }

  if (azulPressionado && !verdePressionado) {
    travarRodada(EQUIPE_AZUL);
  } else if (verdePressionado && !azulPressionado) {
    travarRodada(EQUIPE_VERDE);
  } else if (azulPressionado && verdePressionado) {
    mostrarMensagemTemporaria("EMPATE - REPETIR", 1200);
    iniciarSom(SOM_EMPATE, sizeof(SOM_EMPATE) / sizeof(SOM_EMPATE[0]));
    Serial.println(F("[RODADA] Pressionamento simultaneo. Repetir rodada."));
  }
}

bool pontuacaoLiberada() {
  return (millis() - ultimaAcaoPontuacao) >= BLOQUEIO_PONTUACAO_MS;
}

void registrarBloqueioPontuacao() {
  ultimaAcaoPontuacao = millis();
}

void processarBotoesPontuacao() {
  const bool maisPressionado = botaoMaisPontos.pressed();
  const bool menosPressionado = botaoMenosPontos.pressed();

  if (!maisPressionado && !menosPressionado) {
    return;
  }

  if (!pontuacaoLiberada()) {
    return;
  }

  registrarBloqueioPontuacao();

  if (maisPressionado && !menosPressionado) {
    alterarPlacar(equipeSelecionadaParaPontuar(), PONTO_PADRAO);
  } else if (menosPressionado && !maisPressionado) {
    alterarPlacar(equipeSelecionadaParaPontuar(), -PONTO_PADRAO);
  } else {
    iniciarSom(SOM_ERRO, sizeof(SOM_ERRO) / sizeof(SOM_ERRO[0]));
    mostrarMensagemTemporaria("USE + OU -", 1000);
    Serial.println(F("[ERRO] Botões + e - pressionados simultaneamente."));
  }
}

void processarBotaoReset() {
  if (botaoReset.pressed()) {
    resetLongoExecutado = false;
  }

  if (botaoReset.isPressed() && !resetLongoExecutado && botaoReset.pressedFor() >= RESET_ZERA_PLACAR_MS) {
    zerarPlacarERodada();
    resetLongoExecutado = true;
  }

  if (botaoReset.released()) {
    if (!resetLongoExecutado) {
      resetarRodada();
    }
  }
}

// =====================================================
// SETUP E LOOP
// =====================================================

void setup() {
  Serial.begin(115200);

  pinMode(PINO_CHAVE_PONTUACAO_AZUL, INPUT_PULLUP);
  pinMode(PINO_CHAVE_PONTUACAO_VERDE, INPUT_PULLUP);

  pinMode(PINO_LED_AZUL, OUTPUT);
  pinMode(PINO_LED_VERDE, OUTPUT);
  pinMode(PINO_LED_PRONTO, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);

  digitalWrite(PINO_LED_AZUL, LOW);
  digitalWrite(PINO_LED_VERDE, LOW);
  digitalWrite(PINO_LED_PRONTO, LOW);
  digitalWrite(PINO_BUZZER, LOW);

  botaoAzul.begin(PINO_BOTAO_AZUL);
  botaoVerde.begin(PINO_BOTAO_VERDE);
  botaoReset.begin(PINO_BOTAO_RESET);
  botaoMaisPontos.begin(PINO_BOTAO_MAIS_PONTOS);
  botaoMenosPontos.begin(PINO_BOTAO_MENOS_PONTOS);

  Wire.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();

  imprimirLinhaLCD(0, "PASSA OU REPASSA");
  imprimirLinhaLCD(1, "Inicializando...");
  imprimirLinhaLCD(2, "Mega 2560 + LCD");
  imprimirLinhaLCD(3, "Buzzer D8 ativo");

  delay(900);

  lcd.clear();
  desenharLCD();
  imprimirAjudaSerial();
  iniciarSom(SOM_INICIALIZACAO, sizeof(SOM_INICIALIZACAO) / sizeof(SOM_INICIALIZACAO[0]));

  Serial.println(F("[OK] Sistema iniciado."));
}

void loop() {
  atualizarBotoes();

  processarBotoesResposta();
  processarBotoesPontuacao();
  processarBotaoReset();
  processarSerial();

  atualizarBuzzer();
  atualizarLedPronto();

  if (lcdPrecisaAtualizar || (millis() - ultimaAtualizacaoLCD >= LCD_REFRESH_MS)) {
    desenharLCD();
  }
}
