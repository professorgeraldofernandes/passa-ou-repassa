/*
  Projeto: Passa ou Repassa
  Plataforma: Arduino Mega 2560
  Linguagem: C++ / Arduino

  Descrição:
  Versão inicial do controle eletrônico do jogo Passa ou Repassa.
  Esta versão identifica qual equipe pressionou primeiro o botão,
  aciona o LED correspondente, emite sinal sonoro e permite ajustar
  a pontuação por meio de uma chave seletora A/V com botões de + e -.

  Autor: Geraldo Fernandes
*/

// =========================
// Mapeamento de pinos
// =========================

constexpr uint8_t PINO_BOTAO_AZUL = 22;
constexpr uint8_t PINO_BOTAO_VERDE = 23;
constexpr uint8_t PINO_BOTAO_RESET = 24;

constexpr uint8_t PINO_CHAVE_PONTUACAO_AV = 25;
constexpr uint8_t PINO_BOTAO_MAIS_PONTOS = 26;
constexpr uint8_t PINO_BOTAO_MENOS_PONTOS = 27;

constexpr uint8_t PINO_LED_AZUL = 30;
constexpr uint8_t PINO_LED_VERDE = 31;
constexpr uint8_t PINO_LED_PRONTO = 32;

constexpr uint8_t PINO_BUZZER = 8;

// =========================
// Configurações gerais
// =========================

constexpr unsigned long TEMPO_DEBOUNCE_MS = 50;
constexpr unsigned int FREQUENCIA_BUZZER_HZ = 2000;
constexpr unsigned long TEMPO_BUZZER_MS = 150;

constexpr int PONTUACAO_MINIMA = 0;
constexpr int PONTUACAO_MAXIMA = 999;

// =========================
// Estados do jogo
// =========================

enum class EquipeSelecionada : uint8_t {
  Nenhuma,
  Azul,
  Verde
};

EquipeSelecionada equipeSelecionada = EquipeSelecionada::Nenhuma;

int pontuacaoAzul = 0;
int pontuacaoVerde = 0;

unsigned long ultimoAcionamentoAzul = 0;
unsigned long ultimoAcionamentoVerde = 0;
unsigned long ultimoAcionamentoReset = 0;
unsigned long ultimoAcionamentoMaisPontos = 0;
unsigned long ultimoAcionamentoMenosPontos = 0;

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
void atualizarPontuacao();
void alterarPontuacao(int variacao);
EquipeSelecionada lerEquipeSelecionadaParaPontuacao();
void exibirPontuacaoSerial();

// =========================
// Setup
// =========================

void setup() {
  Serial.begin(9600);

  configurarPinos();
  reiniciarRodada();
  exibirPontuacaoSerial();
}

// =========================
// Loop principal
// =========================

void loop() {
  atualizarJogo();
  atualizarPontuacao();
}

// =========================
// Configuração dos pinos
// =========================

void configurarPinos() {
  pinMode(PINO_BOTAO_AZUL, INPUT_PULLUP);
  pinMode(PINO_BOTAO_VERDE, INPUT_PULLUP);
  pinMode(PINO_BOTAO_RESET, INPUT_PULLUP);

  pinMode(PINO_CHAVE_PONTUACAO_AV, INPUT_PULLUP);
  pinMode(PINO_BOTAO_MAIS_PONTOS, INPUT_PULLUP);
  pinMode(PINO_BOTAO_MENOS_PONTOS, INPUT_PULLUP);

  pinMode(PINO_LED_AZUL, OUTPUT);
  pinMode(PINO_LED_VERDE, OUTPUT);
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

  if (botaoPressionado(PINO_BOTAO_VERDE, ultimoAcionamentoVerde)) {
    selecionarEquipe(EquipeSelecionada::Verde);
    return;
  }
}

// =========================
// Controle de pontuação
// =========================

void atualizarPontuacao() {
  if (botaoPressionado(PINO_BOTAO_MAIS_PONTOS, ultimoAcionamentoMaisPontos)) {
    alterarPontuacao(+1);
    return;
  }

  if (botaoPressionado(PINO_BOTAO_MENOS_PONTOS, ultimoAcionamentoMenosPontos)) {
    alterarPontuacao(-1);
    return;
  }
}

void alterarPontuacao(int variacao) {
  const EquipeSelecionada equipePontuacao = lerEquipeSelecionadaParaPontuacao();

  if (equipePontuacao == EquipeSelecionada::Azul) {
    pontuacaoAzul += variacao;
    pontuacaoAzul = constrain(pontuacaoAzul, PONTUACAO_MINIMA, PONTUACAO_MAXIMA);
  } else if (equipePontuacao == EquipeSelecionada::Verde) {
    pontuacaoVerde += variacao;
    pontuacaoVerde = constrain(pontuacaoVerde, PONTUACAO_MINIMA, PONTUACAO_MAXIMA);
  }

  sinalSonoro();
  exibirPontuacaoSerial();
}

EquipeSelecionada lerEquipeSelecionadaParaPontuacao() {
  // Com INPUT_PULLUP:
  // LOW  = posição A, Equipe Azul
  // HIGH = posição V, Equipe Verde
  return digitalRead(PINO_CHAVE_PONTUACAO_AV) == LOW
           ? EquipeSelecionada::Azul
           : EquipeSelecionada::Verde;
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
  digitalWrite(PINO_LED_VERDE, equipeSelecionada == EquipeSelecionada::Verde ? HIGH : LOW);
}

// =========================
// Exibição auxiliar via Serial
// =========================

void exibirPontuacaoSerial() {
  Serial.print("Pontuacao Azul: ");
  Serial.print(pontuacaoAzul);
  Serial.print(" | Pontuacao Verde: ");
  Serial.println(pontuacaoVerde);
}
