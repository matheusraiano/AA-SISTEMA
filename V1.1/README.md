# AA-SISTEMA V1.1

Segunda versão funcional do projeto, com melhorias significativas em relação à V1.0.

## Hardware utilizado

- Arduino Uno
- 2 Servo Motors
- Sensor Ultrassônico HC-SR04
- Laser Module
- Buzzer

## Novidades e melhorias em relação à V1.0

1. **Tracking mais rápido**
   - O servo horizontal agora se movimenta mais rapidamente tanto no patrulhamento quanto durante o tracking do alvo.
   - O passo de movimento do servo foi aumentado, mantendo a predição de movimento.

2. **Limite de tracking ajustado**
   - O sistema agora rastreia alvos apenas até **150 cm**, tornando o radar mais preciso para alvos próximos.

3. **Laser com Serial**
   - O laser dispara apenas para alvos a **≤90 cm**.
   - Toda vez que o laser dispara, uma mensagem `"Laser disparado!"` é exibida no Serial Monitor.

4. **Buzzer contínuo**
   - Substituímos os bipes discretos por um som contínuo.
   - Quanto mais próximo o alvo, mais agudo e rápido fica o som, permitindo indicação de proximidade em tempo real.

5. **Multi-alvo**
   - O sistema registra múltiplos alvos detectados durante a varredura, mantendo um histórico das posições.

6. **Predição aprimorada**
   - Média móvel dos últimos ângulos detectados para suavizar o tracking.
   - Ajuda a manter o laser e o servo apontando corretamente para alvos em movimento.

## Funcionamento

O sensor ultrassônico realiza medições enquanto o servo horizontal varre o ambiente.

Quando um objeto é detectado dentro do limite definido (≤150 cm), o sistema:

1. Confirma a detecção  
2. Realiza tracking do alvo com predição de movimento  
3. Ajusta a altura da mira  
4. Aponta o laser para o alvo se estiver ≤90 cm, mostrando a mensagem no Serial  
5. Emite aviso sonoro contínuo com frequência e ritmo variando conforme a distância  

## Status

✔ Funcional  
✔ Testado em simulação