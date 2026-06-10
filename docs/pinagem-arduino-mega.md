# Pinagem — Arduino Mega 2560

## 1. Pinagem inicial proposta

| Função | Pino Arduino Mega | Tipo | Observação |
|---|---:|---|---|
| Botão Equipe Azul | 22 | Entrada digital | Usar `INPUT_PULLUP` |
| Botão Equipe Verde | 23 | Entrada digital | Usar `INPUT_PULLUP` |
| Botão Reset Rodada | 24 | Entrada digital | Usar `INPUT_PULLUP` |
| Chave Seletora A/V Pontuação | 25 | Entrada digital | Seleciona qual equipe recebe ajuste de pontos |
| Botão + Pontos | 26 | Entrada digital | Soma 1 ponto na equipe selecionada pela chave A/V |
| Botão - Pontos | 27 | Entrada digital | Subtrai 1 ponto da equipe selecionada pela chave A/V |
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

Na versão inicial do firmware, será adotada a seguinte leitura:

| Leitura no pino 25 | Equipe selecionada |
|---|---|
| `LOW` | Azul |
| `HIGH` | Verde |

Caso a chave fique invertida na montagem física, basta inverter a lógica no firmware ou alterar a ligação do contato utilizado.

## 4. Observações técnicas

- Os pinos podem ser alterados conforme o layout físico da montagem.
- Evitar usar pinos 0 e 1, pois são utilizados pela comunicação serial USB.
- Reservar pinos PWM para recursos futuros, como efeitos sonoros ou iluminação.
- Reservar pinos de comunicação SPI/I2C para displays, módulos SD ou expansões futuras.
- Conferir a chave seletora com multímetro antes da ligação definitiva.

## 5. Expansões futuras sugeridas

| Recurso futuro | Pinos sugeridos | Observação |
|---|---|---|
| Display I2C | 20 SDA / 21 SCL | LCD ou OLED |
| Cartão SD | 50, 51, 52, 53 | Barramento SPI |
| Placar 7 segmentos | A definir | Pode usar CI MAX7219 |
| Relé de efeito | 40 | Acionamento externo isolado |