/*
 * Robot Mini Tñ Chç - Dñ Án STEM (NON-BLOCKING VERSION)
 * Ph§n céng: Arduino Nano, HC-SR04, Servo SG90, L9110S, OLED 0.96", Motor DC
 * ã fix: delay() blocking, debounce, kiÃm tra pin an toàn
 */

#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==================== C¤U HÌNH CHÂN ====================
#define TRIG_PIN 2
#define ECHO_PIN 3
#define SERVO_PIN 9
#define MOTOR_A1 5
#define MOTOR_A2 6
#define TOUCH_PIN 4

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==================== BI¾N TOÀN CäC ====================
Servo steeringServo;
const int SERVO_CENTER = 90;
const int SERVO_LEFT = 60;
const int SERVO_RIGHT = 120;
const int SERVO_LEFT_CIRCLE = 70;
const int SERVO_RIGHT_CIRCLE = 110;

int currentServoPos = SERVO_CENTER;
int targetServoPos = SERVO_CENTER;

unsigned long phaseStartTime = 0;
int currentPhase = 1;
int motorSpeed = 120;

// ==================== GIAI O N 1 ====================
unsigned long lastTurnTime = 0;
int turnCount = 0;
bool isTurning = false;
unsigned long turnStartTime = 0;
int turnDirection = 0; // 0=trái, 1=ph£i

// ==================== GIAI O N 3 ====================
unsigned long phase3StartTime = 0;
unsigned long lastHeadShake = 0;
int headShakeCount = 0;
int headShakeState = 0; // 0=center, 1=left, 2=right
unsigned long headShakeStepTime = 0;

// Debounce cho touch sensor
unsigned long lastTouchTime = 0;
bool lastTouchState = false;
bool touchState = false;
const unsigned long DEBOUNCE_DELAY = 50;

unsigned long touchStartTime = 0;
bool isTouchingConfirmed = false;

// ==================== ANIMATION & PIN ====================
unsigned long lastAnimFrame = 0;
int animFrame = 0;
String currentEmotion = "neutral";

unsigned long lastBatteryCheck = 0;
int abnormalMovementCount = 0;
float lastDistanceCheck = 0;
bool batteryLow = false;

// ==================== SERVO MÀM M I ====================
unsigned long lastServoUpdate = 0;
const int SERVO_STEP = 2; // Ù m°ãt servo

// ==================== THI¾T L¬P ====================
void setup() {
  Serial.begin(9600);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOTOR_A1, OUTPUT);
  pinMode(MOTOR_A2, OUTPUT);
  pinMode(TOUCH_PIN, INPUT_PULLUP);
  
  steeringServo.attach(SERVO_PIN);
  steeringServo.write(SERVO_CENTER);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED failed"));
    for(;;);
  }
  
  // Hello World
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 25);
  display.println("Hello");
  display.setCursor(10, 45);
  display.println("World!");
  display.display();
  
  unsigned long helloStart = millis();
  while(millis() - helloStart < 2000) {
    // Non-blocking wait
    updateServoSmooth();
  }
  
  displayEmotion("neutral");
  randomSeed(analogRead(0));
  phaseStartTime = millis();
  Serial.println(":3");
}

// ==================== O KHO¢NG CÁCH ====================
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distance = duration * 0.034 / 2;
  
  if(distance == 0 || distance > 400) return 400;
  return distance;
}

// ==================== IÀU KHIÂN ØNG C  ====================
void moveForward() {
  analogWrite(MOTOR_A1, motorSpeed);
  analogWrite(MOTOR_A2, 0);
}

void moveBackward() {
  analogWrite(MOTOR_A1, 0);
  analogWrite(MOTOR_A2, motorSpeed);
}

void stopMotor() {
  analogWrite(MOTOR_A1, 0);
  analogWrite(MOTOR_A2, 0);
}

// ==================== SERVO MÀM M I (NON-BLOCKING) ====================
void setServoTarget(int angle) {
  targetServoPos = constrain(angle, 30, 150); // B£o vÇ servo
}

void updateServoSmooth() {
  if(millis() - lastServoUpdate < 20) return; // C­p nh­t m×i 20ms
  
  if(currentServoPos < targetServoPos) {
    currentServoPos = min(currentServoPos + SERVO_STEP, targetServoPos);
    steeringServo.write(currentServoPos);
  } else if(currentServoPos > targetServoPos) {
    currentServoPos = max(currentServoPos - SERVO_STEP, targetServoPos);
    steeringServo.write(currentServoPos);
  }
  
  lastServoUpdate = millis();
}

