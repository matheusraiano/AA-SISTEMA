#include <Servo.h>

/* ===== HARDWARE ===== */

Servo servoX;
Servo servoY;

/* Pinos */
const int trig = 7;
const int echo = 8;
const int laserPin = 6;

/* Controle de movimento */
int angulo = 90;
int direcao = 1;

int ultimaAngulo = 90;
int ultimaDist = 0;

int alvoDetectado = 0;

/* Configuração */
const int limiteDist = 100;
const float suavizacao = 0.7;
const float fatorPredicao = 0.5;

/* ===== SENSOR ===== */

long medirDistancia(){

  long soma = 0;

  for(int i=0;i<3;i++){

    digitalWrite(trig, LOW);
    delayMicroseconds(2);

    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    long duracao = pulseIn(echo, HIGH, 20000);

    soma += duracao * 0.034 / 2;

    delay(5);
  }

  return soma / 3;
}

/* ===== SETUP ===== */

void setup(){

  servoX.attach(9);
  servoY.attach(10);

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(laserPin, OUTPUT);

  Serial.begin(9600);
}

/* ===== LOOP PRINCIPAL ===== */

void loop(){

  digitalWrite(laserPin, HIGH); // laser sempre ligado

  int dist = medirDistancia();

  /* Filtro suavização */
  ultimaDist = (ultimaDist * suavizacao) + (dist * (1 - suavizacao));

  /* ===== DETECÇÃO ===== */

  if(ultimaDist > 0 && ultimaDist < limiteDist){

    int confirm = 0;

    for(int i=0;i<4;i++){
      if(medirDistancia() < limiteDist){
        confirm++;
      }
      delay(10);
    }

    if(confirm >= 3){
      alvoDetectado = 1;
    }
  }

  /* ===== TRACKING ===== */

  if(alvoDetectado){

    int tendencia = angulo - ultimaAngulo;
    int anguloPredito = angulo + tendencia * fatorPredicao;

    ultimaAngulo = angulo;

    int passo = map(ultimaDist, 0, limiteDist, 1, 3);

    if(angulo < anguloPredito) angulo += passo;
    if(angulo > anguloPredito) angulo -= passo;

    int altura = map(ultimaDist, 0, limiteDist, 60, 120);

    servoX.write(angulo);
    servoY.write(altura);

    if(ultimaDist >= limiteDist){
      alvoDetectado = 0;
    }

  }
  else{

    servoX.write(angulo);
    servoY.write(90);

    angulo += direcao;

    if(angulo >= 180) direcao = -1;
    if(angulo <= 0) direcao = 1;
  }

  /* Serial radar data */

  Serial.print(angulo);
  Serial.print(",");
  Serial.print(ultimaDist);
  Serial.print(",");
  Serial.println(90);

  delay(25);
}