# AA-SISTEMA V2.0

Reescrita completa da lógica de controle, com correção de bugs estruturais e introdução de máquina de estados explícita.

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
| `DIST_TRAVAR` | 15 cm | Distância máxima para travar no alvo |
| `DIST_LASER` | 10 cm | Distância para disparar o laser |
| `DIST_SONAR` | 200 cm | Alcance máximo do sonar |
| `CICLOS_CONFIRMACAO` | 2 | Leituras consecutivas para confirmar detecção |
| `CICLOS_PARA_PERDER` | 6 | Leituras sem alvo para iniciar estado de perda |

## Máquina de estados

O sistema opera com 4 estados explícitos:

```
PATRULHA ──► CONFIRMANDO ──► TRAVADO ──► PERDENDO ──► PATRULHA
                │                            │
                └─── falso positivo          └─── reencontrou
                     (volta a PATRULHA)           (volta a TRAVADO)
```

### PATRULHA
Servo varre horizontalmente de 2° a 178°. Ao detectar algo dentro de `DIST_TRAVAR`, começa a contar confirmações.

### CONFIRMANDO
Aguarda `CICLOS_CONFIRMACAO` leituras consecutivas com alvo. Se confirmar, vai para TRAVADO. Se falhar, volta a PATRULHA (evita falsos positivos de eco).

### TRAVADO
Servo segue o alvo com controle proporcional. O ângulo alvo é atualizado continuamente com média móvel de 3 amostras para reduzir jitter. Servo vertical sobe proporcionalmente à proximidade. Laser dispara quando distância ≤ `DIST_LASER`.

### PERDENDO
Alvo saiu do sonar. O sistema faz re-scan em leque (±30°) ao redor da última posição conhecida antes de desistir. Se reencontrar o alvo, volta direto para TRAVADO sem precisar reconfirmar.

## Funcionamento

1. O sistema inicia em **PATRULHA**, varrendo de 2° a 178°
2. Ao detectar objeto ≤ 15 cm por 2 leituras consecutivas, entra em **CONFIRMANDO**
3. Confirmado, entra em **TRAVADO**: servo segue o alvo, buzzer soa com frequência crescente conforme aproximação
4. Objeto ≤ 10 cm: **laser dispara** e mensagem é enviada ao Serial Monitor
5. Ao perder o alvo por 6 ciclos, entra em **PERDENDO** e realiza re-scan
6. Se não reencontrar após 24 ciclos sem alvo, volta a **PATRULHA**

## Melhorias em relação à V1.1

### Bugs corrigidos

- **Variável `static` dentro do `loop()`** — `ciclosSemAlvo` era `static` local, o que fazia o contador acumular entre detecções diferentes. Agora é variável global com reset explícito em cada transição de estado.
- **`anguloAlvo` congelava no primeiro frame** — na V1.1, o ângulo alvo só era salvo na transição de detecção, nunca durante o tracking. Se o alvo se movesse, o servo ficava parado. Agora é atualizado a cada ciclo enquanto o alvo está visível.
- **`pulseIn` retornando 0 entrava na média** — leituras sem eco geravam `dist = 0` que poluía a suavização EMA e puxava `ultimaDist` para baixo. Agora `medirDistancia()` retorna `-1.0` para leituras inválidas e a EMA só acumula com valores positivos.
- **`anguloX` sem limites** — o servo podia receber valores fora de 0°–180° em determinadas condições. Agora todo ponto de movimento usa `constrain()`.

### Robustez

- **Loop totalmente não-bloqueante** — `delay()` eliminado. Todo o tempo é gerenciado com `millis()` e três timers independentes: sonar a cada 50 ms, servo a cada 20 ms, Serial a cada 200 ms.
- **Serial em 115200 baud** — menos tempo de bloqueio por caractere transmitido.
- **Timeout do `pulseIn` calculado dinamicamente** — baseado em `DIST_SONAR`, não hardcoded.
- **Filtragem por amostra** — cada leitura do sonar é validada individualmente (faixa 2 cm–200 cm) antes de entrar na EMA.

### Lógica

- **Máquina de estados explícita** — substituiu os `bool travado`, `bool alvoDetectado` e contadores espalhados por 4 estados bem definidos com transições claras.
- **Re-scan inteligente** — ao entrar em PERDENDO, o sistema varre em leque progressivo (±30°) ao redor da última posição conhecida do alvo antes de voltar à varredura completa.
- **Média móvel de ângulo** — histórico de 3 ângulos suaviza o tracking e reduz o jitter do servo horizontal.
- **Recuperação rápida** — ao reencontrar o alvo no estado PERDENDO, vai direto para TRAVADO sem passar por CONFIRMANDO.

## Serial Monitor

Configurar para **115200 baud**. Output a cada 200 ms:

```
Estado:PATRULHA AX:92 AY:90 Alvo:90 Dist:0.0cm
Estado:CONFIRMANDO AX:87 AY:90 Alvo:87 Dist:13.4cm
Estado:TRAVADO AX:87 AY:102 Alvo:87 Dist:11.2cm
>>> LASER DISPARADO <<<
Estado:TRAVADO AX:87 AY:108 Alvo:87 Dist:8.7cm
```

## Status

✔ Funcional  
✔ Testado em simulação
