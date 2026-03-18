#include <Servo.h>

/* ===== HARDWARE ===== */
Servo servoX;
Servo servoY;

/* ===== PINOS ===== */
const int trig = 7;
const int echo = 8;
const int laserPin = 6;
const int buzzer = 5;

/* ===== CONTROLE ===== */
int anguloX = 90;       // posição servo horizontal
int servoYPos = 90;     // posição servo vertical
int ultimaDist = 0;
bool alvoDetectado = false;

/* ===== CONFIGURAÇÃO ===== */
const int limiteDist = 30;   // alcance máximo do sonar (cm)
const float suavizacao = 0.7;

/* ===== RADAR ===== */
const int centroRadarX = 90;  // ponto central horizontal
const int passoMaxX = 6;      // passo máximo horizontal por ciclo
const int passoMaxY = 4;      // passo máximo vertical por ciclo
int direcaoX = 1;             // direção da patrulha horizontal

/* ===== GANHOS PROPORCIONAIS ===== */
const float kP_X = 0.5;  // ganho proporcional horizontal
const float kP_Y = 0.4;  // ganho proporcional vertical

/* ===== SENSOR ===== */
long medirDistancia(){
  long soma = 0;
  for(int i=0;i<2;i++){
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    long duracao = pulseIn(echo, HIGH, 20000);
    soma += duracao * 0.034 / 2;
    delay(2);
  }
  return soma / 2;
}

/* ===== BUZZER ===== */
void avisoDistancia(int dist){
  if(dist <= limiteDist){
    int freq = map(dist, limiteDist, 0, 700, 2500);
    int duracao = map(dist, limiteDist, 0, 200, 30);
    tone(buzzer, freq, duracao);
    delay(duracao);
  } else {
    noTone(buzzer);
  }
}

/* ===== SETUP ===== */
void setup(){
  servoX.attach(9);
  servoY.attach(10);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(laserPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(laserPin, LOW);
  Serial.begin(9600);
}

/* ===== LOOP ===== */
void loop(){
  // Medição do sonar
  int dist = medirDistancia();
  ultimaDist = (ultimaDist * suavizacao) + (dist * (1 - suavizacao));

  avisoDistancia(ultimaDist);

  // detectar alvo mais próximo
  if(ultimaDist > 0 && ultimaDist <= limiteDist){
    alvoDetectado = true;
  } else {
    alvoDetectado = false;
  }

  if(alvoDetectado){
    // --- TRACKING PROPORCIONAL AO SONAR ---
    // Horizontal: centralizar alvo
    int erroX = centroRadarX - anguloX;
    int movimentoX = kP_X * erroX;
    if(movimentoX > passoMaxX) movimentoX = passoMaxX;
    if(movimentoX < -passoMaxX) movimentoX = -passoMaxX;
    anguloX += movimentoX;

    // Vertical: proporcional à distância (apenas sobe se alvo mais próximo)
    int alturaAlvo = map(ultimaDist, 0, limiteDist, 120, 60);
    int erroY = alturaAlvo - servoYPos;
    if(erroY > 0){
      int movimentoY = kP_Y * erroY;
      if(movimentoY > passoMaxY) movimentoY = passoMaxY;
      servoYPos += movimentoY;
    }

    servoX.write(anguloX);
    servoY.write(servoYPos);

    // --- ATAQUE LASER ---
    if(ultimaDist <= 8){
      digitalWrite(laserPin, HIGH);
      delay(40);
      digitalWrite(laserPin, LOW);
      delay(40);
      Serial.println("Laser disparado!");
    }

  } else {
    // patrulha horizontal suave
    anguloX += passoMaxX * direcaoX;
    if(anguloX >= 180) direcaoX = -1;
    if(anguloX <= 0) direcaoX = 1;

    servoX.write(anguloX);
    servoY.write(90);
  }

  // Serial
  Serial.print("AnguloX:"); Serial.print(anguloX);
  Serial.print(", AnguloY:"); Serial.print(servoYPos);
  Serial.print(", Distancia:"); Serial.print(ultimaDist);
  Serial.print(", AlvoDetectado:"); Serial.println(alvoDetectado);

  delay(10);
}