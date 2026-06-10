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

### Ligação inicial recomendada

```text
Pino 25 do Arduino ---- Contato comum da chave seletora
Contato posição A ----- GND
Contato posição V ----- Sem ligação direta, usando pull-up interno
```

Com essa ligação:

| Estado elétrico no pino 25 | Posição considerada pelo firmware |
|---|---|
| `LOW` | A — Equipe Azul |
| `HIGH` | V — Equipe Verde |

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
| LED azul | 1 | Indicador da Equipe Azul |
| LED verde | 1 | Indicador da Equipe Verde |
| LED de sistema pronto | 1 | Indicação de rodada liberada |
| Resistor 220 ohms | 3 | Para os LEDs |
| Buzzer | 1 | Sinalização sonora |
| Protoboard ou placa de montagem | 1 | Montagem inicial |
| Jumpers | Diversos | Ligações elétricas |