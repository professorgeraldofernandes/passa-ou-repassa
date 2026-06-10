/*
  Projeto: Passa ou Repassa
  Sketch: Teste de Conexoes
  Plataforma: Arduino Mega 2560

  Objetivo:
  Testar as entradas, saidas e dispositivos I2C ja mapeados no projeto usando:
  - Monitor Serial
  - Display LCD I2C 20x4
  - Display 12864B V2 com modulo I2C

  Bibliotecas necessarias:
  - Wire
  - LiquidCrystal_I2C

  Observacao importante:
  O LCD 20x4 e o Display 12864B V2 devem compartilhar o mesmo barramento I2C:
  - SDA = pino 20 do Arduino Mega 2560
  - SCL = pino 21 do Arduino Mega 2560

  Para que dois dispositivos I2C funcionem no mesmo barramento, eles precisam ter
  enderecos I2C diferentes. Caso os dois estejam no mesmo endereco, havera conflito.
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =========================
// Configuracao dos enderecos I2C
// =========================

// Enderecos comuns para LCD I2C 20x4: 0x27 ou 0x3F.
// Ajuste manualmente se o scanner indicar outro endereco.
constexpr uint8_t ENDERECO_LCD20X4_PREFERENCIAL = 0x27;

// Ajuste conforme o endereco encontrado pelo scanner I2C.
// Muitos modulos I2C 12864 podem aparecer como 0x3C, 0x3D, 0x3F ou outro endereco.
constexpr uint8_t ENDERECO_DISPLAY_12864_PREFERENCIAL = 0x3F;

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
// Barramento I2C - Arduino Mega 2560
// =========================

constexpr uint8_t PINO_I2C_SDA = 20;
constexpr uint8_t PINO_I2C_SCL = 21;

// =========================
// Display LCD I2C 20x4
// =========================

LiquidCrystal_I2C *lcd20x4 = NULL;
uint8_t enderecoLCD20x4 = 0;
uint8_t enderecoDisplay12864 = 0;

// =========================
// Configuracoes gerais
// =========================

constexpr unsigned long INTERVALO_ATUALIZACAO_MS = 500;
constexpr unsigned int FREQUENCIA_BUZZER_HZ = 1800;
constexpr unsigned long TEMPO_BUZZER_MS = 80;
constexpr uint8_t MAX_DISPOSITIVOS_I2C = 12;

unsigned long ultimaAtualizacao = 0;
uint8_t dispositivosI2C[MAX_DISPOSITIVOS_I2C];
uint8_t totalDispositivosI2C = 0;

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
void inicializarDispositivosI2C();
void inicializarLCD20x4();
void atualizarEntradas();
void atualizarSaidasDeTeste();
void atualizarMonitorSerial();
void atualizarLCD20x4();
void escreverLinhaLCD(uint8_t linha, const String &texto);
bool entradaAtiva(uint8_t pino);
void escanearBarramentoI2C();
bool dispositivoI2CResponde(uint8_t endereco);
bool enderecoFoiEncontrado(uint8_t endereco);
uint8_t detectarEnderecoLCD20x4();
uint8_t detectarEnderecoDisplay12864();
EstadoChaveAV lerEstadoChaveAV();
const char *textoOnOff(bool estado);
const char *textoChaveAV(EstadoChaveAV estado);
void imprimirEnderecoI2C(uint8_t endereco);
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
  Serial.println(F("Displays no mesmo barramento I2C"));
  Serial.println(F("SDA=20 | SCL=21"));
  Serial.println(F("========================================"));

  configurarPinos();
  Wire.begin();

  inicializarDispositivosI2C();
  inicializarLCD20x4();
  testarSaidasIniciais();

  Serial.println(F("Sistema de teste iniciado."));
  Serial.println(F("Acione cada botao/chave e confira o Serial e o LCD 20x4."));
  Serial.println(F("O Display 12864B V2 sera validado como dispositivo I2C no barramento."));
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
// Inicializacao I2C
// =========================

void inicializarDispositivosI2C() {
  escanearBarramentoI2C();

  enderecoLCD20x4 = detectarEnderecoLCD20x4();
  enderecoDisplay12864 = detectarEnderecoDisplay12864();

  if (totalDispositivosI2C == 0) {
    Serial.println(F("Nenhum dispositivo I2C encontrado. Verifique SDA, SCL, VCC e GND."));
    return;
  }

  if (enderecoLCD20x4 == 0) {
    Serial.println(F("LCD I2C 20x4 nao identificado automaticamente."));
  }

  if (enderecoDisplay12864 == 0) {
    Serial.println(F("Display 12864B V2 I2C nao identificado automaticamente."));
  }

  if (enderecoLCD20x4 != 0 && enderecoDisplay12864 != 0 && enderecoLCD20x4 == enderecoDisplay12864) {
    Serial.println(F("ATENCAO: LCD 20x4 e Display 12864 parecem estar no mesmo endereco I2C."));
    Serial.println(F("Dois dispositivos I2C no mesmo barramento precisam de enderecos diferentes."));
  }
}

void escanearBarramentoI2C() {
  totalDispositivosI2C = 0;

  Serial.println(F("Escaneando barramento I2C..."));

  for (uint8_t endereco = 1; endereco < 127; endereco++) {
    if (dispositivoI2CResponde(endereco)) {
      if (totalDispositivosI2C < MAX_DISPOSITIVOS_I2C) {
        dispositivosI2C[totalDispositivosI2C] = endereco;
        totalDispositivosI2C++;
      }

      Serial.print(F("Dispositivo I2C encontrado em "));
      imprimirEnderecoI2C(endereco);
      Serial.println();
    }
  }

  Serial.print(F("Total de dispositivos I2C encontrados: "));
  Serial.println(totalDispositivosI2C);
}

bool dispositivoI2CResponde(uint8_t endereco) {
  Wire.beginTransmission(endereco);
  return Wire.endTransmission() == 0;
}

bool enderecoFoiEncontrado(uint8_t endereco) {
  for (uint8_t i = 0; i < totalDispositivosI2C; i++) {
    if (dispositivosI2C[i] == endereco) {
      return true;
    }
  }

  return false;
}

uint8_t detectarEnderecoLCD20x4() {
  if (enderecoFoiEncontrado(ENDERECO_LCD20X4_PREFERENCIAL)) {
    return ENDERECO_LCD20X4_PREFERENCIAL;
  }

  const uint8_t enderecosComunsLCD[] = {0x27, 0x3F};

  for (uint8_t i = 0; i < 2; i++) {
    if (enderecoFoiEncontrado(enderecosComunsLCD[i])) {
      return enderecosComunsLCD[i];
    }
  }

  return 0;
}

uint8_t detectarEnderecoDisplay12864() {
  if (enderecoFoiEncontrado(ENDERECO_DISPLAY_12864_PREFERENCIAL)) {
    return ENDERECO_DISPLAY_12864_PREFERENCIAL;
  }

  // Caso o endereco preferencial nao seja encontrado, assume o primeiro endereco
  // I2C detectado que seja diferente do LCD 20x4.
  for (uint8_t i = 0; i < totalDispositivosI2C; i++) {
    if (dispositivosI2C[i] != enderecoLCD20x4) {
      return dispositivosI2C[i];
    }
  }

  return 0;
}

void inicializarLCD20x4() {
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
  escreverLinhaLCD(2, "LCD: 0x" + String(enderecoLCD20x4, HEX));

  if (enderecoDisplay12864 == 0) {
    escreverLinhaLCD(3, "12864: Nao detect.");
  } else {
    escreverLinhaLCD(3, "12864: 0x" + String(enderecoDisplay12864, HEX));
  }

  Serial.print(F("LCD I2C 20x4 selecionado no endereco "));
  imprimirEnderecoI2C(enderecoLCD20x4);
  Serial.println();
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

  digitalWrite(PINO_LED_AZUL, entradas.botaoAzul ? HIGH : LOW);
  digitalWrite(PINO_LED_VERDE, entradas.botaoVerde ? HIGH : LOW);

  const bool chaveValida = estadoChave == EstadoChaveAV::Azul || estadoChave == EstadoChaveAV::Verde;
  digitalWrite(PINO_LED_PRONTO, chaveValida ? HIGH : LOW);

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

  Serial.print(F("LCD 20x4 I2C: "));
  if (enderecoLCD20x4 == 0) {
    Serial.println(F("NAO ENCONTRADO"));
  } else {
    imprimirEnderecoI2C(enderecoLCD20x4);
    Serial.println();
  }

  Serial.print(F("Display 12864B V2 I2C: "));
  if (enderecoDisplay12864 == 0) {
    Serial.println(F("NAO ENCONTRADO"));
  } else {
    imprimirEnderecoI2C(enderecoDisplay12864);
    Serial.println(F(" - CONEXAO I2C OK"));
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

  escreverLinhaLCD(0, "Az:" + String(textoOnOff(entradas.botaoAzul)) + " Vd:" + String(textoOnOff(entradas.botaoVerde)) + " Rs:" + String(textoOnOff(entradas.botaoReset)));
  escreverLinhaLCD(1, "+:" + String(textoOnOff(entradas.botaoMais)) + " -:" + String(textoOnOff(entradas.botaoMenos)) + " Ch:" + String(textoChaveAV(estadoChave)));
  escreverLinhaLCD(2, "LCD20x4: 0x" + String(enderecoLCD20x4, HEX));

  if (enderecoDisplay12864 == 0) {
    escreverLinhaLCD(3, "12864 I2C: N/D");
  } else {
    escreverLinhaLCD(3, "12864 I2C: 0x" + String(enderecoDisplay12864, HEX));
  }
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

void imprimirEnderecoI2C(uint8_t endereco) {
  Serial.print(F("0x"));
  if (endereco < 16) {
    Serial.print(F("0"));
  }
  Serial.print(endereco, HEX);
}

// =========================
// Buzzer
// =========================

void beepCurto() {
  tone(PINO_BUZZER, FREQUENCIA_BUZZER_HZ, TEMPO_BUZZER_MS);
  delay(TEMPO_BUZZER_MS + 40);
  noTone(PINO_BUZZER);
}
