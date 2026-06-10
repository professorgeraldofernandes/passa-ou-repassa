# Esquema de Ligação — Versão Inicial

## 1. Ligações dos botões

Os botões serão ligados utilizando a configuração `INPUT_PULLUP` do Arduino.

### Ligação recomendada

```text
Pino digital do Arduino ---- Botão ---- GND
```

Quando o botão estiver solto, o Arduino lerá `HIGH`.  
Quando o botão for pressionado, o Arduino lerá `LOW`.

## 2. Ligações dos LEDs

Cada LED deverá possuir resistor limitador de corrente.

### Ligação recomendada

```text
Pino digital do Arduino ---- Resistor 220 ohms ---- Anodo do LED
Catodo do LED ------------------------------- GND
```

## 3. Ligação do buzzer

Para buzzer de baixa corrente:

```text
Pino digital/PWM do Arduino ---- Terminal positivo do buzzer
GND ---------------------------- Terminal negativo do buzzer
```

Para buzzer, sirene ou carga de maior corrente, utilizar transistor, relé ou módulo apropriado.

## 4. Alimentação

- Arduino Mega 2560 via USB ou fonte adequada.
- Evitar alimentar cargas externas diretamente pelos pinos do Arduino.
- Usar fonte externa para módulos de maior consumo.
- Interligar GND das fontes quando houver comunicação entre os circuitos.

## 5. Lista inicial de componentes

| Item | Quantidade | Observação |
|---|---:|---|
| Arduino Mega 2560 | 1 | Controlador principal |
| Botão tipo push button | 3 | Azul, verde e reset |
| LED azul | 1 | Indicador da Equipe Azul |
| LED verde | 1 | Indicador da Equipe Verde |
| LED de sistema pronto | 1 | Indicação de rodada liberada |
| Resistor 220 ohms | 3 | Para os LEDs |
| Buzzer | 1 | Sinalização sonora |
| Protoboard ou placa de montagem | 1 | Montagem inicial |
| Jumpers | Diversos | Ligações elétricas |