// ==================== DEBOUNCE TOUCH SENSOR ====================
void updateTouchSensor() {
  bool reading = (digitalRead(TOUCH_PIN) == LOW);
  
  if(reading != lastTouchState) {
    lastTouchTime = millis();
  }
  
  if((millis() - lastTouchTime) > DEBOUNCE_DELAY) {
    if(reading != touchState) {
      touchState = reading;
      
      if(touchState) {
        // B¯t §u ch¡m
        isTouchingConfirmed = true;
        touchStartTime = millis();
      } else {
        // Th£ ra
        isTouchingConfirmed = false;
      }
    }
  }
  
  lastTouchState = reading;
}

// ==================== HIÂN THÊ C¢M XÚC VÚI ANIMATION ====================
void displayEmotion(String emotion) {
  if(emotion != currentEmotion) {
    currentEmotion = emotion;
    animFrame = 0;
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  int offsetY = (animFrame == 0) ? 0 : 2;
  
  if(emotion == "neutral") {
    display.setTextSize(3);
    display.setCursor(30, 20 + offsetY);
    display.println("^_^");
  }
  else if(emotion == "raised_eyebrow") {
    display.setTextSize(3);
    display.setCursor(25, 20 + offsetY);
    display.println("^_-");
  }
  else if(emotion == "happy") {
    display.setTextSize(3);
    display.setCursor(25, 20 + offsetY);
    display.println(animFrame == 0 ? "^o^" : "^O^");
  }
  else if(emotion == "very_happy") {
    display.setTextSize(3);
    display.setCursor(20, 18 + offsetY * 2);
    display.println(animFrame == 0 ? "*^o^*" : "*^O^*");
  }
  else if(emotion == "scared") {
    int shakeX = (animFrame == 0) ? 25 : 27;
    display.setTextSize(3);
    display.setCursor(shakeX, 20);
    display.println("O_O");
  }
  else if(emotion == "curious") {
    display.setTextSize(3);
    display.setCursor(30, 20 + offsetY);
    display.println(animFrame == 0 ? "o_o" : "O_o");
  }
  else if(emotion == "dead") {
    display.setTextSize(3);
    display.setCursor(25, 20);
    display.println("X_X");
    display.setTextSize(1);
    display.setCursor(20, 50);
    display.println("Battery Low");
  }
  
  display.display();
}

void updateAnimation() {
  if(millis() - lastAnimFrame > 500) {
    animFrame = (animFrame + 1) % 2;
    lastAnimFrame = millis();
    displayEmotion(currentEmotion);
  }
}

// ==================== KIÂM TRA PIN AN TOÀN (NON-BLOCKING) ====================
void checkBatterySafe() {
  if(millis() - lastBatteryCheck < 5000) return;
  
  float currentDist = getDistance();
  
  // So sánh vÛi l§n o tr°Ûc
  if(lastDistanceCheck > 0) {
    // N¿u robot ang ch¡y mà kho£ng cách không Õi ’ b¥t th°Ýng
    if(abs(currentDist - lastDistanceCheck) < 3) {
      abnormalMovementCount++;
    } else {
      abnormalMovementCount = max(0, abnormalMovementCount - 1);
    }
  }
  
  lastDistanceCheck = currentDist;
  lastBatteryCheck = millis();
  
  if(abnormalMovementCount > 2) {
    batteryLow = true;
  }
}

// ==================== GIAI O N 1: RANDOM NON-BLOCKING ====================
void phase1() {
  unsigned long elapsed = millis() - phaseStartTime;
  
  if(elapsed >= 10000) {
    stopMotor();
    setServoTarget(SERVO_CENTER);
    currentPhase = 2;
    phaseStartTime = millis();
    Serial.println(":3");
    return;
  }
  
  moveForward();
  
  if(!isTurning) {
    // KiÃm tra xem có c§n r½ không
    if(turnCount < 4 && millis() - lastTurnTime > 1500) {
      isTurning = true;
      turnStartTime = millis();
      turnDirection = random(0, 2);
      turnCount++;
      
      if(turnDirection == 0) {
        setServoTarget(SERVO_LEFT);
      } else {
        setServoTarget(SERVO_RIGHT);
      }
    }
    // Random r½ thêm sau khi ç 4 l§n
    else if(turnCount >= 4 && random(0, 100) < 3) {
      isTurning = true;
      turnStartTime = millis();
      turnDirection = random(0, 2);
      
      if(turnDirection == 0) {
        setServoTarget(SERVO_LEFT);
      } else {
        setServoTarget(SERVO_RIGHT);
      }
    }
  } else {
    // ang trong quá trình r½
    if(millis() - turnStartTime > 800) {
      setServoTarget(SERVO_CENTER);
      isTurning = false;
      lastTurnTime = millis();
    }
  }
}

// ==================== GIAI O N 2: VÒNG TRÒN & SÐ 8 ====================
void phase2() {
  unsigned long elapsed = millis() - phaseStartTime;
  
  if(elapsed >= 10000) {
    stopMotor();
    setServoTarget(SERVO_CENTER);
    currentPhase = 3;
    phaseStartTime = millis();
    phase3StartTime = millis();
    lastHeadShake = millis();
    headShakeCount = 0;
    headShakeState = 0;
    Serial.println(":3");
    return;
  }
  
  moveForward();
  
  if(elapsed < 5000) {
    // 3 vòng tròn
    setServoTarget(SERVO_LEFT_CIRCLE);
  } else {
    // Hình sÑ 8
    unsigned long subTime = elapsed - 5000;
    if(subTime < 2500) {
      setServoTarget(SERVO_LEFT_CIRCLE);
    } else {
      setServoTarget(SERVO_RIGHT_CIRCLE);
    }
  }
}

// ==================== GIAI O N 3: T¯ NG TÁC ====================
void phase3() {
  unsigned long elapsed = millis() - phaseStartTime;
  float distance = getDistance();
  
  // 50s cuÑi: sã v­t c£n
  if(elapsed > 130000) {
    stopMotor();
    setServoTarget(SERVO_CENTER);
    
    if(distance < 30) {
      displayEmotion("scared");
      moveBackward();
    } else {
      displayEmotion("neutral");
    }
    
    if(elapsed > 180000) {
      currentPhase = 1;
      phaseStartTime = millis();
      turnCount = 0;
      isTurning = false;
      Serial.println(":3");
    }
    return;
  }
  
  // 20s §u: Aim ho·c l¯c §u
  if(elapsed < 20000) {
    stopMotor();
    
    if(distance < 50) {
      // Aim theo v­t thÃ
      if(distance < 30) {
        setServoTarget(SERVO_CENTER);
      } else if(random(0, 2) == 0) {
        setServoTarget(75);
      } else {
        setServoTarget(105);
      }
      displayEmotion("curious");
      headShakeCount = 0; // Reset
      
    } else {
      // L¯c §u NON-BLOCKING
      if(headShakeCount < 4 && millis() - lastHeadShake > 5000) {
        headShakeState = 0;
        headShakeStepTime = millis();
        lastHeadShake = millis();
      }
      
      if(headShakeState == 0 && millis() - headShakeStepTime < 500) {
        setServoTarget(SERVO_LEFT);
      } else if(headShakeState == 0 && millis() - headShakeStepTime >= 500) {
        headShakeState = 1;
        headShakeStepTime = millis();
      } else if(headShakeState == 1 && millis() - headShakeStepTime < 500) {
        setServoTarget(SERVO_RIGHT);
      } else if(headShakeState == 1 && millis() - headShakeStepTime >= 500) {
        headShakeState = 2;
        setServoTarget(SERVO_CENTER);
        headShakeCount++;
      }
      
      displayEmotion("neutral");
    }
    
  } else {
    // Sau 20s: t°¡ng tác
    
    if(isTouchingConfirmed) {
      unsigned long touchDuration = millis() - touchStartTime;
      if(touchDuration < 2000) {
        displayEmotion("happy");
      } else {
        displayEmotion("very_happy");
      }
      stopMotor();
      
    } else {
      if(distance > 80) {
        displayEmotion("neutral");
        stopMotor();
        setServoTarget(SERVO_CENTER);
        
      } else if(distance > 50 && distance <= 80) {
        moveForward();
        setServoTarget(SERVO_CENTER);
        displayEmotion("curious");
        
      } else if(distance > 10 && distance <= 50) {
        moveForward();
        displayEmotion("raised_eyebrow");
        
      } else if(distance <= 10) {
        stopMotor();
        setServoTarget(SERVO_CENTER);
        displayEmotion("raised_eyebrow");
      }
    }
  }
}

// ==================== VÒNG L¶P CHÍNH ====================
void loop() {
  // C­p nh­t các hÇ thÑng NON-BLOCKING
  updateServoSmooth();
  updateTouchSensor();
  updateAnimation();
  checkBatterySafe();
  
  // Xí lý pin y¿u
  if(batteryLow) {
    stopMotor();
    setServoTarget(SERVO_CENTER);
    displayEmotion("dead");
    
    unsigned long deadStart = millis();
    while(millis() - deadStart < 5000) {
      updateServoSmooth();
      updateAnimation();
    }
    
    batteryLow = false;
    abnormalMovementCount = 0;
    return;
  }
  
  // Ch¡y giai o¡n hiÇn t¡i
  switch(currentPhase) {
    case 1:
      phase1();
      break;
    case 2:
      phase2();
      break;
    case 3:
      phase3();
      break;
  }
}