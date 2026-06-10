# Arquitetura de Hardware

## 1. Controlador principal

O projeto utilizará o **Arduino Mega 2560** como controlador principal.

A escolha do Mega 2560 é adequada porque ele possui grande quantidade de entradas e saídas digitais, facilitando a expansão do projeto com botões, LEDs, displays, buzzers, relés e sensores.

## 2. Blocos principais

```text
[Botões das Equipes] --->
                         
[Botões do Mediador] ---> [Arduino Mega 2560] ---> [LEDs Indicadores]
                                             ---> [Buzzer]
                                             ---> [Placar Futuro]
                                             ---> [Relés / Efeitos Futuros]
```

## 3. Entradas previstas

- Botão da Equipe Azul.
- Botão da Equipe Verde.
- Botão de reset da rodada.
- Botão de ponto para Equipe Azul, em etapa futura.
- Botão de ponto para Equipe Verde, em etapa futura.
- Botão de penalidade, em etapa futura.

## 4. Saídas previstas

- LED indicador da Equipe Azul.
- LED indicador da Equipe Verde.
- Buzzer de sinalização.
- LEDs de status do sistema.
- Display ou placar digital, em etapa futura.
- Relé de efeito externo, em etapa futura.

## 5. Boas práticas de montagem

- Usar resistores adequados nos LEDs.
- Usar botões com ligação em `INPUT_PULLUP`, sempre que possível.
- Evitar deixar entradas digitais flutuando.
- Separar alimentação lógica de cargas externas.
- Usar módulo de relé com optoacoplamento quando houver cargas externas.
- Identificar todos os fios e bornes.
- Prever caixa ou painel de proteção para uso em sala de aula.

## 6. Observação de segurança

Caso o projeto acione sirene, lâmpada, motor, solenóide ou qualquer carga de maior potência, o circuito de potência deverá ser isolado do Arduino por módulo apropriado, com fonte dedicada e proteção elétrica adequada.