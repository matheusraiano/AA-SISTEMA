#include <Servo.h>

/* ===== HARDWARE ===== */
Servo servoX;
Servo servoY;

/* ===== PINOS ===== */
const int trig = 7;
const int echo = 8;
const int laserPin = 6;
const int buzzer = 5;

/* ===== CONTROLE DE MOVIMENTO ===== */
int angulo = 90;
int direcao = 1;

int ultimaAngulo = 90;
int ultimaDist = 0;

int alvoDetectado = 0;

/* ===== CONFIGURAÇÃO ===== */
const int limiteDist = 150;      // Tracking só até 150 cm
const float suavizacao = 0.7;
const float fatorPredicao = 0.5;

/* ===== MULTI-ALVO ===== */
const int MAX_ALVOS = 50;
int alvosAngulo[MAX_ALVOS];
int alvosDist[MAX_ALVOS];
int contadorAlvos = 0;

/* ===== PREDIÇÃO APRIMORADA ===== */
const int N = 5;
int ultimosAngulos[N] = {90,90,90,90,90};
int indice = 0;

/* ===== SENSOR ===== */
long medirDistancia(){
  long soma = 0;
  for(int i=0;i<2;i++){ // menos leituras para acelerar
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

/* ===== AVISO SONORO CONTÍNUO ===== */
void avisoDistancia(int dist){
  if(dist <= limiteDist){ // só se estiver dentro do limite do track
    int freq = map(dist, limiteDist, 0, 500, 2000);   // mais perto = mais agudo
    int duracao = map(dist, limiteDist, 0, 300, 50);  // mais perto = mais rápido
    tone(buzzer, freq, duracao);
    delay(duracao);
  } else {
    noTone(buzzer); // fora do alcance, sem som
  }
}

/* ===== MULTI-ALVO ===== */
void registrarAlvo(int ang, int dist){
  if(contadorAlvos < MAX_ALVOS){
    alvosAngulo[contadorAlvos] = ang;
    alvosDist[contadorAlvos] = dist;
    contadorAlvos++;
  }
}
void resetarAlvos(){
  contadorAlvos = 0;
}

/* ===== SETUP ===== */
void setup(){
  servoX.attach(9);
  servoY.attach(10);

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(laserPin, OUTPUT);
  pinMode(buzzer, OUTPUT);

  Serial.begin(9600);
}

/* ===== LOOP PRINCIPAL ===== */
void loop(){
  digitalWrite(laserPin, HIGH); // laser sempre ligado
  int dist = medirDistancia();

  /* Filtro suavização */
  ultimaDist = (ultimaDist * suavizacao) + (dist * (1 - suavizacao));

  /* Aviso sonoro contínuo */
  avisoDistancia(ultimaDist);

  /* Registrar alvo para mapa multi-alvo */
  if(ultimaDist > 0 && ultimaDist <= limiteDist){
    registrarAlvo(angulo, ultimaDist);
    alvoDetectado = 1;
  }

  /* ===== TRACKING ===== */
  if(alvoDetectado && ultimaDist <= limiteDist){
    /* Atualizar média móvel para predição */
    ultimosAngulos[indice] = angulo;
    indice = (indice + 1) % N;

    int soma = 0;
    for(int i=0;i<N;i++) soma += ultimosAngulos[i];
    int anguloPredito = soma / N;

    /* Passo de movimento baseado na distância */
    int passo = map(ultimaDist, 0, limiteDist, 2, 6); // servo mais rápido
    if(angulo < anguloPredito) angulo += passo;
    if(angulo > anguloPredito) angulo -= passo;

    /* Controle vertical */
    int altura = map(ultimaDist, 0, limiteDist, 60, 120);

    servoX.write(angulo);
    servoY.write(altura);

    /* Disparo laser em alvo próximo */
    if(ultimaDist <= 90){ // laser só dispara ≤90 cm
      digitalWrite(laserPin, HIGH);
      delay(40);
      digitalWrite(laserPin, LOW);
      delay(40);
      Serial.println("Laser disparado!"); // indica no Serial
    }

    if(ultimaDist >= limiteDist){
      alvoDetectado = 0;
      resetarAlvos(); // limpa multi-alvo
    }
  }
  else{
    servoX.write(angulo);
    servoY.write(90);
    angulo += direcao * 6; // patrulhamento mais rápido

    if(angulo >= 180) direcao = -1;
    if(angulo <= 0) direcao = 1;
  }

  /* Enviar dados para Serial (radar) */
  Serial.print("AnguloX:"); Serial.print(angulo);
  Serial.print(", AnguloY:"); Serial.print(map(ultimaDist, 0, limiteDist, 60, 120));
  Serial.print(", Distancia:"); Serial.print(ultimaDist);
  Serial.print(", AlvoDetectado:"); Serial.print(alvoDetectado);
  Serial.print(", ContadorAlvos:"); Serial.println(contadorAlvos);

  delay(10); // loop mais rápido
}