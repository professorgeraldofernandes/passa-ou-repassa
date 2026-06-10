# Bibliotecas do Projeto

Este documento registra as bibliotecas utilizadas no projeto **Passa ou Repassa**.

## 1. LCD I2C 20x4

Para o display **LCD I2C 20x4**, a biblioteca validada no projeto foi:

| Item | Informação |
|---|---|
| Nome na Arduino IDE | `LiquidCrystal I2C` |
| Autor exibido na IDE | Frank de Brabander |
| Versão testada | `1.1.2` |
| Repositório | `johnrickman/LiquidCrystal_I2C` |
| Link | `https://github.com/johnrickman/LiquidCrystal_I2C` |

## 2. Instalação pela Arduino IDE

1. Abrir a Arduino IDE.
2. Acessar `Sketch > Incluir Biblioteca > Gerenciar Bibliotecas...`.
3. Pesquisar por `LiquidCrystal I2C`.
4. Selecionar a biblioteca de **Frank de Brabander**.
5. Instalar a versão `1.1.2`.

## 3. Uso no código

```cpp
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
```

Exemplo de criação do objeto:

```cpp
LiquidCrystal_I2C lcd(0x27, 20, 4);
```

Inicialização recomendada para essa biblioteca:

```cpp
lcd.init();
lcd.backlight();
lcd.clear();
```

## 4. Observação sobre o barramento I2C

O LCD I2C 20x4 e o Display 12864B V2 com módulo I2C compartilham o mesmo barramento do Arduino Mega 2560:

| Sinal | Pino Arduino Mega |
|---|---:|
| SDA | 20 |
| SCL | 21 |

Cada dispositivo conectado ao barramento I2C deve possuir endereço diferente.