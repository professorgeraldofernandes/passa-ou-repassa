/*
  Projeto: Passa ou Repassa
  Plataforma: Arduino Mega 2560
  Linguagem: C++ / Arduino

  Descrição:
  Versão inicial do controle eletrônico do jogo Passa ou Repassa.
  Esta versão identifica qual equipe pressionou primeiro o botão,
  aciona o LED correspondente e emite um sinal sonoro.

  Autor: Geraldo Fernandes
*/

// =========================
// Mapeamento de pinos
// =========================

constexpr uint8_t PINO_BOTAO_AZUL = 22;
constexpr uint8_t PINO_BOTAO_VERMELHO = 23;
constexpr uint8_t PINO_BOTAO_RESET = 24;

constexpr uint8_t PINO_LED_AZUL = 30;
constexpr uint8_t PINO_LED_VERMELHO = 31;
constexpr uint8_t PINO_LED_PRONTO = 32;

constexpr uint8_t PINO_BUZZER = 8;

// =========================
// Configurações gerais
// =========================

constexpr unsigned long TEMPO_DEBOUNCE_MS = 50;
constexpr unsigned int FREQUENCIA_BUZZER_HZ = 2000;
constexpr unsigned long TEMPO_BUZZER_MS = 150;

// =========================
// Estados do jogo
// =========================

enum class EquipeSelecionada : uint8_t {
  Nenhuma,
  Azul,
  Vermelha
};

EquipeSelecionada equipeSelecionada = EquipeSelecionada::Nenhuma;

unsigned long ultimoAcionamentoAzul = 0;
unsigned long ultimoAcionamentoVermelho = 0;
unsigned long ultimoAcionamentoReset = 0;

// =========================
// Protótipos das funções
// =========================

void configurarPinos();
void atualizarJogo();
void selecionarEquipe(EquipeSelecionada equipe);
void reiniciarRodada();
bool botaoPressionado(uint8_t pino, unsigned long &ultimoAcionamento);
void sinalSonoro();
void atualizarIndicadores();

// =========================
// Setup
// =========================

void setup() {
  configurarPinos();
  reiniciarRodada();
}

// =========================
// Loop principal
// =========================

void loop() {
  atualizarJogo();
}

// =========================
// Configuração dos pinos
// =========================

void configurarPinos() {
  pinMode(PINO_BOTAO_AZUL, INPUT_PULLUP);
  pinMode(PINO_BOTAO_VERMELHO, INPUT_PULLUP);
  pinMode(PINO_BOTAO_RESET, INPUT_PULLUP);

  pinMode(PINO_LED_AZUL, OUTPUT);
  pinMode(PINO_LED_VERMELHO, OUTPUT);
  pinMode(PINO_LED_PRONTO, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);

  noTone(PINO_BUZZER);
}

// =========================
// Controle principal do jogo
// =========================

void atualizarJogo() {
  if (botaoPressionado(PINO_BOTAO_RESET, ultimoAcionamentoReset)) {
    reiniciarRodada();
    return;
  }

  // Se uma equipe já foi selecionada, a rodada fica travada
  // até que o mediador pressione o botão de reset.
  if (equipeSelecionada != EquipeSelecionada::Nenhuma) {
    return;
  }

  if (botaoPressionado(PINO_BOTAO_AZUL, ultimoAcionamentoAzul)) {
    selecionarEquipe(EquipeSelecionada::Azul);
    return;
  }

  if (botaoPressionado(PINO_BOTAO_VERMELHO, ultimoAcionamentoVermelho)) {
    selecionarEquipe(EquipeSelecionada::Vermelha);
    return;
  }
}

// =========================
// Seleção da equipe
// =========================

void selecionarEquipe(EquipeSelecionada equipe) {
  equipeSelecionada = equipe;
  atualizarIndicadores();
  sinalSonoro();
}

// =========================
// Reinício da rodada
// =========================

void reiniciarRodada() {
  equipeSelecionada = EquipeSelecionada::Nenhuma;
  atualizarIndicadores();
  sinalSonoro();
}

// =========================
// Leitura com debounce simples
// =========================

bool botaoPressionado(uint8_t pino, unsigned long &ultimoAcionamento) {
  const bool pressionado = digitalRead(pino) == LOW;
  const unsigned long agora = millis();

  if (pressionado && (agora - ultimoAcionamento >= TEMPO_DEBOUNCE_MS)) {
    ultimoAcionamento = agora;

    // Aguarda o botão ser solto para evitar múltiplas leituras na mesma pressão.
    while (digitalRead(pino) == LOW) {
      delay(1);
    }

    return true;
  }

  return false;
}

// =========================
// Sinalização sonora
// =========================

void sinalSonoro() {
  tone(PINO_BUZZER, FREQUENCIA_BUZZER_HZ, TEMPO_BUZZER_MS);
}

// =========================
// Atualização dos LEDs
// =========================

void atualizarIndicadores() {
  const bool rodadaLivre = equipeSelecionada == EquipeSelecionada::Nenhuma;

  digitalWrite(PINO_LED_PRONTO, rodadaLivre ? HIGH : LOW);
  digitalWrite(PINO_LED_AZUL, equipeSelecionada == EquipeSelecionada::Azul ? HIGH : LOW);
  digitalWrite(PINO_LED_VERMELHO, equipeSelecionada == EquipeSelecionada::Vermelha ? HIGH : LOW);
}
