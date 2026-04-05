#include <Servo.h>

// ===== HARDWARE =====
Servo servoX;
Servo servoY;

// ===== PINOS =====
const int PIN_TRIG    = 7;
const int PIN_ECHO    = 8;
const int PIN_LASER   = 6;
const int PIN_BUZZER  = 5;
const int PIN_SERVO_X = 9;
const int PIN_SERVO_Y = 10;

// ===== LIMITES DO SISTEMA =====
const int DIST_TRAVAR  = 30;
const int DIST_LASER   = 15;
const int DIST_SONAR   = 200;

// ===== MÁQUINA DE ESTADOS =====
enum Estado { PATRULHA, CONFIRMANDO, TRAVADO, PERDENDO };
Estado estadoAtual = PATRULHA;

// ===== CONTROLE DE TEMPO =====
unsigned long tSonar   = 0;
unsigned long tServo   = 0;
unsigned long tSerial  = 0;
unsigned long tEstado  = 0;

const unsigned long INT_SONAR  = 50;
const unsigned long INT_SERVO  = 20;
const unsigned long INT_SERIAL = 200;

// ===== POSIÇÕES DOS SERVOS =====
int anguloX   = 90;
int servoYPos = 90;

// ===== VARREDURA =====
int direcaoX         = 1;
const int PASSO_X    = 3;
const int LIMITE_MIN = 2;
const int LIMITE_MAX = 178;

// ===== TRACKING =====
int   anguloAlvo   = 90;
float distFiltrada = 0.0;
const float ALPHA  = 0.5;

// Média móvel de ângulo (3 amostras)
int historicoAngulo[3] = {90, 90, 90};
int idxHistorico       = 0;

// Contadores de estado
int ciclosSemAlvo = 0;
int ciclosComAlvo = 0;
const int CICLOS_PARA_PERDER  = 6;
const int CICLOS_CONFIRMACAO  = 2;

// Re-scan
bool reScan          = false;
int  anguloReScan    = 90;
int  direcaoReScan   = 1;
int  amplitudeReScan = 0;
const int MAX_RESCAN = 30;

// ===== PID =====
const float KP = 0.2;
const float KI = 0.07;
const float KD = 0.8;

const float INTEGRAL_MAX = 20.0;
const float SAIDA_MAX    = 8.0;

float pid_integral         = 0.0;
float pid_prevErro         = 0.0;
unsigned long pid_tAnterior = 0;


// ============================================================
//  PID
// ============================================================
float calcularPID(float setpoint, float atual) {
  unsigned long agora = millis();
  float dt = (agora - pid_tAnterior) / 1000.0;
  pid_tAnterior = agora;

  if (dt <= 0.001 || dt > 0.5) return 0;

  float erro = setpoint - atual;

  pid_integral += erro * dt;
  pid_integral  = constrain(pid_integral, -INTEGRAL_MAX, INTEGRAL_MAX);

  float derivada = (erro - pid_prevErro) / dt;
  pid_prevErro   = erro;

  float saida = KP * erro + KI * pid_integral + KD * derivada;
  return constrain(saida, -SAIDA_MAX, SAIDA_MAX);
}

void resetarPID() {
  pid_integral  = 0.0;
  pid_prevErro  = 0.0;
  pid_tAnterior = millis();
}


// ============================================================
//  SENSOR
// ============================================================
float medirDistancia() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long timeout = (unsigned long)(DIST_SONAR * 2.0 / 0.034);
  long duracao = pulseIn(PIN_ECHO, HIGH, timeout);

  if (duracao == 0) return -1.0;

  float cm = duracao * 0.034 / 2.0;
  if (cm < 2.0 || cm > DIST_SONAR) return -1.0;

  return cm;
}


// ============================================================
//  MÉDIA MÓVEL DE ÂNGULO
// ============================================================
void registrarAngulo(int ang) {
  historicoAngulo[idxHistorico] = ang;
  idxHistorico = (idxHistorico + 1) % 3;
}

int mediaAngulo() {
  return (historicoAngulo[0] + historicoAngulo[1] + historicoAngulo[2]) / 3;
}


// ============================================================
//  BUZZER
// ============================================================
void atualizarBuzzer(float dist) {
  if (dist > 0 && dist <= DIST_TRAVAR) {
    int freq = map((int)dist, DIST_TRAVAR, 0, 500, 3000);
    tone(PIN_BUZZER, freq);
  } else {
    noTone(PIN_BUZZER);
  }
}


// ============================================================
//  MÁQUINA DE ESTADOS
// ============================================================
void mudarEstado(Estado novo) {
  if (novo == estadoAtual) return;
  estadoAtual = novo;
  tEstado     = millis();

  if (novo == PATRULHA) {
    ciclosComAlvo = 0;
    ciclosSemAlvo = 0;
    digitalWrite(PIN_LASER, LOW);
    noTone(PIN_BUZZER);
    resetarPID();
  }

  if (novo == CONFIRMANDO) {
    ciclosSemAlvo = 0;
  }

  if (novo == TRAVADO) {
    ciclosComAlvo = 0;
    ciclosSemAlvo = 0;
    for (int i = 0; i < 3; i++) historicoAngulo[i] = anguloX;
    idxHistorico = 0;
    resetarPID();
  }

  if (novo == PERDENDO) {
    anguloReScan    = mediaAngulo();
    direcaoReScan   = 1;
    amplitudeReScan = 0;
    reScan          = true;
    resetarPID();
  }
}


