/*
  Projeto: Passa ou Repassa
  Sketch: Teste de Conexoes
  Plataforma: Arduino Mega 2560

  Objetivo:
  Testar as entradas e saidas ja mapeadas no projeto usando:
  - Monitor Serial
  - Display LCD I2C 20x4
  - Display Grafico 12864B V2 com controlador ST7920

  Bibliotecas necessarias:
  - Wire
  - LiquidCrystal_I2C
  - U8g2

  Observacao:
  O Display 12864B V2 deve estar configurado em modo serial.
  Em muitos modulos ST7920, isso exige ligar o pino PSB ao GND.
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <U8g2lib.h>

// =========================
// Pinos das entradas
// =========================

constexpr uint8_t PINO_BOTAO_AZUL = 22;
constexpr uint8_t PINO_BOTAO_VERDE = 23;
constexpr uint8_t PINO_BOTAO_RESET = 24;

constexpr uint8_t PINO_CHAVE_PONTUACAO_A = 25;
constexpr uint8_t PINO_BOTAO_MAIS_PONTOS = 26;
constexpr uint8_t PINO_BOTAO_MENOS_PONTOS = 27;
constexpr uint8_t PINO_CHAVE_PONTUACAO_V = 28;

// =========================
// Pinos das saidas
// =========================

constexpr uint8_t PINO_LED_AZUL = 30;
constexpr uint8_t PINO_LED_VERDE = 31;
constexpr uint8_t PINO_LED_PRONTO = 32;
constexpr uint8_t PINO_BUZZER = 8;

// =========================
// Display LCD I2C 20x4
// Arduino Mega 2560: SDA = 20, SCL = 21
// =========================

LiquidCrystal_I2C *lcd20x4 = NULL;
uint8_t enderecoLCD20x4 = 0;

// =========================
// Display 12864B V2 - ST7920 em modo serial
// Ligações sugeridas:
// E/SCLK  -> pino 52
// RW/SID  -> pino 51
// RS/CS   -> pino 53
// RST     -> pino 49
// PSB     -> GND, para modo serial
// =========================

constexpr uint8_t PINO_12864_E_CLK = 52;
constexpr uint8_t PINO_12864_RW_DAT = 51;
constexpr uint8_t PINO_12864_RS_CS = 53;
constexpr uint8_t PINO_12864_RST = 49;

U8G2_ST7920_128X64_1_SW_SPI display12864(
  U8G2_R0,
  PINO_12864_E_CLK,
  PINO_12864_RW_DAT,
  PINO_12864_RS_CS,
  PINO_12864_RST
);

// =========================
// Configurações gerais
// =========================

constexpr unsigned long INTERVALO_ATUALIZACAO_MS = 500;
constexpr unsigned int FREQUENCIA_BUZZER_HZ = 1800;
constexpr unsigned long TEMPO_BUZZER_MS = 80;

unsigned long ultimaAtualizacao = 0;

// =========================
// Estruturas auxiliares
// =========================

struct EstadoEntradas {
  bool botaoAzul;
  bool botaoVerde;
  bool botaoReset;
  bool chaveA;
  bool chaveV;
  bool botaoMais;
  bool botaoMenos;
};

enum class EstadoChaveAV : uint8_t {
  Azul,
  Verde,
  Nenhuma,
  Erro
};

EstadoEntradas entradas;

// =========================
// Prototipos
// =========================

void configurarPinos();
void testarSaidasIniciais();
void inicializarLCD20x4();
void inicializarDisplay12864();
void atualizarEntradas();
void atualizarSaidasDeTeste();
void atualizarMonitorSerial();
void atualizarLCD20x4();
void atualizarDisplay12864();
void escreverLinhaLCD(uint8_t linha, const String &texto);
bool entradaAtiva(uint8_t pino);
uint8_t detectarEnderecoLCD20x4();
bool dispositivoI2CResponde(uint8_t endereco);
EstadoChaveAV lerEstadoChaveAV();
const char *textoOnOff(bool estado);
const char *textoChaveAV(EstadoChaveAV estado);
void beepCurto();

// =========================
// Setup
// =========================

void setup() {
  Serial.begin(9600);
  delay(300);

  Serial.println(F("========================================"));
  Serial.println(F("Passa ou Repassa - Teste de Conexoes"));
  Serial.println(F("Arduino Mega 2560"));
  Serial.println(F("========================================"));

  configurarPinos();
  Wire.begin();

  inicializarLCD20x4();
  inicializarDisplay12864();
  testarSaidasIniciais();

  Serial.println(F("Sistema de teste iniciado."));
  Serial.println(F("Acione cada botao/chave e confira o Serial e os displays."));
}

// =========================
// Loop principal
// =========================

void loop() {
  atualizarEntradas();
  atualizarSaidasDeTeste();

  const unsigned long agora = millis();

  if (agora - ultimaAtualizacao >= INTERVALO_ATUALIZACAO_MS) {
    ultimaAtualizacao = agora;

    atualizarMonitorSerial();
    atualizarLCD20x4();
    atualizarDisplay12864();
  }
}

// =========================
// Configuracao dos pinos
// =========================

void configurarPinos() {
  pinMode(PINO_BOTAO_AZUL, INPUT_PULLUP);
  pinMode(PINO_BOTAO_VERDE, INPUT_PULLUP);
  pinMode(PINO_BOTAO_RESET, INPUT_PULLUP);
  pinMode(PINO_CHAVE_PONTUACAO_A, INPUT_PULLUP);
  pinMode(PINO_CHAVE_PONTUACAO_V, INPUT_PULLUP);
  pinMode(PINO_BOTAO_MAIS_PONTOS, INPUT_PULLUP);
  pinMode(PINO_BOTAO_MENOS_PONTOS, INPUT_PULLUP);

  pinMode(PINO_LED_AZUL, OUTPUT);
  pinMode(PINO_LED_VERDE, OUTPUT);
  pinMode(PINO_LED_PRONTO, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);

  digitalWrite(PINO_LED_AZUL, LOW);
  digitalWrite(PINO_LED_VERDE, LOW);
  digitalWrite(PINO_LED_PRONTO, LOW);
  noTone(PINO_BUZZER);
}

// =========================
// Teste inicial das saidas
// =========================

void testarSaidasIniciais() {
  Serial.println(F("Testando LEDs e buzzer..."));

  digitalWrite(PINO_LED_AZUL, HIGH);
  beepCurto();
  delay(300);
  digitalWrite(PINO_LED_AZUL, LOW);

  digitalWrite(PINO_LED_VERDE, HIGH);
  beepCurto();
  delay(300);
  digitalWrite(PINO_LED_VERDE, LOW);

  digitalWrite(PINO_LED_PRONTO, HIGH);
  beepCurto();
  delay(300);
  digitalWrite(PINO_LED_PRONTO, LOW);
}

// =========================
// Inicializacao dos displays
// =========================

void inicializarLCD20x4() {
  enderecoLCD20x4 = detectarEnderecoLCD20x4();

  if (enderecoLCD20x4 == 0) {
    Serial.println(F("LCD I2C 20x4 nao encontrado no barramento I2C."));
    Serial.println(F("Verifique SDA=20, SCL=21, VCC, GND e endereco I2C."));
    return;
  }

  lcd20x4 = new LiquidCrystal_I2C(enderecoLCD20x4, 20, 4);
  lcd20x4->init();
  lcd20x4->backlight();
  lcd20x4->clear();

  escreverLinhaLCD(0, "Passa ou Repassa");
  escreverLinhaLCD(1, "Teste Conexoes");
  escreverLinhaLCD(2, "LCD I2C OK");
  escreverLinhaLCD(3, "Endereco: 0x" + String(enderecoLCD20x4, HEX));

  Serial.print(F("LCD I2C encontrado no endereco 0x"));
  Serial.println(enderecoLCD20x4, HEX);
}

void inicializarDisplay12864() {
  display12864.begin();
  display12864.setFont(u8g2_font_6x10_tf);

  display12864.firstPage();
  do {
    display12864.drawStr(0, 10, "Passa ou Repassa");
    display12864.drawStr(0, 24, "Teste Conexoes");
    display12864.drawStr(0, 38, "Display 12864B V2");
    display12864.drawStr(0, 52, "ST7920 Serial");
  } while (display12864.nextPage());

  Serial.println(F("Display 12864 inicializado. Se nao houver imagem, verificar contraste, PSB e pinos."));
}

// =========================
// Atualizacao das entradas
// =========================

void atualizarEntradas() {
  entradas.botaoAzul = entradaAtiva(PINO_BOTAO_AZUL);
  entradas.botaoVerde = entradaAtiva(PINO_BOTAO_VERDE);
  entradas.botaoReset = entradaAtiva(PINO_BOTAO_RESET);
  entradas.chaveA = entradaAtiva(PINO_CHAVE_PONTUACAO_A);
  entradas.chaveV = entradaAtiva(PINO_CHAVE_PONTUACAO_V);
  entradas.botaoMais = entradaAtiva(PINO_BOTAO_MAIS_PONTOS);
  entradas.botaoMenos = entradaAtiva(PINO_BOTAO_MENOS_PONTOS);
}

bool entradaAtiva(uint8_t pino) {
  return digitalRead(pino) == LOW;
}

// =========================
// Saidas dinamicas de teste
// =========================

void atualizarSaidasDeTeste() {
  const EstadoChaveAV estadoChave = lerEstadoChaveAV();

  // LED azul acompanha o botao da equipe azul.
  digitalWrite(PINO_LED_AZUL, entradas.botaoAzul ? HIGH : LOW);

  // LED verde acompanha o botao da equipe verde.
  digitalWrite(PINO_LED_VERDE, entradas.botaoVerde ? HIGH : LOW);

  // LED pronto acende quando a chave A/V esta em estado valido.
  const bool chaveValida = estadoChave == EstadoChaveAV::Azul || estadoChave == EstadoChaveAV::Verde;
  digitalWrite(PINO_LED_PRONTO, chaveValida ? HIGH : LOW);

  // Buzzer soa enquanto + ou - pontos estiver pressionado.
  if (entradas.botaoMais || entradas.botaoMenos) {
    tone(PINO_BUZZER, FREQUENCIA_BUZZER_HZ);
  } else {
    noTone(PINO_BUZZER);
  }
}

// =========================
// Monitor Serial
// =========================

void atualizarMonitorSerial() {
  const EstadoChaveAV estadoChave = lerEstadoChaveAV();

  Serial.println(F("----------------------------------------"));
  Serial.print(F("Botao Azul: "));
  Serial.println(textoOnOff(entradas.botaoAzul));

  Serial.print(F("Botao Verde: "));
  Serial.println(textoOnOff(entradas.botaoVerde));

  Serial.print(F("Botao Reset: "));
  Serial.println(textoOnOff(entradas.botaoReset));

  Serial.print(F("Chave A: "));
  Serial.print(textoOnOff(entradas.chaveA));
  Serial.print(F(" | Chave V: "));
  Serial.print(textoOnOff(entradas.chaveV));
  Serial.print(F(" | Selecao: "));
  Serial.println(textoChaveAV(estadoChave));

  Serial.print(F("Botao + Pontos: "));
  Serial.println(textoOnOff(entradas.botaoMais));

  Serial.print(F("Botao - Pontos: "));
  Serial.println(textoOnOff(entradas.botaoMenos));

  Serial.print(F("LCD I2C: "));
  if (enderecoLCD20x4 == 0) {
    Serial.println(F("NAO ENCONTRADO"));
  } else {
    Serial.print(F("0x"));
    Serial.println(enderecoLCD20x4, HEX);
  }
}

// =========================
// LCD I2C 20x4
// =========================

void atualizarLCD20x4() {
  if (lcd20x4 == NULL) {
    return;
  }

  const EstadoChaveAV estadoChave = lerEstadoChaveAV();

  escreverLinhaLCD(0, "Teste de Conexoes");
  escreverLinhaLCD(1, "Az:" + String(textoOnOff(entradas.botaoAzul)) + " Vd:" + String(textoOnOff(entradas.botaoVerde)));
  escreverLinhaLCD(2, "R:" + String(textoOnOff(entradas.botaoReset)) + " +:" + String(textoOnOff(entradas.botaoMais)) + " -:" + String(textoOnOff(entradas.botaoMenos)));
  escreverLinhaLCD(3, "Chave: " + String(textoChaveAV(estadoChave)));
}

void escreverLinhaLCD(uint8_t linha, const String &texto) {
  if (lcd20x4 == NULL || linha > 3) {
    return;
  }

  String textoAjustado = texto;

  if (textoAjustado.length() > 20) {
    textoAjustado = textoAjustado.substring(0, 20);
  }

  while (textoAjustado.length() < 20) {
    textoAjustado += ' ';
  }

  lcd20x4->setCursor(0, linha);
  lcd20x4->print(textoAjustado);
}

// =========================
// Display grafico 12864
// =========================

void atualizarDisplay12864() {
  const EstadoChaveAV estadoChave = lerEstadoChaveAV();

  display12864.firstPage();
  do {
    display12864.setFont(u8g2_font_6x10_tf);
    display12864.drawStr(0, 10, "Teste Conexoes");

    display12864.setCursor(0, 22);
    display12864.print(F("Az:"));
    display12864.print(textoOnOff(entradas.botaoAzul));
    display12864.print(F(" Vd:"));
    display12864.print(textoOnOff(entradas.botaoVerde));

    display12864.setCursor(0, 34);
    display12864.print(F("R:"));
    display12864.print(textoOnOff(entradas.botaoReset));
    display12864.print(F(" +:"));
    display12864.print(textoOnOff(entradas.botaoMais));
    display12864.print(F(" -:"));
    display12864.print(textoOnOff(entradas.botaoMenos));

    display12864.setCursor(0, 46);
    display12864.print(F("Chave: "));
    display12864.print(textoChaveAV(estadoChave));

    display12864.setCursor(0, 58);
    display12864.print(F("I2C: "));
    if (enderecoLCD20x4 == 0) {
      display12864.print(F("N/D"));
    } else {
      display12864.print(F("0x"));
      display12864.print(enderecoLCD20x4, HEX);
    }
  } while (display12864.nextPage());
}

// =========================
// Scanner I2C simples
// =========================

uint8_t detectarEnderecoLCD20x4() {
  const uint8_t enderecosPreferenciais[] = {0x27, 0x3F};

  for (uint8_t i = 0; i < 2; i++) {
    if (dispositivoI2CResponde(enderecosPreferenciais[i])) {
      return enderecosPreferenciais[i];
    }
  }

  for (uint8_t endereco = 1; endereco < 127; endereco++) {
    if (dispositivoI2CResponde(endereco)) {
      return endereco;
    }
  }

  return 0;
}

bool dispositivoI2CResponde(uint8_t endereco) {
  Wire.beginTransmission(endereco);
  return Wire.endTransmission() == 0;
}

// =========================
// Tratamento da chave A/V
// =========================

EstadoChaveAV lerEstadoChaveAV() {
  if (entradas.chaveA && !entradas.chaveV) {
    return EstadoChaveAV::Azul;
  }

  if (!entradas.chaveA && entradas.chaveV) {
    return EstadoChaveAV::Verde;
  }

  if (!entradas.chaveA && !entradas.chaveV) {
    return EstadoChaveAV::Nenhuma;
  }

  return EstadoChaveAV::Erro;
}

const char *textoOnOff(bool estado) {
  return estado ? "ON" : "OFF";
}

const char *textoChaveAV(EstadoChaveAV estado) {
  switch (estado) {
    case EstadoChaveAV::Azul:
      return "A/Azul";
    case EstadoChaveAV::Verde:
      return "V/Verde";
    case EstadoChaveAV::Nenhuma:
      return "Nenhuma";
    case EstadoChaveAV::Erro:
      return "Erro";
    default:
      return "Indef.";
  }
}

// =========================
// Buzzer
// =========================

void beepCurto() {
  tone(PINO_BUZZER, FREQUENCIA_BUZZER_HZ, TEMPO_BUZZER_MS);
  delay(TEMPO_BUZZER_MS + 40);
  noTone(PINO_BUZZER);
}
