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
const int DIST_TRAVAR  = 15;   // cm — distância para travar no alvo
const int DIST_LASER   = 10;   // cm — distância para disparar o laser
const int DIST_SONAR   = 200;  // cm — alcance máximo do sonar (timeout)

// ===== MÁQUINA DE ESTADOS =====
enum Estado { PATRULHA, CONFIRMANDO, TRAVADO, PERDENDO };
Estado estadoAtual = PATRULHA;

// ===== CONTROLE DE TEMPO (millis) =====
unsigned long tSonar   = 0;   // último disparo do sonar
unsigned long tServo   = 0;   // última atualização dos servos
unsigned long tSerial  = 0;   // último print no Serial
unsigned long tEstado  = 0;   // tempo que entrou no estado atual

const unsigned long INT_SONAR  = 50;   // ms entre medições
const unsigned long INT_SERVO  = 20;   // ms entre movimentos de servo
const unsigned long INT_SERIAL = 200;  // ms entre prints

// ===== POSIÇÕES DOS SERVOS =====
int anguloX    = 90;
int servoYPos  = 90;

// ===== VARREDURA =====
int direcaoX        = 1;
const int PASSO_X   = 3;   // graus por ciclo na varredura
const int LIMITE_MIN = 2;
const int LIMITE_MAX = 178;

// ===== TRACKING =====
int   anguloAlvo      = 90;
float distFiltrada    = 0.0;
const float ALPHA     = 0.5;   // coeficiente de suavização EMA

// Média móvel do ângulo alvo (3 amostras)
int   historicoAngulo[3] = {90, 90, 90};
int   idxHistorico       = 0;

// Contador de ciclos sem alvo (ex-static — agora global)
int ciclosSemAlvo = 0;
const int CICLOS_PARA_PERDER   = 6;   // ciclos sem alvo → começa a perder
const int CICLOS_CONFIRMACAO   = 2;   // ciclos com alvo → confirma detecção
int ciclosComAlvo = 0;

// Re-scan: varrer em torno da última posição conhecida
bool  reScan          = false;
int   anguloReScan    = 90;
int   direcaoReScan   = 1;
int   amplitudeReScan = 0;
const int MAX_RESCAN  = 30;   // graus de cada lado para re-scan


// ============================================================
//  SENSOR — medição não-bloqueante com filtragem por amostra
// ============================================================
float medirDistancia() {
  // Dispara pulso
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // pulseIn com timeout baseado no alcance máximo
  // DIST_SONAR cm → tempo máximo = (DIST_SONAR * 2 / 0.034) µs
  unsigned long timeout = (unsigned long)(DIST_SONAR * 2.0 / 0.034);
  long duracao = pulseIn(PIN_ECHO, HIGH, timeout);

  if (duracao == 0) return -1.0;   // sem eco = inválido (não entra na média)

  float cm = duracao * 0.034 / 2.0;
  if (cm < 2.0 || cm > DIST_SONAR) return -1.0;  // fora da faixa válida

  return cm;
}


// ============================================================
//  MÉDIA MÓVEL DE ÂNGULO — reduz jitter no tracking
// ============================================================
void registrarAngulo(int ang) {
  historicoAngulo[idxHistorico] = ang;
  idxHistorico = (idxHistorico + 1) % 3;
}

int mediaAngulo() {
  return (historicoAngulo[0] + historicoAngulo[1] + historicoAngulo[2]) / 3;
}