// ============================================================
//  SETUP
// ============================================================
void setup() {
  servoX.attach(PIN_SERVO_X);
  servoY.attach(PIN_SERVO_Y);
  pinMode(PIN_TRIG,   OUTPUT);
  pinMode(PIN_ECHO,   INPUT);
  pinMode(PIN_LASER,  OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_LASER, LOW);

  servoX.write(anguloX);
  servoY.write(servoYPos);

  Serial.begin(115200);
  Serial.println("AA-SISTEMA V2.1 iniciado");
}


// ============================================================
//  LOOP PRINCIPAL
// ============================================================
void loop() {
  unsigned long agora = millis();
  bool novaLeitura    = false;
  bool alvoVisto      = false;

  // ── SONAR ────────────────────────────────────────────────
  if (agora - tSonar >= INT_SONAR) {
    tSonar = agora;

    float dist = medirDistancia();

    if (dist > 0) {
      if (distFiltrada == 0.0)
        distFiltrada = dist;
      else
        distFiltrada = ALPHA * dist + (1.0 - ALPHA) * distFiltrada;

      novaLeitura = true;
      alvoVisto   = (distFiltrada <= DIST_TRAVAR);
    }

    atualizarBuzzer(alvoVisto ? distFiltrada : 999);
  }

  // ── ESTADOS ──────────────────────────────────────────────
  if (novaLeitura) {
    switch (estadoAtual) {

      case PATRULHA:
        if (alvoVisto) {
          ciclosComAlvo++;
          if (ciclosComAlvo >= CICLOS_CONFIRMACAO)
            mudarEstado(CONFIRMANDO);
        } else {
          ciclosComAlvo = 0;
        }
        break;

      case CONFIRMANDO:
        if (alvoVisto) {
          anguloAlvo = anguloX;
          registrarAngulo(anguloX);
          mudarEstado(TRAVADO);
        } else {
          mudarEstado(PATRULHA);
        }
        break;

      case TRAVADO:
        if (alvoVisto) {
          registrarAngulo(anguloX);
          anguloAlvo    = mediaAngulo();
          ciclosSemAlvo = 0;

          if (distFiltrada <= DIST_LASER) {
            digitalWrite(PIN_LASER, HIGH);
            Serial.println(">>> LASER DISPARADO <<<");
          } else {
            digitalWrite(PIN_LASER, LOW);
          }
        } else {
          ciclosSemAlvo++;
          if (ciclosSemAlvo >= CICLOS_PARA_PERDER) {
            digitalWrite(PIN_LASER, LOW);
            mudarEstado(PERDENDO);
          }
        }
        break;

      case PERDENDO:
        if (alvoVisto) {
          mudarEstado(TRAVADO);
        } else {
          ciclosSemAlvo++;
          if (ciclosSemAlvo >= CICLOS_PARA_PERDER * 4) {
            reScan       = false;
            distFiltrada = 0.0;
            mudarEstado(PATRULHA);
          }
        }
        break;
    }
  }

  // ── SERVOS ───────────────────────────────────────────────
  if (agora - tServo >= INT_SERVO) {
    tServo = agora;

    switch (estadoAtual) {

      case PATRULHA:
      case CONFIRMANDO:
        anguloX += PASSO_X * direcaoX;
        anguloX  = constrain(anguloX, LIMITE_MIN, LIMITE_MAX);
        if (anguloX >= LIMITE_MAX) direcaoX = -1;
        if (anguloX <= LIMITE_MIN) direcaoX =  1;
        servoYPos = 90;
        break;

      case TRAVADO: {
        // PID no eixo X
        float correcao = calcularPID((float)anguloAlvo, (float)anguloX);
        anguloX = constrain(anguloX + (int)correcao, LIMITE_MIN, LIMITE_MAX);

        // Proporcional no eixo Y
        int alturaAlvo = map((int)distFiltrada, 0, DIST_TRAVAR, 115, 75);
        int erroY      = alturaAlvo - servoYPos;
        servoYPos     += constrain(erroY / 2, -4, 4);
        servoYPos      = constrain(servoYPos, 60, 130);
        break;
      }

      case PERDENDO:
        anguloX = constrain(
          anguloReScan + (amplitudeReScan * direcaoReScan),
          LIMITE_MIN, LIMITE_MAX
        );
        amplitudeReScan++;
        if (amplitudeReScan > MAX_RESCAN) {
          amplitudeReScan = 0;
          direcaoReScan  *= -1;
        }
        servoYPos = 90;
        break;
    }

    servoX.write(anguloX);
    servoY.write(servoYPos);
  }

  // ── SERIAL ───────────────────────────────────────────────
  if (agora - tSerial >= INT_SERIAL) {
    tSerial = agora;

    const char* nomeEstado[] = {"PATRULHA", "CONFIRMANDO", "TRAVADO", "PERDENDO"};
    Serial.print("Estado:"); Serial.print(nomeEstado[estadoAtual]);
    Serial.print(" AX:");    Serial.print(anguloX);
    Serial.print(" AY:");    Serial.print(servoYPos);
    Serial.print(" Alvo:");  Serial.print(anguloAlvo);
    Serial.print(" Dist:");  Serial.print(distFiltrada, 1);
    Serial.print("cm I:");   Serial.print(pid_integral, 2);
    Serial.println();
  }
}
