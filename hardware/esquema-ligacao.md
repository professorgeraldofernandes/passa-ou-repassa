# Esquema de Ligação — Versão Inicial

## 1. Ligações dos botões

Os botões serão ligados utilizando a configuração `INPUT_PULLUP` do Arduino.

### Ligação recomendada

```text
Pino digital do Arduino ---- Botão ---- GND
```

Quando o botão estiver solto, o Arduino lerá `HIGH`.  
Quando o botão for pressionado, o Arduino lerá `LOW`.

## 2. Ligação da chave seletora A/V

A chave seletora de duas posições será utilizada para selecionar qual equipe receberá ajuste de pontuação.

Para uma leitura mais segura, cada posição da chave deverá ser ligada a uma entrada digital própria do Arduino.

### Ligação recomendada

```text
Pino 25 do Arduino ---- Contato da posição A ---- GND
Pino 28 do Arduino ---- Contato da posição V ---- GND
```

Com `INPUT_PULLUP`, a leitura será:

| Estado elétrico | Interpretação |
|---|---|
| Pino 25 = `LOW` e pino 28 = `HIGH` | A — Equipe Azul |
| Pino 25 = `HIGH` e pino 28 = `LOW` | V — Equipe Verde |
| Pino 25 = `HIGH` e pino 28 = `HIGH` | Nenhuma posição detectada ou falha de ligação |
| Pino 25 = `LOW` e pino 28 = `LOW` | Erro de ligação ou acionamento simultâneo indevido |

> Observação: antes da ligação definitiva, conferir os terminais da chave com multímetro, pois a disposição física dos contatos pode variar conforme o fabricante.

## 3. Ligações dos LEDs

Cada LED deverá possuir resistor limitador de corrente.

### Ligação recomendada

```text
Pino digital do Arduino ---- Resistor 220 ohms ---- Anodo do LED
Catodo do LED ------------------------------- GND
```

## 4. Ligação do buzzer

Para buzzer de baixa corrente:

```text
Pino digital/PWM do Arduino ---- Terminal positivo do buzzer
GND ---------------------------- Terminal negativo do buzzer
```

Para buzzer, sirene ou carga de maior corrente, utilizar transistor, relé ou módulo apropriado.

## 5. Alimentação

- Arduino Mega 2560 via USB ou fonte adequada.
- Evitar alimentar cargas externas diretamente pelos pinos do Arduino.
- Usar fonte externa para módulos de maior consumo.
- Interligar GND das fontes quando houver comunicação entre os circuitos.

## 6. Lista inicial de componentes

| Item | Quantidade | Observação |
|---|---:|---|
| Arduino Mega 2560 | 1 | Controlador principal |
| Botão tipo push button | 5 | Azul, verde, reset, + pontos e - pontos |
| Chave seletora 2 posições | 1 | Seleção A/V da equipe para pontuação |
| Bloco de contato NA para chave seletora | 2 | Um contato para A e outro para V |
| LED azul | 1 | Indicador da Equipe Azul |
| LED verde | 1 | Indicador da Equipe Verde |
| LED de sistema pronto | 1 | Indicação de rodada liberada |
| Resistor 220 ohms | 3 | Para os LEDs |
| Buzzer | 1 | Sinalização sonora |
| Protoboard ou placa de montagem | 1 | Montagem inicial |
| Jumpers | Diversos | Ligações elétricas |