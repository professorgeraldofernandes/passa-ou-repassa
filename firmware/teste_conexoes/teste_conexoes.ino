/*
  Projeto: Passa ou Repassa
  Sketch: teste_conexoes.ino
  Plataforma: Arduino Mega 2560

  Objetivo:
  - Detectar botoes/chaves pressionados.
  - Mostrar eventos e status no Monitor Serial.
  - Mostrar status no LCD I2C 20x4.
  - Escanear o barramento I2C para validar os displays conectados.

  Ligacao recomendada dos botoes/chaves:
  - Um terminal do botao no pino digital do Arduino.
  - Outro terminal do botao no GND.
  - O codigo usa INPUT_PULLUP, portanto:
    SOLTO       = HIGH
    PRESSIONADO = LOW

  Bibliotecas:
  - Wire
  - LiquidCrystal_I2C
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =====================================================
// Configuracao do LCD I2C 20x4
// =====================================================

constexpr uint8_t ENDERECO_LCD_PREFERENCIAL = 0x27;
constexpr uint8_t ENDERECO_LCD_ALTERNATIVO  = 0x3F;
constexpr uint8_t LCD_COLUNAS = 20;
constexpr uint8_t LCD_LINHAS  = 4;

LiquidCrystal_I2C *lcd20x4 = NULL;
uint8_t enderecoLCD20x4 = 0;

// =====================================================
// Pinos das entradas - Arduino Mega 2560
// Ajuste aqui se alterar a fiacao.
// =====================================================

constexpr uint8_t PINO_BOTAO_AZUL         = 22;
constexpr uint8_t PINO_BOTAO_VERDE        = 23;
constexpr uint8_t PINO_BOTAO_RESET        = 24;
constexpr uint8_t PINO_CHAVE_PONTUACAO_A  = 28;
constexpr uint8_t PINO_BOTAO_MAIS_PONTOS  = 26;
constexpr uint8_t PINO_BOTAO_MENOS_PONTOS = 27;
constexpr uint8_t PINO_CHAVE_PONTUACAO_V  = 25;

// =====================================================
// Saidas auxiliares de teste
// =====================================================

constexpr uint8_t PINO_LED_AZUL  = 30;
constexpr uint8_t PINO_LED_VERDE = 31;
constexpr uint8_t PINO_LED_OK    = 32;
constexpr uint8_t PINO_BUZZER    = 8;

// =====================================================
// Parametros gerais
// =====================================================

constexpr unsigned long TEMPO_DEBOUNCE_MS          = 45;
constexpr unsigned long INTERVALO_SERIAL_STATUS_MS = 1000;
constexpr unsigned long INTERVALO_LCD_MS           = 150;
constexpr unsigned int  FREQUENCIA_BEEP_HZ         = 1800;
constexpr unsigned long DURACAO_BEEP_MS            = 60;
constexpr uint8_t       MAX_DISPOSITIVOS_I2C       = 12;

// =====================================================
// Estruturas
// =====================================================

struct EntradaDigital {
  const char *nome;
  const char *sigla;
  uint8_t pino;
  bool leituraInstavel;
  bool pressionado;
  unsigned long instanteMudanca;
};

EntradaDigital entradas[] = {
  {"BOTAO AZUL",         "Az", PINO_BOTAO_AZUL,         false, false, 0},
  {"BOTAO VERDE",        "Vd", PINO_BOTAO_VERDE,        false, false, 0},
  {"BOTAO RESET",        "Rs", PINO_BOTAO_RESET,        false, false, 0},
  {"CHAVE PONTUACAO A",  "A",  PINO_CHAVE_PONTUACAO_A,  false, false, 0},
  {"BOTAO + PONTOS",     "+",  PINO_BOTAO_MAIS_PONTOS,  false, false, 0},
  {"BOTAO - PONTOS",     "-",  PINO_BOTAO_MENOS_PONTOS, false, false, 0},
  {"CHAVE PONTUACAO V",  "V",  PINO_CHAVE_PONTUACAO_V,  false, false, 0}
};

const uint8_t TOTAL_ENTRADAS = sizeof(entradas) / sizeof(entradas[0]);

uint8_t dispositivosI2C[MAX_DISPOSITIVOS_I2C];
uint8_t totalDispositivosI2C = 0;

unsigned long ultimaAtualizacaoSerial = 0;
unsigned long ultimaAtualizacaoLCD = 0;

char ultimoAcionamento[17] = "Nenhum";

// =====================================================
// Prototipos
// =====================================================

void configurarPinos();
void inicializarEntradas();
void escanearBarramentoI2C();
bool dispositivoI2CResponde(uint8_t endereco);
bool enderecoI2CEncontrado(uint8_t endereco);
uint8_t selecionarEnderecoLCD();
void inicializarLCD();
void atualizarEntradas();
void tratarMudancaEntrada(EntradaDigital &entrada, bool novoEstadoPressionado);
void atualizarSaidasIndicadoras();
void atualizarMonitorSerial(bool forcarImpressao);
void atualizarLCD();
void escreverLinhaLCD(uint8_t linha, const char *texto);
void imprimirCabecalhoSerial();
void imprimirResumoI2C();
void imprimirEventoEntrada(const EntradaDigital &entrada, bool pressionado);
void copiarUltimoAcionamento(const char *texto);
void beepCurto();
bool lerPressionado(uint8_t pino);
const char *textoEstado(bool pressionado);
const char *textoCurto(bool pressionado);
uint8_t contarEntradasPressionadas();

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(9600);
  delay(300);

  imprimirCabecalhoSerial();
  configurarPinos();
  inicializarEntradas();

  Wire.begin();
  escanearBarramentoI2C();
  enderecoLCD20x4 = selecionarEnderecoLCD();
  inicializarLCD();
  imprimirResumoI2C();

  Serial.println(F("Sistema pronto. Pressione os botoes para testar as conexoes."));
  Serial.println();

  atualizarMonitorSerial(true);
  atualizarLCD();
}

// =====================================================
// Loop principal
// =====================================================

void loop() {
  atualizarEntradas();
  atualizarSaidasIndicadoras();

  const unsigned long agora = millis();

  if (agora - ultimaAtualizacaoSerial >= INTERVALO_SERIAL_STATUS_MS) {
    ultimaAtualizacaoSerial = agora;
    atualizarMonitorSerial(false);
  }

  if (agora - ultimaAtualizacaoLCD >= INTERVALO_LCD_MS) {
    ultimaAtualizacaoLCD = agora;
    atualizarLCD();
  }
}

// =====================================================
// Configuracao
// =====================================================

void configurarPinos() {
  for (uint8_t i = 0; i < TOTAL_ENTRADAS; i++) {
    pinMode(entradas[i].pino, INPUT_PULLUP);
  }

  pinMode(PINO_LED_AZUL, OUTPUT);
  pinMode(PINO_LED_VERDE, OUTPUT);
  pinMode(PINO_LED_OK, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);

  digitalWrite(PINO_LED_AZUL, LOW);
  digitalWrite(PINO_LED_VERDE, LOW);
  digitalWrite(PINO_LED_OK, LOW);
  noTone(PINO_BUZZER);
}

void inicializarEntradas() {
  const unsigned long agora = millis();

  for (uint8_t i = 0; i < TOTAL_ENTRADAS; i++) {
    const bool estadoInicial = lerPressionado(entradas[i].pino);
    entradas[i].leituraInstavel = estadoInicial;
    entradas[i].pressionado = estadoInicial;
    entradas[i].instanteMudanca = agora;
  }
}

// =====================================================
// Leitura com debounce
// =====================================================

void atualizarEntradas() {
  const unsigned long agora = millis();

  for (uint8_t i = 0; i < TOTAL_ENTRADAS; i++) {
    EntradaDigital &entrada = entradas[i];
    const bool leituraAtual = lerPressionado(entrada.pino);

    if (leituraAtual != entrada.leituraInstavel) {
      entrada.leituraInstavel = leituraAtual;
      entrada.instanteMudanca = agora;
    }

    if ((agora - entrada.instanteMudanca) >= TEMPO_DEBOUNCE_MS && leituraAtual != entrada.pressionado) {
      tratarMudancaEntrada(entrada, leituraAtual);
    }
  }
}

void tratarMudancaEntrada(EntradaDigital &entrada, bool novoEstadoPressionado) {
  entrada.pressionado = novoEstadoPressionado;

  imprimirEventoEntrada(entrada, novoEstadoPressionado);

  if (novoEstadoPressionado) {
    copiarUltimoAcionamento(entrada.nome);
    beepCurto();
  }

  atualizarMonitorSerial(true);
  atualizarLCD();
}

bool lerPressionado(uint8_t pino) {
  return digitalRead(pino) == LOW;
}

// =====================================================
// Saidas indicadoras
// =====================================================

void atualizarSaidasIndicadoras() {
  digitalWrite(PINO_LED_AZUL, entradas[0].pressionado ? HIGH : LOW);
  digitalWrite(PINO_LED_VERDE, entradas[1].pressionado ? HIGH : LOW);
  digitalWrite(PINO_LED_OK, contarEntradasPressionadas() > 0 ? HIGH : LOW);
}

// =====================================================
// Monitor Serial
// =====================================================

void imprimirCabecalhoSerial() {
  Serial.println(F("========================================"));
  Serial.println(F("Passa ou Repassa - Teste de Conexoes"));
  Serial.println(F("Arduino Mega 2560"));
  Serial.println(F("Entradas com INPUT_PULLUP"));
  Serial.println(F("Monitor Serial: 9600 bps"));
  Serial.println(F("========================================"));
}

void atualizarMonitorSerial(bool forcarImpressao) {
  (void)forcarImpressao;

  Serial.println(F("---------------- STATUS ----------------"));

  for (uint8_t i = 0; i < TOTAL_ENTRADAS; i++) {
    Serial.print(entradas[i].nome);
    Serial.print(F(" | D"));
    Serial.print(entradas[i].pino);
    Serial.print(F(": "));
    Serial.println(textoEstado(entradas[i].pressionado));
  }

  Serial.print(F("Total pressionado: "));
  Serial.println(contarEntradasPressionadas());

  Serial.print(F("Ultimo acionamento: "));
  Serial.println(ultimoAcionamento);

  Serial.println();
}

void imprimirEventoEntrada(const EntradaDigital &entrada, bool pressionado) {
  Serial.print(F("["));
  Serial.print(millis());
  Serial.print(F(" ms] "));
  Serial.print(pressionado ? F("PRESSIONADO: ") : F("SOLTO: "));
  Serial.print(entrada.nome);
  Serial.print(F(" | Pino D"));
  Serial.println(entrada.pino);
}

// =====================================================
// LCD 20x4
// =====================================================

void inicializarLCD() {
  if (enderecoLCD20x4 == 0) {
    Serial.println(F("LCD 20x4 nao encontrado. O teste seguira apenas pelo Monitor Serial."));
    return;
  }

  lcd20x4 = new LiquidCrystal_I2C(enderecoLCD20x4, LCD_COLUNAS, LCD_LINHAS);
  lcd20x4->init();
  lcd20x4->backlight();
  lcd20x4->clear();

  escreverLinhaLCD(0, "Passa ou Repassa");
  escreverLinhaLCD(1, "Teste Conexoes");
  escreverLinhaLCD(2, "LCD I2C detectado");

  char linha[21];
  snprintf(linha, sizeof(linha), "Endereco: 0x%02X", enderecoLCD20x4);
  escreverLinhaLCD(3, linha);

  delay(1000);
}

void atualizarLCD() {
  if (lcd20x4 == NULL) {
    return;
  }

  char linha[21];

  snprintf(linha, sizeof(linha), "Teste Conexoes %02u", contarEntradasPressionadas());
  escreverLinhaLCD(0, linha);

  snprintf(linha, sizeof(linha), "Ult:%-16s", ultimoAcionamento);
  escreverLinhaLCD(1, linha);

  snprintf(
    linha,
    sizeof(linha),
    "Az:%s Vd:%s Rs:%s",
    textoCurto(entradas[0].pressionado),
    textoCurto(entradas[1].pressionado),
    textoCurto(entradas[2].pressionado)
  );
  escreverLinhaLCD(2, linha);

  snprintf(
    linha,
    sizeof(linha),
    "A:%s V:%s +:%s -:%s",
    textoCurto(entradas[3].pressionado),
    textoCurto(entradas[6].pressionado),
    textoCurto(entradas[4].pressionado),
    textoCurto(entradas[5].pressionado)
  );
  escreverLinhaLCD(3, linha);
}

void escreverLinhaLCD(uint8_t linha, const char *texto) {
  if (lcd20x4 == NULL || linha >= LCD_LINHAS) {
    return;
  }

  char buffer[LCD_COLUNAS + 1];
  uint8_t i = 0;

  for (; i < LCD_COLUNAS && texto[i] != '\0'; i++) {
    buffer[i] = texto[i];
  }

  for (; i < LCD_COLUNAS; i++) {
    buffer[i] = ' ';
  }

  buffer[LCD_COLUNAS] = '\0';

  lcd20x4->setCursor(0, linha);
  lcd20x4->print(buffer);
}

// =====================================================
// I2C
// =====================================================

void escanearBarramentoI2C() {
  totalDispositivosI2C = 0;

  Serial.println(F("Escaneando barramento I2C..."));

  for (uint8_t endereco = 1; endereco < 127; endereco++) {
    if (dispositivoI2CResponde(endereco)) {
      if (totalDispositivosI2C < MAX_DISPOSITIVOS_I2C) {
        dispositivosI2C[totalDispositivosI2C] = endereco;
        totalDispositivosI2C++;
      }

      Serial.print(F("Dispositivo I2C encontrado em 0x"));
      if (endereco < 16) {
        Serial.print(F("0"));
      }
      Serial.println(endereco, HEX);
    }
  }

  if (totalDispositivosI2C == 0) {
    Serial.println(F("Nenhum dispositivo I2C encontrado."));
    Serial.println(F("Verifique SDA=20, SCL=21, VCC e GND."));
  }

  Serial.println();
}

bool dispositivoI2CResponde(uint8_t endereco) {
  Wire.beginTransmission(endereco);
  return Wire.endTransmission() == 0;
}

bool enderecoI2CEncontrado(uint8_t endereco) {
  for (uint8_t i = 0; i < totalDispositivosI2C; i++) {
    if (dispositivosI2C[i] == endereco) {
      return true;
    }
  }

  return false;
}

uint8_t selecionarEnderecoLCD() {
  if (enderecoI2CEncontrado(ENDERECO_LCD_PREFERENCIAL)) {
    return ENDERECO_LCD_PREFERENCIAL;
  }

  if (enderecoI2CEncontrado(ENDERECO_LCD_ALTERNATIVO)) {
    return ENDERECO_LCD_ALTERNATIVO;
  }

  return 0;
}

void imprimirResumoI2C() {
  Serial.println(F("--------------- I2C --------------------"));
  Serial.print(F("Dispositivos encontrados: "));
  Serial.println(totalDispositivosI2C);

  Serial.print(F("LCD 20x4: "));
  if (enderecoLCD20x4 == 0) {
    Serial.println(F("NAO IDENTIFICADO"));
  } else {
    Serial.print(F("0x"));
    if (enderecoLCD20x4 < 16) {
      Serial.print(F("0"));
    }
    Serial.println(enderecoLCD20x4, HEX);
  }

  Serial.println(F("Observacao: outros displays I2C aparecem no scanner."));
  Serial.println(F("Para escrever no 12864, confirme o controlador/biblioteca."));
  Serial.println();
}

// =====================================================
// Utilitarios
// =====================================================

void copiarUltimoAcionamento(const char *texto) {
  strncpy(ultimoAcionamento, texto, sizeof(ultimoAcionamento) - 1);
  ultimoAcionamento[sizeof(ultimoAcionamento) - 1] = '\0';
}

void beepCurto() {
  tone(PINO_BUZZER, FREQUENCIA_BEEP_HZ, DURACAO_BEEP_MS);
}

const char *textoEstado(bool pressionado) {
  return pressionado ? "PRESSIONADO" : "SOLTO";
}

const char *textoCurto(bool pressionado) {
  return pressionado ? "ON" : "--";
}

uint8_t contarEntradasPressionadas() {
  uint8_t total = 0;

  for (uint8_t i = 0; i < TOTAL_ENTRADAS; i++) {
    if (entradas[i].pressionado) {
      total++;
    }
  }

  return total;
}
