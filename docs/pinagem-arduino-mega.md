# Pinagem — Arduino Mega 2560

## 1. Pinagem inicial proposta

| Função | Pino Arduino Mega | Tipo | Observação |
|---|---:|---|---|
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
| Buzzer | 8 | Saída digital/PWM | Buzzer passivo ou ativo |

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

## 5. Observações técnicas

- Os pinos podem ser alterados conforme o layout físico da montagem.
- Evitar usar pinos 0 e 1, pois são utilizados pela comunicação serial USB.
- Reservar pinos PWM para recursos futuros, como efeitos sonoros ou iluminação.
- Reservar pinos de comunicação SPI/I2C para displays, módulos SD ou expansões futuras.
- Conferir a chave seletora com multímetro antes da ligação definitiva.
- A chave seletora pode exigir dois blocos de contato, um para a posição A e outro para a posição V.

## 6. Expansões futuras sugeridas

| Recurso futuro | Pinos sugeridos | Observação |
|---|---|---|
| Display I2C | 20 SDA / 21 SCL | LCD ou OLED |
| Cartão SD | 50, 51, 52, 53 | Barramento SPI |
| Placar 7 segmentos | A definir | Pode usar CI MAX7219 |
| Relé de efeito | 40 | Acionamento externo isolado |