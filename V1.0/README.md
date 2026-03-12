# AA-SISTEMA V1.0

Primeira versão funcional do projeto.

## Hardware utilizado

- Arduino Uno
- 2 Servo Motors
- Sensor Ultrassônico HC-SR04
- Laser Module

## Funcionamento

O sensor ultrassônico realiza medições enquanto o servo horizontal varre o ambiente.

Quando um objeto é detectado dentro do limite definido, o sistema:

1. Confirma a detecção
2. Realiza tracking do alvo
3. Ajusta a altura da mira
4. Aponta o laser para o alvo

## Arquivo principal

arduino/aa_sistema_v1.ino


## Status

✔ Funcional  
✔ Testado em simulação  