// ============================================================
//  BUZZER — frequência proporcional à distância, não-bloqueante
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
//  TRANSIÇÃO DE ESTADO — centraliza mudanças e reseta contadores
// ============================================================
void mudarEstado(Estado novo) {
  if (novo == estadoAtual) return;
  estadoAtual = novo;
  tEstado = millis();

  if (novo == PATRULHA) {
    ciclosComAlvo = 0;
    ciclosSemAlvo = 0;
    digitalWrite(PIN_LASER, LOW);
    noTone(PIN_BUZZER);
  }

  if (novo == CONFIRMANDO) {
    ciclosSemAlvo = 0;
  }

  if (novo == TRAVADO) {
    ciclosComAlvo = 0;
    ciclosSemAlvo = 0;
    // Inicializa histórico com posição atual
    for (int i = 0; i < 3; i++) historicoAngulo[i] = anguloX;
    idxHistorico = 0;
  }

  if (novo == PERDENDO) {
    // Guarda posição para re-scan
    anguloReScan    = mediaAngulo();
    direcaoReScan   = 1;
    amplitudeReScan = 0;
    reScan          = true;
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

  Serial.begin(115200);   // baud maior = menos bloqueio por char
  Serial.println("AA-SISTEMA V2.0 iniciado");
}


// ============================================================
//  LOOP PRINCIPAL — não-bloqueante
// ============================================================
void loop() {
  unsigned long agora = millis();
  bool novaLeitura    = false;
  bool alvoVisto      = false;

  // ── SONAR (a cada INT_SONAR ms) ──────────────────────────
  if (agora - tSonar >= INT_SONAR) {
    tSonar = agora;

    float dist = medirDistancia();

    if (dist > 0) {
      // EMA só com amostras válidas — bug do dist=0 corrigido
      if (distFiltrada == 0.0)
        distFiltrada = dist;
      else
        distFiltrada = ALPHA * dist + (1.0 - ALPHA) * distFiltrada;

      novaLeitura = true;
      alvoVisto   = (distFiltrada <= DIST_TRAVAR);
    }

    atualizarBuzzer(alvoVisto ? distFiltrada : 999);
  }

  // ── MÁQUINA DE ESTADOS ───────────────────────────────────
  if (novaLeitura) {
    switch (estadoAtual) {

      // ── PATRULHA ──────────────────────────────────────────
      case PATRULHA:
        if (alvoVisto) {
          ciclosComAlvo++;
          if (ciclosComAlvo >= CICLOS_CONFIRMACAO) {
            mudarEstado(CONFIRMANDO);
          }
        } else {
          ciclosComAlvo = 0;
        }
        break;

      // ── CONFIRMANDO ───────────────────────────────────────
      case CONFIRMANDO:
        if (alvoVisto) {
          // Confirma: salva ângulo e trava
          anguloAlvo = anguloX;
          registrarAngulo(anguloX);
          mudarEstado(TRAVADO);
        } else {
          // Falso positivo — volta a patrulhar
          mudarEstado(PATRULHA);
        }
        break;

      // ── TRAVADO ───────────────────────────────────────────
      case TRAVADO:
        if (alvoVisto) {
          // Atualiza ângulo continuamente enquanto vendo o alvo
          // (bug do "anguloAlvo congela" corrigido)
          registrarAngulo(anguloX);
          anguloAlvo    = mediaAngulo();
          ciclosSemAlvo = 0;

          // Laser
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

      // ── PERDENDO ──────────────────────────────────────────
      case PERDENDO:
        if (alvoVisto) {
          // Reencontrou — trava de novo sem precisar confirmar
          mudarEstado(TRAVADO);
        } else {
          ciclosSemAlvo++;
          // Depois de muito tempo sem achar, volta a patrulhar normalmente
          if (ciclosSemAlvo >= CICLOS_PARA_PERDER * 4) {
            reScan = false;
            distFiltrada = 0.0;
            mudarEstado(PATRULHA);
          }
        }
        break;
    }
  }

  // ── MOVIMENTO DOS SERVOS (a cada INT_SERVO ms) ────────────
  if (agora - tServo >= INT_SERVO) {
    tServo = agora;

    switch (estadoAtual) {

      case PATRULHA:
      case CONFIRMANDO: {
        // Varredura horizontal suave
        anguloX += PASSO_X * direcaoX;
        anguloX  = constrain(anguloX, LIMITE_MIN, LIMITE_MAX);
        if (anguloX >= LIMITE_MAX) direcaoX = -1;
        if (anguloX <= LIMITE_MIN) direcaoX =  1;
        servoYPos = 90;
        break;
      }

      case TRAVADO: {
        // Tracking proporcional para o ângulo alvo
        int erro    = anguloAlvo - anguloX;
        int passo   = constrain(erro / 2, -6, 6);
        if (abs(erro) > 1) anguloX += passo;
        anguloX = constrain(anguloX, LIMITE_MIN, LIMITE_MAX);

        // Vertical: sobe proporcionalmente à proximidade
        int alturaAlvo = map((int)distFiltrada, 0, DIST_TRAVAR, 115, 75);
        int erroY      = alturaAlvo - servoYPos;
        servoYPos     += constrain(erroY / 2, -4, 4);
        servoYPos      = constrain(servoYPos, 60, 130);
        break;
      }

      case PERDENDO: {
        // Re-scan em leque ao redor da última posição conhecida
        anguloX = constrain(
          anguloReScan + (amplitudeReScan * direcaoReScan),
          LIMITE_MIN, LIMITE_MAX
        );
        amplitudeReScan++;
        if (amplitudeReScan > MAX_RESCAN) {
          amplitudeReScan = 0;
          direcaoReScan  *= -1;   // inverte direção do re-scan
        }
        servoYPos = 90;
        break;
      }
    }

    servoX.write(anguloX);
    servoY.write(servoYPos);
  }

  // ── SERIAL (a cada INT_SERIAL ms — não bloqueia o loop) ──
  if (agora - tSerial >= INT_SERIAL) {
    tSerial = agora;

    const char* nomeEstado[] = {"PATRULHA", "CONFIRMANDO", "TRAVADO", "PERDENDO"};
    Serial.print("Estado:");  Serial.print(nomeEstado[estadoAtual]);
    Serial.print(" AX:");     Serial.print(anguloX);
    Serial.print(" AY:");     Serial.print(servoYPos);
    Serial.print(" Alvo:");   Serial.print(anguloAlvo);
    Serial.print(" Dist:");   Serial.print(distFiltrada, 1);
    Serial.print("cm");
    Serial.println();
  }
}
