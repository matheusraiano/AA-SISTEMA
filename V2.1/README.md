# AA-SISTEMA V2.1

Introdução de controle PID no eixo horizontal, substituindo o controle proporcional simples da V2.0.

## Hardware utilizado

- Arduino Uno
- 2x Servo Motor SG90
- Sensor Ultrassônico HC-SR04
- Módulo Laser
- Buzzer Piezo

## Pinagem

| Componente | Pino Arduino |
|---|---|
| HC-SR04 Trig | 7 |
| HC-SR04 Echo | 8 |
| Laser | 6 |
| Buzzer | 5 |
| Servo horizontal (X) | 9 |
| Servo vertical (Y) | 10 |

## Parâmetros principais

| Constante | Valor | Descrição |
|---|---|---|
| `DIST_TRAVAR` | 20 cm | Distância máxima para travar no alvo |
| `DIST_LASER` | 15 cm | Distância para disparar o laser |
| `DIST_SONAR` | 200 cm | Alcance máximo do sonar |
| `CICLOS_CONFIRMACAO` | 2 | Leituras consecutivas para confirmar detecção |
| `CICLOS_PARA_PERDER` | 6 | Leituras sem alvo para iniciar estado de perda |

## Ganhos PID

| Constante | Valor | Descrição |
|---|---|---|
| `KP` | 0.2 | Ganho proporcional |
| `KI` | 0.07 | Ganho integral |
| `KD` | 0.8 | Ganho derivativo |
| `INTEGRAL_MAX` | 20.0 | Limite anti-windup |
| `SAIDA_MAX` | 8.0 | Passo máximo por ciclo (graus) |

## Máquina de estados

```
PATRULHA ──► CONFIRMANDO ──► TRAVADO ──► PERDENDO ──► PATRULHA
                │                            │
                └─── falso positivo          └─── reencontrou
                     (volta a PATRULHA)           (volta a TRAVADO)
```

### PATRULHA
Servo varre horizontalmente de 2° a 178°. Ao detectar algo dentro de `DIST_TRAVAR`, começa a contar confirmações.

### CONFIRMANDO
Aguarda `CICLOS_CONFIRMACAO` leituras consecutivas com alvo. Se confirmar, vai para TRAVADO. Se falhar, volta a PATRULHA.

### TRAVADO
PID controla o servo X para seguir o alvo. Ângulo atualizado continuamente com média móvel de 3 amostras. Servo vertical sobe proporcionalmente à proximidade. Laser dispara quando distância ≤ `DIST_LASER`. PID é resetado em toda transição de estado.

### PERDENDO
Alvo saiu do sonar. Re-scan em leque (±30°) ao redor da última posição conhecida. Se reencontrar, volta direto para TRAVADO.

## Funcionamento

1. O sistema inicia em **PATRULHA**, varrendo de 2° a 178°
2. Ao detectar objeto ≤ 20 cm por 2 leituras consecutivas, entra em **CONFIRMANDO**
3. Confirmado, entra em **TRAVADO**: PID controla o servo X, buzzer soa com frequência crescente conforme aproximação
4. Objeto ≤ 15 cm: **laser dispara** e mensagem é enviada ao Serial Monitor
5. Ao perder o alvo por 6 ciclos, entra em **PERDENDO** e realiza re-scan
6. Se não reencontrar após 24 ciclos sem alvo, volta a **PATRULHA**

## Melhorias em relação à V2.0

- **Controle PID no eixo X** — substitui o controle proporcional simples. Elimina oscilação e para exatamente no alvo.
- **Anti-windup** — limita o acúmulo do termo integral (`INTEGRAL_MAX`) para evitar comportamento explosivo quando o servo atinge os limites físicos.
- **Reset do PID em toda transição de estado** — integral e derivativo não contaminam a próxima detecção.
- **`dt` calculado com `millis()`** — tempo real entre ciclos, não estimado. Garante precisão do PID independente da carga do loop.

## Serial Monitor

Configurar para **115200 baud**. Output a cada 200 ms:

```
Estado:PATRULHA AX:92 AY:90 Alvo:90 Dist:0.0cm I:0.00
Estado:CONFIRMANDO AX:87 AY:90 Alvo:87 Dist:18.4cm I:0.00
Estado:TRAVADO AX:87 AY:98 Alvo:87 Dist:16.2cm I:0.14
>>> LASER DISPARADO <<<
Estado:TRAVADO AX:87 AY:104 Alvo:87 Dist:12.7cm I:0.21
```

O campo `I:` mostra o acúmulo do termo integral em tempo real — útil para diagnosticar se o PID está travando antes do alvo.

## Status

✔ Funcional  
✔ Testado em hardware
