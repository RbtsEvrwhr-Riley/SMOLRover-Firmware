// IMPORTANT, see app_httpd.cpp for other settings
// wiring/construction: pins 12~15 to a motor controller (L293, Polulu etc) or pins 12 and 13 to continuous rotation servos
// battery management to taste (TP4056 for single cell etc)
byte brightlight = 4; // light

byte rightservo = 13;
byte leftservo = 12;
byte leftmotor = 13;
byte rightmotor = 12;

byte servo3 = 14;
byte servo4 = 15;

//#define USE_SERVOS_FOR_MOVEMENT // if not, use 2p motors. 4p motors is deprecated. this does, and ever should, only affect the Motors(...) function

#define SERVO_1_CENTER 1500
#define SERVO_2_CENTER 1500
#define SERVO_1_SWING 300
#define SERVO_2_SWING -300
#define SERVO_1_STOP 1500 // 1500 for ESCs, 0 for CR servos that are out of tune
#define SERVO_2_STOP 1500 // 1500 for ESCs, 0 for CR servos that are out of tune


bool initCamera();
String getServerAddress();
void startCameraServer();
extern boolean camera_has_failed;

#include <ESP32_ISR_Servo.h> //https://github.com/khoih-prog/ESP32_ISR_Servo>

#define TIMER_INTERRUPT_DEBUG       0 // redefine for ESP32FastTimerInterrupt
#define ISR_SERVO_DEBUG             0
// Select different ESP32 timer number (0-3) to avoid conflict
#define USE_ESP32_TIMER_NO          3

int servoIndex1  = -1;
int servoIndex2  = -1;


void setup() {
  pinMode(leftmotor, INPUT);
  pinMode(rightmotor, INPUT);
  pinMode(leftservo, INPUT);
  pinMode(rightservo, INPUT);
  pinMode(brightlight, OUTPUT); //
  digitalWrite(brightlight, HIGH);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();


  if (!initCamera())
  {
    //    return; // freeze with the light left on to indicate a problem
    while (true) // best to just blink if there's a problem
    {
      delay(500);
      digitalWrite(brightlight, HIGH);
      delay(500);
      digitalWrite(brightlight, LOW);
    }
  }

  digitalWrite(brightlight, LOW);

  startCameraServer();

  digitalWrite(brightlight, LOW);
  Serial.println();
  Serial.print("Ready! Use ' http://");
  Serial.print(getServerAddress());
  digitalWrite(brightlight, HIGH);
  Serial.println(" ' to connect.");

  digitalWrite(brightlight, LOW);

  Motor(LOW, LOW, LOW, LOW, false);
}

unsigned long stoptimer = 0;
unsigned long frametimer = 0;
static unsigned long last_time = 0;
static boolean turning = false;

static boolean do_motor_setup = true;

/**
   Sets the motor state for locomotion (even if CR servos are being used).
   Then updates the motor (or servo) pin states.
*/
void Motor(int L1, int L2, int R1, int R2, bool turn)
{
#ifdef USE_SERVOS_FOR_MOVEMENT
  if (do_motor_setup)
  {
    pinMode(rightservo, OUTPUT);
    pinMode(leftservo, OUTPUT);
    //Select ESP32 timer USE_ESP32_TIMER_NO
    ESP32_ISR_Servos.useTimer(USE_ESP32_TIMER_NO);
    servoIndex1 = ESP32_ISR_Servos.setupServo(rightservo, 0, 3000);
    servoIndex2 = ESP32_ISR_Servos.setupServo(leftservo, 0, 3000);
    if (servoIndex1 != -1)
    {
      Serial.println("Setup Servo1 OK");
      ESP32_ISR_Servos.setPulseWidth(servoIndex1, 1500);
    }
    else
    {
      Serial.println("Setup Servo1 failed");
    }
    if (servoIndex2 != -1)
    {
      Serial.println("Setup Servo2 OK");
      ESP32_ISR_Servos.setPulseWidth(servoIndex2, 1500);
    }
    else
    {
      Serial.println("Setup Servo2 failed");
    }

    do_motor_setup = false;
  }

  int pwL = SERVO_1_STOP;
  int pwR = SERVO_2_STOP;

  if (L1 == HIGH and L2 == LOW)
    pwL = SERVO_1_CENTER - SERVO_1_SWING;
  else if (L1 == LOW and L2 == HIGH)
    pwL = SERVO_1_CENTER + SERVO_1_SWING;

  if (R1 == HIGH and R2 == LOW)
    pwR = SERVO_2_CENTER - SERVO_2_SWING;
  else if (R1 == LOW and R2 == HIGH)
    pwR = SERVO_2_CENTER + SERVO_2_SWING;

  ESP32_ISR_Servos.setPulseWidth(servoIndex1, pwL);
  ESP32_ISR_Servos.setPulseWidth(servoIndex2, pwR);

  Serial.print("Servos");
  Serial.print(pwL);
  Serial.print(" ");
  Serial.print(pwR);
  Serial.println();

#else
  if (do_motor_setup)
  {
    pinMode(leftmotor, INPUT);
    pinMode(rightmotor, INPUT);
    do_motor_setup = false;
  }
  if (L1 == L2)
  {
    pinMode(leftmotor, INPUT);
    digitalWrite(leftmotor, LOW);
  }
  else
  {
    digitalWrite(leftmotor, L1);
    pinMode(leftmotor, OUTPUT);
  }

  if (R1 == R2)
  {
    pinMode(rightmotor, INPUT);
    digitalWrite(rightmotor, LOW);
  }
  else
  {
    digitalWrite(rightmotor, R1);
    pinMode(rightmotor, OUTPUT);
  }

  Serial.print("Motors2P");
  Serial.print(L1);
  Serial.print(L2);
  Serial.print(R1);
  Serial.print(R2);
  Serial.println();



#endif

  turning = turn;
  if (L1 == LOW && L2 == LOW && R1 == LOW && R2 == LOW)
  {
    stoptimer = 0;
  }
  else
  {
    stoptimer = 12;
  }


}

void loop() {
  // stop in case of no input

  if (camera_has_failed) // deal with it here; reboot? restart the server?
  {
    ESP.restart();
  }



  if (millis() - last_time > 500) {
    last_time = millis();



    if (stoptimer > 1)
    {
      stoptimer = stoptimer - 1;
      if (turning)
        stoptimer = 1;
    }
    else if (stoptimer == 1)
    {
      Motor(LOW, LOW, LOW, LOW, false);
      Serial.println("Stoptimer");
      stoptimer = 0;
    }
    else if (stoptimer < 0)
    {
      stoptimer = 1;
    }
    else if (stoptimer == 0)
    {
    }

    if (frametimer > 1)
    {
      frametimer = frametimer - 1;
      if (turning)
        frametimer = 1;
    }
    else if (frametimer == 1)
    {
      Serial.println("Frametimer");
      frametimer = frametimer - 1;
      Motor(LOW, LOW, LOW, LOW, false);
    }
    else if (frametimer < 0)
    {
      frametimer = 1;
    }
    else if (frametimer == 0)
    {
    }
  }
}
