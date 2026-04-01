# AA-SISTEMA

Projeto experimental de um sistema antiaéreo automatizado utilizando Arduino.

O sistema utiliza um sensor ultrassônico montado em servos para realizar varredura do ambiente e detectar objetos. Quando um alvo é identificado, o sistema realiza tracking automático, aponta um laser para a posição detectada e emite aviso sonoro contínuo de proximidade.

## Tecnologias utilizadas

- Arduino
- C++
- Servo Motors
- Sensor Ultrassônico HC-SR04
- Laser Module
- Buzzer

## Estrutura do projeto

```
AA-SISTEMA/
├── V1.0/
│   └── arduino/AA-SISTEMA-V1.0/
│   └── docs/
│   └── tinkercad/
│   └── README
├── V1.1/
│   ├── arduino/AA-SISTEMA-V1.1/
│   └── docs/
│   └── tinkercad/
│   └── README
├── V2.0/
│   └── arduino/AA-SISTEMA-V2.0/
│   └── README
├── V2.1/
│   └── arduino/AA-SISTEMA-V2.1/
│   └── README
└
```

## Versões

| Versão | Descrição |
|--------|-----------|
| V1.0 | Primeira versão funcional |
| V1.1 | Tracking mais rápido, buzzer contínuo, laser com Serial |
| V2.0 | Reescrita completa: máquina de estados, loop não-bloqueante, bugs corrigidos |
| V2.1 | Controle PID no eixo X, eliminação do tremor do servo |

## Objetivo do projeto

Estudo de sistemas de detecção, automação e controle utilizando Arduino.
