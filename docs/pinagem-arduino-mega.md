# Pinagem — Arduino Mega 2560

## 1. Pinagem inicial proposta

| Função | Pino Arduino Mega | Tipo | Observação |
|---|---:|---|---|
| Buzzer | 8 | Saída digital/PWM | Buzzer passivo ou ativo |
| Barramento I2C — SDA | 20 | Comunicação I2C | Usado pelo LCD 20x4 e pelo Display 12864B V2 com módulo I2C |
| Barramento I2C — SCL | 21 | Comunicação I2C | Usado pelo LCD 20x4 e pelo Display 12864B V2 com módulo I2C |
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

## 5. Barramento I2C

O Arduino Mega 2560 possui pinos I2C dedicados:

| Sinal I2C | Pino Arduino Mega |
|---|---:|
| SDA | 20 |
| SCL | 21 |
| VCC dos módulos | 5V |
| GND dos módulos | GND |

Os dois displays I2C devem ser ligados em paralelo no mesmo barramento:

```text
Arduino Mega SDA 20 ---- SDA LCD 20x4 ---- SDA Display 12864B V2
Arduino Mega SCL 21 ---- SCL LCD 20x4 ---- SCL Display 12864B V2
Arduino Mega 5V     ---- VCC LCD 20x4 ---- VCC Display 12864B V2
Arduino Mega GND    ---- GND LCD 20x4 ---- GND Display 12864B V2
```

Cada display precisa ter um endereço I2C diferente. Endereços comuns em módulos I2C:

- `0x27`
- `0x3F`
- `0x20` a `0x27`, quando usa expansor PCF8574/PCF8574A

O sketch de teste possui scanner I2C para listar todos os dispositivos encontrados.

## 6. LCD I2C 20x4

| Sinal LCD I2C | Pino Arduino Mega |
|---|---:|
| SDA | 20 |
| SCL | 21 |
| VCC | 5V |
| GND | GND |

## 7. Display 12864B V2 com módulo I2C

| Sinal Display 12864B V2 I2C | Pino Arduino Mega |
|---|---:|
| SDA | 20 |
| SCL | 21 |
| VCC | 5V |
| GND | GND |

> Observação: como existem módulos 12864B V2 I2C com controladores/adaptadores diferentes, o teste inicial validará a presença do display no barramento I2C. Para escrever gráficos/textos no display 12864, será necessário confirmar o controlador ou a biblioteca específica do módulo.

## 8. Observações técnicas

- Os pinos podem ser alterados conforme o layout físico da montagem.
- Evitar usar pinos 0 e 1, pois são utilizados pela comunicação serial USB.
- Os pinos 20 e 21 ficam reservados para o barramento I2C.
- Não usar os pinos 20 e 21 como entradas ou saídas comuns enquanto os displays estiverem conectados.
- Conferir a chave seletora com multímetro antes da ligação definitiva.
- A chave seletora pode exigir dois blocos de contato, um para a posição A e outro para a posição V.

## 9. Expansões futuras sugeridas

| Recurso futuro | Pinos sugeridos | Observação |
|---|---|---|
| Cartão SD | 50, 51, 52, 53 | Barramento SPI nativo do Arduino Mega |
| Placar 7 segmentos | A definir | Pode usar CI MAX7219 |
| Relé de efeito | 40 | Acionamento externo isolado |