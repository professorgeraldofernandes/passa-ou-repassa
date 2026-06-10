/*
  Projeto: Passa ou Repassa
  Arquivo: passa_ou_repassa_atualizado.ino
  Placa: Arduino Mega 2560
  Display: LCD 20x4 I2C

  Funções principais:
  - Dois botões de resposta: Equipe Azul e Equipe Vermelha.
  - A primeira equipe que apertar trava a rodada.
  - LED e buzzer indicam a equipe que travou.
  - Reset libera a próxima rodada sem apagar o placar.
  - Pontuação manual pelo mediador com teclas + e -.
  - Seletor/chave de equipe para pontuação: Azul ou Vermelha.
  - Se nenhuma chave estiver selecionada, + e - pontuam a equipe que travou.
  - LCD 20x4 exibe placar e estado do jogo.
  - Monitor Serial permite testes e simulação dos comandos.

  Biblioteca LCD:
  - LiquidCrystal I2C by Frank de Brabander
  - Repositório: johnrickman/LiquidCrystal_I2C
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =====================================================
// CONFIGURAÇÕES GERAIS
// =====================================================

// Endereço comum do LCD I2C: 0x27 ou 0x3F.
// Caso o display não mostre nada, teste trocar para 0x3F.
constexpr uint8_t LCD_ADDR = 0x27;
constexpr uint8_t LCD_COLS = 20;
constexpr uint8_t LCD_ROWS = 4;

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// Valor somado/subtraído a cada comando de pontuação.
constexpr int PONTO_PADRAO = 1;

// Limites de segurança para exibição no LCD.
constexpr int PLACAR_MIN = -99;
constexpr int PLACAR_MAX = 999;

// Debounce dos botões.
constexpr unsigned long DEBOUNCE_MS = 45;

// Atualização periódica do LCD.
constexpr unsigned long LCD_REFRESH_MS = 250;

// Tempo do alerta sonoro ao travar uma rodada.
constexpr unsigned long ALERTA_RODADA_MS = 1200;
constexpr unsigned long ALERTA_INTERVALO_MS = 120;

// Para buzzer passivo, mantenha true.
// Para buzzer ativo 5 V, altere para false.
constexpr bool BUZZER_PASSIVO = true;

// Segurar RESET por este tempo zera o placar.
constexpr unsigned long RESET_ZERA_PLACAR_MS = 5000;

// =====================================================
// MAPA DE PINOS - ARDUINO MEGA 2560
// =====================================================

// Botões principais do jogo.
constexpr uint8_t PIN_BTN_AZUL = 22;
constexpr uint8_t PIN_BTN_VERMELHO = 23;
constexpr uint8_t PIN_BTN_RESET = 24;

// Chaves/seletor de equipe para pontuação.
constexpr uint8_t PIN_SEL_AZUL = 25;
constexpr uint8_t PIN_BTN_MAIS = 26;
constexpr uint8_t PIN_BTN_MENOS = 27;
constexpr uint8_t PIN_SEL_VERMELHO = 28;

// Saídas.
constexpr uint8_t PIN_LED_AZUL = 30;
constexpr uint8_t PIN_LED_VERMELHO = 31;
constexpr uint8_t PIN_LED_STATUS = 32;
constexpr uint8_t PIN_BUZZER = 8;

// LCD I2C no Mega:
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
// CLASSE DE BOTÃO COM DEBOUNCE
// =====================================================

class BotaoDebounce {
public:
  void begin(uint8_t pino) {
    pin = pino;
    pinMode(pin, INPUT_PULLUP);

    bool leituraInicial = digitalRead(pin);
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

BotaoDebounce btnAzul;
BotaoDebounce btnVermelho;
BotaoDebounce btnReset;
BotaoDebounce btnMais;
BotaoDebounce btnMenos;

// =====================================================
// FUNÇÕES AUXILIARES
// =====================================================

void buzzerOn(uint16_t frequenciaHz) {
  buzzerLigado = true;

  if (BUZZER_PASSIVO) {
    tone(PIN_BUZZER, frequenciaHz);
  } else {
    digitalWrite(PIN_BUZZER, HIGH);
  }
}

void buzzerOff() {
  buzzerLigado = false;

  if (BUZZER_PASSIVO) {
    noTone(PIN_BUZZER);
  }

  digitalWrite(PIN_BUZZER, LOW);
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

void mostrarMensagemTemporaria(const char* texto, unsigned long duracaoMs) {
  strncpy(mensagemTemporaria, texto, 20);
  mensagemTemporaria[20] = '\0';
  mensagemTemporariaAte = millis() + duracaoMs;
  lcdPrecisaAtualizar = true;
}

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
  const bool azulSelecionado = digitalRead(PIN_SEL_AZUL) == LOW;
  const bool vermelhoSelecionado = digitalRead(PIN_SEL_VERMELHO) == LOW;

  if (azulSelecionado && !vermelhoSelecionado) {
    return EQUIPE_AZUL;
  }

  if (vermelhoSelecionado && !azulSelecionado) {
    return EQUIPE_VERMELHA;
  }

  // Atalho operacional:
  // se nenhuma chave estiver selecionada, pontua a equipe que travou a rodada.
  if (!azulSelecionado && !vermelhoSelecionado && equipeTravada != EQUIPE_NENHUMA) {
    return equipeTravada;
  }

  return EQUIPE_NENHUMA;
}

void atualizarSaidas() {
  digitalWrite(PIN_LED_AZUL, equipeTravada == EQUIPE_AZUL ? HIGH : LOW);
  digitalWrite(PIN_LED_VERMELHO, equipeTravada == EQUIPE_VERMELHA ? HIGH : LOW);
}

void atualizarLedStatus() {
  const unsigned long agora = millis();

  if (equipeTravada == EQUIPE_NENHUMA) {
    digitalWrite(PIN_LED_STATUS, HIGH);
  } else {
    // Pisca quando a rodada estiver travada.
    digitalWrite(PIN_LED_STATUS, ((agora / 300) % 2) ? HIGH : LOW);
  }
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

void solicitarAtualizacaoLCD() {
  lcdPrecisaAtualizar = true;
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
        zerarPlacar();
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

void atualizarBotoes() {
  btnAzul.update();
  btnVermelho.update();
  btnReset.update();
  btnMais.update();
  btnMenos.update();
}

void processarBotoesResposta() {
  const bool azulPressionado = btnAzul.pressed();
  const bool vermelhoPressionado = btnVermelho.pressed();

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
  if (btnMais.pressed()) {
    alterarPlacar(equipeSelecionadaParaPontuar(), PONTO_PADRAO);
  }

  if (btnMenos.pressed()) {
    alterarPlacar(equipeSelecionadaParaPontuar(), -PONTO_PADRAO);
  }
}

void processarBotaoReset() {
  if (btnReset.pressed()) {
    resetLongoExecutado = false;
  }

  if (btnReset.isPressed() && !resetLongoExecutado && btnReset.pressedFor() >= RESET_ZERA_PLACAR_MS) {
    zerarPlacar();
    resetarRodada();
    resetLongoExecutado = true;
  }

  if (btnReset.released()) {
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

  pinMode(PIN_SEL_AZUL, INPUT_PULLUP);
  pinMode(PIN_SEL_VERMELHO, INPUT_PULLUP);

  pinMode(PIN_LED_AZUL, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_LED_AZUL, LOW);
  digitalWrite(PIN_LED_VERMELHO, LOW);
  digitalWrite(PIN_LED_STATUS, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  btnAzul.begin(PIN_BTN_AZUL);
  btnVermelho.begin(PIN_BTN_VERMELHO);
  btnReset.begin(PIN_BTN_RESET);
  btnMais.begin(PIN_BTN_MAIS);
  btnMenos.begin(PIN_BTN_MENOS);

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
  atualizarLedStatus();

  if (lcdPrecisaAtualizar || (millis() - ultimaAtualizacaoLCD >= LCD_REFRESH_MS)) {
    desenharLCD();
  }
}
