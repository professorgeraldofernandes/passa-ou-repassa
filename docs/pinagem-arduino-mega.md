# Pinagem — Arduino Mega 2560

## 1. Pinagem inicial proposta

| Função | Pino Arduino Mega | Tipo | Observação |
|---|---:|---|---|
| Buzzer | 8 | Saída digital/PWM | Buzzer passivo ou ativo |
| LCD I2C 20x4 — SDA | 20 | Comunicação I2C | Pino SDA nativo do Arduino Mega 2560 |
| LCD I2C 20x4 — SCL | 21 | Comunicação I2C | Pino SCL nativo do Arduino Mega 2560 |
| Botão Equipe Azul | 22 | Entrada digital | Usar `INPUT_PULLUP` |
| Botão Equipe Verde | 23 | Entrada digital | Usar `INPUT_PULLUP` |
| Botão Reset Rodada | 24 | Entrada digital | Usar `INPUT_PULLUP` |
| Chave Seletora A — Pontuação Azul | 25 | Entrada digital | Ativa quando a chave estiver na posição A |
| Botão + Pontos | 26 | Entrada digital | Soma 1 ponto na equipe selecionada pela chave A/V |
| Botão - Pontos | 27 | Entrada digital | Subtrai 1 ponto da equipe selecionada pela chave A/V |
| Chave Seletora V — Pontuação Verde | 28 | Entrada digital | Ativa quando a chave estiver na posição V |
| LED Equipe Azul | 30 | Saída digital | Usar resistor limitador |
| LED Equipe Verde | 31 | Saída digital | Usar resistor limitador |
| LED Sistema Pronto | 32 | Saída digital | Indicação de sistema liberado |
| Display 12864B V2 — RST | 49 | Saída digital | Reset do display gráfico ST7920 |
| Display 12864B V2 — RW/SID | 51 | Comunicação serial/SPI por software | Dados seriais do display ST7920 |
| Display 12864B V2 — E/SCLK | 52 | Comunicação serial/SPI por software | Clock serial do display ST7920 |
| Display 12864B V2 — RS/CS | 53 | Comunicação serial/SPI por software | Chip select do display ST7920 |

## 2. Lógica dos botões

Será utilizada a configuração `INPUT_PULLUP`.

Nesse modo:

- Botão solto = nível lógico `HIGH`.
- Botão pressionado = nível lógico `LOW`.

Essa estratégia reduz a quantidade de componentes externos e evita entradas flutuantes.

## 3. Lógica da chave seletora A/V

A chave seletora define qual equipe será alterada pelos botões de pontuação:

- Posição `A`: os botões `+ Pontos` e `- Pontos` alteram a pontuação da Equipe Azul.
- Posição `V`: os botões `+ Pontos` e `- Pontos` alteram a pontuação da Equipe Verde.

Para uma leitura mais segura, serão utilizadas duas entradas digitais:

| Entrada | Pino | Condição ativa | Equipe selecionada |
|---|---:|---|---|
| Chave posição A | 25 | `LOW` | Azul |
| Chave posição V | 28 | `LOW` | Verde |

## 4. Validação da chave seletora

O firmware deverá considerar a seleção válida apenas quando uma única posição estiver ativa.

| Pino A | Pino V | Interpretação |
|---|---|---|
| `LOW` | `HIGH` | Equipe Azul selecionada |
| `HIGH` | `LOW` | Equipe Verde selecionada |
| `HIGH` | `HIGH` | Nenhuma equipe selecionada ou falha de ligação |
| `LOW` | `LOW` | Erro de ligação ou acionamento simultâneo indevido |

## 5. Display LCD I2C 20x4

O LCD I2C 20x4 utiliza o barramento I2C nativo do Arduino Mega 2560:

| Sinal LCD I2C | Pino Arduino Mega |
|---|---:|
| SDA | 20 |
| SCL | 21 |
| VCC | 5V |
| GND | GND |

Endereços I2C comuns:

- `0x27`
- `0x3F`

O sketch de teste possui scanner I2C simples para detectar automaticamente o endereço do LCD.

## 6. Display 12864B V2 — ST7920

O display gráfico 12864B V2 será testado no modo serial com a biblioteca `U8g2`.

| Sinal Display 12864B V2 | Pino Arduino Mega |
|---|---:|
| E / SCLK | 52 |
| RW / SID | 51 |
| RS / CS | 53 |
| RST | 49 |
| VCC | 5V |
| GND | GND |
| PSB | GND para modo serial |

> Observação: se o display não apresentar imagem, verificar o ajuste de contraste, o pino PSB, a alimentação e a ordem dos sinais.

## 7. Observações técnicas

- Os pinos podem ser alterados conforme o layout físico da montagem.
- Evitar usar pinos 0 e 1, pois são utilizados pela comunicação serial USB.
- Reservar pinos de comunicação I2C para o LCD 20x4.
- Reservar pinos de comunicação SPI ou software SPI para o display 12864B V2.
- Conferir a chave seletora com multímetro antes da ligação definitiva.
- A chave seletora pode exigir dois blocos de contato, um para a posição A e outro para a posição V.

## 8. Expansões futuras sugeridas

| Recurso futuro | Pinos sugeridos | Observação |
|---|---|---|
| Cartão SD | A definir | Evitar conflito com o display 12864 se usar SPI |
| Placar 7 segmentos | A definir | Pode usar CI MAX7219 |
| Relé de efeito | 40 | Acionamento externo isolado |