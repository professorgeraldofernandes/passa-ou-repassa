/*
  Projeto: Passa ou Repassa
  Arquivo: firmware/passa_ou_repassa/passa_ou_repassa.ino
  Versão: 0.2.0
  Plataforma: Arduino Mega 2560
  Linguagem: C++ / Arduino
  Display: LCD 20x4 I2C

  Descrição:
  Controle eletrônico do jogo Passa ou Repassa com duas equipes,
  travamento da primeira equipe que pressionar, sinalização por LEDs,
  buzzer, reset de rodada, pontuação manual e placar no display LCD 20x4.

  Recursos implementados:
  - Botão de resposta da equipe Azul.
  - Botão de resposta da equipe Vermelha.
  - Travamento da rodada pela primeira equipe que pressionar.
  - LEDs indicadores da equipe que travou.
  - LED de status/pronto.
  - Buzzer de alerta da rodada.
  - Reset curto para liberar nova rodada.
  - Reset pressionado por 5 segundos para zerar placar.
  - Botões + e - para ajuste de pontuação.
  - Duas entradas digitais para seleção de pontuação: Azul e Vermelha.
  - Pontuação exibida no LCD 20x4.
  - Comandos de teste pelo Monitor Serial.

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

constexpr unsigned long DEBOUNCE_MS = 45;
constexpr unsigned long LCD_REFRESH_MS = 250;
constexpr unsigned long ALERTA_RODADA_MS = 1200;
constexpr unsigned long ALERTA_INTERVALO_MS = 120;
constexpr unsigned long RESET_ZERA_PLACAR_MS = 5000;

// true  = buzzer passivo, usa tone().
// false = buzzer ativo 5 V, usa HIGH/LOW.
constexpr bool BUZZER_PASSIVO = true;

// =====================================================
// MAPEAMENTO DE PINOS - ARDUINO MEGA 2560
// =====================================================

constexpr uint8_t PINO_BOTAO_AZUL = 22;
constexpr uint8_t PINO_BOTAO_VERMELHO = 23;
constexpr uint8_t PINO_BOTAO_RESET = 24;

// Chave seletora A/V corrigida para duas entradas digitais independentes.
constexpr uint8_t PINO_CHAVE_PONTUACAO_AZUL = 25;
constexpr uint8_t PINO_BOTAO_MAIS_PONTOS = 26;
constexpr uint8_t PINO_BOTAO_MENOS_PONTOS = 27;
constexpr uint8_t PINO_CHAVE_PONTUACAO_VERMELHA = 28;

constexpr uint8_t PINO_LED_AZUL = 30;
constexpr uint8_t PINO_LED_VERMELHO = 31;
constexpr uint8_t PINO_LED_PRONTO = 32;
constexpr uint8_t PINO_BUZZER = 8;

// LCD I2C no Arduino Mega 2560:
// SDA = pino 20
// SCL = pino 21

// =====================================================
// TIPOS E ESTADOS
// =====================================================

enum Equipe : uint8_t {
  EQUIPE_NENHUMA = 0,
  EQUIPE_AZUL = 1,
  EQUIPE_VERMELHA = 2
};

enum BuzzerModo : uint8_t {
  BUZZER_DESLIGADO = 0,
  BUZZER_BIP_CURTO = 1,
  BUZZER_ALERTA_RODADA = 2
};

int placarAzul = 0;
int placarVermelho = 0;

Equipe equipeTravada = EQUIPE_NENHUMA;

bool lcdPrecisaAtualizar = true;
unsigned long ultimaAtualizacaoLCD = 0;

BuzzerModo buzzerModo = BUZZER_DESLIGADO;
unsigned long buzzerFim = 0;
unsigned long buzzerUltimaAlternancia = 0;
bool buzzerLigado = false;

bool resetLongoExecutado = false;

char mensagemTemporaria[21] = "";
unsigned long mensagemTemporariaAte = 0;

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
BotaoDebounce botaoVermelho;
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

void iniciarBipCurto(uint16_t frequenciaHz, unsigned long duracaoMs) {
  buzzerModo = BUZZER_BIP_CURTO;
  buzzerFim = millis() + duracaoMs;
  buzzerOn(frequenciaHz);
}

void iniciarAlertaRodada() {
  buzzerModo = BUZZER_ALERTA_RODADA;
  buzzerFim = millis() + ALERTA_RODADA_MS;
  buzzerUltimaAlternancia = 0;
  buzzerLigado = false;
}

void atualizarBuzzer() {
  const unsigned long agora = millis();

  if (buzzerModo == BUZZER_DESLIGADO) {
    return;
  }

  if (agora >= buzzerFim) {
    buzzerOff();
    buzzerModo = BUZZER_DESLIGADO;
    return;
  }

  if (buzzerModo == BUZZER_ALERTA_RODADA) {
    if (agora - buzzerUltimaAlternancia >= ALERTA_INTERVALO_MS) {
      buzzerUltimaAlternancia = agora;

      if (buzzerLigado) {
        buzzerOff();
      } else {
        buzzerOn(1600);
      }
    }
  }
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

  snprintf(linha, sizeof(linha), "AZUL:%3d VERM:%3d", placarAzul, placarVermelho);
  imprimirLinhaLCD(1, linha);

  if (millis() < mensagemTemporariaAte) {
    imprimirLinhaLCD(2, mensagemTemporaria);
  } else if (equipeTravada == EQUIPE_AZUL) {
    imprimirLinhaLCD(2, "TRAVOU: AZUL");
  } else if (equipeTravada == EQUIPE_VERMELHA) {
    imprimirLinhaLCD(2, "TRAVOU: VERMELHA");
  } else {
    imprimirLinhaLCD(2, "AGUARDANDO...");
  }

  if (equipeTravada == EQUIPE_NENHUMA) {
    imprimirLinhaLCD(3, "Aperte Azul/Verm.");
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

    case EQUIPE_VERMELHA:
      return "VERMELHA";

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
  const bool vermelhaSelecionada = digitalRead(PINO_CHAVE_PONTUACAO_VERMELHA) == LOW;

  if (azulSelecionado && !vermelhaSelecionada) {
    return EQUIPE_AZUL;
  }

  if (vermelhaSelecionada && !azulSelecionado) {
    return EQUIPE_VERMELHA;
  }

  // Atalho operacional:
  // Se nenhuma chave estiver selecionada, pontua a equipe que travou a rodada.
  if (!azulSelecionado && !vermelhaSelecionada && equipeTravada != EQUIPE_NENHUMA) {
    return equipeTravada;
  }

  return EQUIPE_NENHUMA;
}

void atualizarSaidas() {
  digitalWrite(PINO_LED_AZUL, equipeTravada == EQUIPE_AZUL ? HIGH : LOW);
  digitalWrite(PINO_LED_VERMELHO, equipeTravada == EQUIPE_VERMELHA ? HIGH : LOW);
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
  buzzerOff();
  buzzerModo = BUZZER_DESLIGADO;
  atualizarSaidas();
  mostrarMensagemTemporaria("RODADA LIBERADA", 1000);

  Serial.println(F("[RESET] Rodada liberada."));
}

void zerarPlacar() {
  placarAzul = 0;
  placarVermelho = 0;
  mostrarMensagemTemporaria("PLACAR ZERADO", 1200);

  Serial.println(F("[PLACAR] Placar zerado."));
}

void zerarPlacarERodada() {
  equipeTravada = EQUIPE_NENHUMA;
  buzzerOff();
  buzzerModo = BUZZER_DESLIGADO;
  atualizarSaidas();
  zerarPlacar();
}

void travarRodada(Equipe equipe) {
  if (equipeTravada != EQUIPE_NENHUMA) {
    return;
  }

  equipeTravada = equipe;
  atualizarSaidas();
  iniciarAlertaRodada();
  solicitarAtualizacaoLCD();

  Serial.print(F("[RODADA] Travou equipe: "));
  Serial.println(nomeEquipe(equipe));
}

void alterarPlacar(Equipe equipe, int delta) {
  if (equipe == EQUIPE_AZUL) {
    placarAzul = limitarPlacar(placarAzul + delta);
  } else if (equipe == EQUIPE_VERMELHA) {
    placarVermelho = limitarPlacar(placarVermelho + delta);
  } else {
    iniciarBipCurto(350, 180);
    mostrarMensagemTemporaria("SELECIONE EQUIPE", 1200);
    Serial.println(F("[ERRO] Nenhuma equipe selecionada para pontuar."));
    return;
  }

  iniciarBipCurto(delta > 0 ? 2200 : 700, 90);
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
  Serial.println(F("v  -> simula botao da equipe VERMELHA"));
  Serial.println(F("r  -> reset/libera rodada"));
  Serial.println(F("1  -> AZUL +1"));
  Serial.println(F("2  -> AZUL -1"));
  Serial.println(F("3  -> VERMELHA +1"));
  Serial.println(F("4  -> VERMELHA -1"));
  Serial.println(F("c  -> zera placar"));
  Serial.println(F("s  -> mostra status"));
  Serial.println(F("?  -> ajuda"));
  Serial.println();
}

void imprimirStatusSerial() {
  Serial.print(F("[STATUS] Azul: "));
  Serial.print(placarAzul);
  Serial.print(F(" | Vermelha: "));
  Serial.print(placarVermelho);
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
        travarRodada(EQUIPE_VERMELHA);
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
        alterarPlacar(EQUIPE_VERMELHA, PONTO_PADRAO);
        break;

      case '4':
        alterarPlacar(EQUIPE_VERMELHA, -PONTO_PADRAO);
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
  botaoVermelho.update();
  botaoReset.update();
  botaoMaisPontos.update();
  botaoMenosPontos.update();
}

void processarBotoesResposta() {
  const bool azulPressionado = botaoAzul.pressed();
  const bool vermelhoPressionado = botaoVermelho.pressed();

  if (equipeTravada != EQUIPE_NENHUMA) {
    return;
  }

  if (azulPressionado && !vermelhoPressionado) {
    travarRodada(EQUIPE_AZUL);
  } else if (vermelhoPressionado && !azulPressionado) {
    travarRodada(EQUIPE_VERMELHA);
  } else if (azulPressionado && vermelhoPressionado) {
    mostrarMensagemTemporaria("EMPATE - REPETIR", 1200);
    iniciarBipCurto(500, 250);
    Serial.println(F("[RODADA] Pressionamento simultaneo. Repetir rodada."));
  }
}

void processarBotoesPontuacao() {
  if (botaoMaisPontos.pressed()) {
    alterarPlacar(equipeSelecionadaParaPontuar(), PONTO_PADRAO);
  }

  if (botaoMenosPontos.pressed()) {
    alterarPlacar(equipeSelecionadaParaPontuar(), -PONTO_PADRAO);
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
  pinMode(PINO_CHAVE_PONTUACAO_VERMELHA, INPUT_PULLUP);

  pinMode(PINO_LED_AZUL, OUTPUT);
  pinMode(PINO_LED_VERMELHO, OUTPUT);
  pinMode(PINO_LED_PRONTO, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);

  digitalWrite(PINO_LED_AZUL, LOW);
  digitalWrite(PINO_LED_VERMELHO, LOW);
  digitalWrite(PINO_LED_PRONTO, LOW);
  digitalWrite(PINO_BUZZER, LOW);

  botaoAzul.begin(PINO_BOTAO_AZUL);
  botaoVermelho.begin(PINO_BOTAO_VERMELHO);
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
  imprimirLinhaLCD(3, "SDA20 SCL21");

  delay(900);

  lcd.clear();
  desenharLCD();
  imprimirAjudaSerial();

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
