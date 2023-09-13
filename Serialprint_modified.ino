//ST,0,INIT,0,ED\r\n
//ST,0,MOVE,0,125,50,ED\r\n

#include <Arduino.h>

const int CW_PIN = 5;
const int CCW_PIN = 6;
const int RELAY_PIN = 12;
const int RIGHT_SENSOR_PIN = 10;
const int LEFT_SENSOR_PIN = 11;

const float SENSOR_BACK_STEP = 250; 
const float STEP_ANGLE = 0.072;
const int STEP_DELAY = 49; // Initial step delay value

float _step_start_position = 0;
float _step_end_position = 0;
float _step_current_position = 0;

float _step_zero_angle = 50;  // ex) 50: 50 step == 0'

int _step_delay = STEP_DELAY;

enum class MotorState { STOPPED, MOVING_CW, MOVING_CCW };

MotorState _motorState = MotorState::STOPPED;
bool _isHoming = false;

void setup() {
  pinMode(CW_PIN, OUTPUT);
  pinMode(CCW_PIN, OUTPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LEFT_SENSOR_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); //  LOW initially
  Serial.begin(115200);
  Serial.flush();
}

void motorStep(bool isClockwise, int steps) {
  for(int i=0; i<steps; i++) {
    digitalWrite(isClockwise ? CW_PIN : CCW_PIN, HIGH);
    delayMicroseconds(_step_delay);
    digitalWrite(isClockwise ? CW_PIN : CCW_PIN, LOW);
    delayMicroseconds(_step_delay);

    if(isClockwise)
      _step_current_position ++;
    else 
      _step_current_position --;
  }
}

void stopMotor() {
  digitalWrite(CW_PIN, LOW);
  digitalWrite(CCW_PIN, LOW);

  _motorState = MotorState::STOPPED;
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//ST,0,INIT,0,ED\r\n   
//////MOVE START /////
//ST,0,LAMP,1,ED\r\n
//ST,0,MOVE,0,125,50,ED\r\n
void moveMotor(int angle, int speedPercent) {
  if (_step_start_position == 0 && _step_end_position == 0) {
    Serial.print("ST,0,RETMOVE,ERR_NOINIT,0,0,ED\r\n"); //not initialized
    return;

  }

  // Calculate the target position based on the angle
  int target_position = static_cast<int>((angle - _step_zero_angle) / STEP_ANGLE);
  int step_target_position = _motorState == MotorState::MOVING_CW ? target_position : -target_position;

  //Serial.print("test1 zero step" + String(_step_zero_angle) + "\r\n" );
  Serial.print("test2 target step" + String(step_target_position) + "\r\n" );

  // Determine the direction of movement
  bool isClockwise = (_step_current_position - step_target_position) <= 0;

  // Calculate the speed based on the speed percentage
  int speed = map(speedPercent, 0, 100, 1000, 10);

  Serial.print("Moving motor to angle " + String(angle) + " at speed " + String(speed) + "\r\n");

  _step_delay = speed;
  float retAngle = (_step_current_position - _step_zero_angle) * STEP_ANGLE;

  _motorState = isClockwise ? MotorState::MOVING_CW : MotorState::MOVING_CCW;

  while (_step_current_position != step_target_position) {
    bool sleft = digitalRead(LEFT_SENSOR_PIN) == LOW;
    bool sright = digitalRead(RIGHT_SENSOR_PIN) == LOW;

    // Detect sensors
    if (sleft || sright) {
      stopMotor();
      delay(1000); // Stop for 1 second

      // Back up motor a little
      if (isClockwise) {
        motorStep(true, SENSOR_BACK_STEP); // Rotate 8 degrees CCW
      } else {
        motorStep(false, SENSOR_BACK_STEP); // Rotate 8 degrees CW
      }

      delay(1000); // Stop for 1 second
      return;
    }

    // No sensor detected, move the motor by one step
    motorStep(isClockwise, 1);

    //U can replace the below CGT & CTL error if-conditions with if!=
    //if (step_target_position != _step_current_position)  {
    //// Motor did not reach the target position and speed
      //Serial.print("ST,0,RETMOVE,ERR_NOT," + String(retAngle) + "," + String(speedPercent) + ",ED\r\n");
      //return;
    //}

    if (_step_current_position >= _step_end_position) {
      Serial.print("ST,0,RETMOVE,ERR_CGT," + String(retAngle) + "," + String(speedPercent) + ",ED\r\n");
      return;
    }

    if (_step_current_position <= _step_start_position) {
      Serial.print("ST,0,RETMOVE,ERR_CLT," + String(retAngle) + "," + String(speedPercent) + ",ED\r\n");
      return;
    }
  }

  // Motor reached the target position and speed
  //float retAngle = (_step_current_position - _step_zero_angle) * STEP_ANGLE;
  Serial.print("ST,0,RETMOVE,OK," + String(retAngle) + "," + String(speedPercent) + ",ED\r\n");
}

////MOVE END /////////

void initializeMotor() {
  int stepOverCheck = 500000;

  bool sleft = true;
  bool sright = true;

  int old_step_delay = _step_delay;
  //_step_delay = 1000;

  // loop whether check initial position or not
  do {
    motorStep(false, 1); // Rotate CCW until right sensor is interrupted
    stepOverCheck--;
    if(stepOverCheck < 0) {
      Serial.print("ST,0,RETINIT,ERR-OV-CCW,ED\r\n");
      _step_delay = old_step_delay;
      return;
    }
    sleft = digitalRead(LEFT_SENSOR_PIN) == LOW;
    sright = digitalRead(RIGHT_SENSOR_PIN) == LOW;
    
    if(sright) {  // LEFT
      //Serial.print("ST,0,RETINIT,ERR_OP,ED\r\n"); // OPPOSITE SIDE INIT
      stopMotor();
      delay(1000);

      motorStep(true, SENSOR_BACK_STEP); // Rotate 8 degrees CW
      break;
    }
    else if(sleft) {  // LEFT
      Serial.print("ST,0,RETINIT,ERR_OP,ED\r\n"); // OPPOSITE SIDE INIT
      stopMotor();
      delay(_step_delay);

      motorStep(true, SENSOR_BACK_STEP); // Rotate 8 degrees CW
   
      delay(_step_delay);
      stepOverCheck = 500000;
      do {
        motorStep(true, 1); // Rotate CW until left sensor is interrupted
        stepOverCheck--;
        if(stepOverCheck < 0) {
          Serial.print("ST,0,RETINIT,ERR-OV-CW,ED\r\n");
          return;
        }
        sleft = digitalRead(LEFT_SENSOR_PIN) == LOW;
        sright = digitalRead(RIGHT_SENSOR_PIN) == LOW;

        if(sright){ 
          stopMotor();
          delay(1000);
          motorStep(true, SENSOR_BACK_STEP); // Rotate 8 degrees CW
          delay(_step_delay);
          break;
        }
      } while(sright == false);
    }

    if(sright)  break;

  } while(sleft == false && sright == false); //inner loop

  stopMotor();
  delay(_step_delay);
  
  _step_start_position = 0;
  _step_current_position = 0;

  // loop to rotate in CCW after SRIGHT interruption
  stepOverCheck = 500000;
  do {
    motorStep(false, 1); // Rotate CCW until right sensor is interrupted
    stepOverCheck--;
    if(stepOverCheck < 0) {
      Serial.print("ST,0,RETINIT,ERR-OV-CW,ED\r\n");
      _step_delay = old_step_delay;
      return;
    }    
    sleft = digitalRead(LEFT_SENSOR_PIN) == LOW;
    sright = digitalRead(RIGHT_SENSOR_PIN) == LOW;
  } while(sright == false);

  stopMotor();
  delay(_step_delay);
  motorStep(true, SENSOR_BACK_STEP); // Rotate 8 degrees CW
  delay(_step_delay);

  // Last loop
  stepOverCheck = 500000;
  do {
    motorStep(true, 1); // Rotate CW until left sensor is interrupted
    stepOverCheck--;
    if(stepOverCheck < 0) {
      //Serial.print("ST,0,RETINIT,ERR-OV-CW,ED\r\n");
      return;
    }
    sleft = digitalRead(LEFT_SENSOR_PIN) == LOW;
    sright = digitalRead(RIGHT_SENSOR_PIN) == LOW;

    if(sleft){
      stopMotor();
      delay(1000);
      motorStep(false, SENSOR_BACK_STEP); // Rotate 8 degrees CCW
      delay(1000);
      stopMotor(); //ST,0,MOVE,0,125,50,ED\r\n    
      break; //ST,0,INIT,0,ED\r\n
    }
  } while(sleft == false);

  _step_end_position = _step_current_position;

  float angleTotal = (_step_end_position - _step_start_position) * STEP_ANGLE;

  Serial.print("ST,0,RETINIT," + String(angleTotal) + ",ED\r\n");
  _step_delay = old_step_delay;
}

void status() {
  String status;
  int angle = 0;

  switch (_motorState) {
    case MotorState::STOPPED:
      status = "Stopped";
      break;
    case MotorState::MOVING_CW:
      status = "Moving CW";
      break;
    case MotorState::MOVING_CCW:
      status = "Moving CCW";
      break;
  }

  Serial.print("ST,");
  Serial.print(status);
  Serial.print(",");
  Serial.print(angle);
  Serial.print(",");
  Serial.print(_step_delay);
  Serial.print(",ED \r\n");
}

void ionizerLamp(bool state) {
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
}

void processCommand(String commandStr) {
  commandStr.trim();

  String cmd = getValue(commandStr, ',', 2);

  // Check for serial connection status
  if (cmd.equals("OK")) {
    Serial.print("ST,0,RETOK,OK,ED\r\n");
    return;
  }

  // Initial Mortor
  if (cmd.equals("INIT")) {
    initializeMotor();
    return;
  }
  
  // Send Status
  if (cmd.equals("STATUS")) {
    status();
    return;
  }
  
  // ST,0,MOVE,0,100,100,ED\r\n  
  //ST,0,INIT,0,ED\r\n   
  // Move Motor
  if (cmd.equals("MOVE")) {
    int angle = getValue(commandStr, ',', 4).toInt();
    int speedPercent = getValue(commandStr, ',', 5).toInt();
    moveMotor(angle, speedPercent);
    return;
  }

  // Lamp Control
  if (cmd.equals("LAMP")) {
    int lampState = getValue(commandStr, ',', 3).toInt();
    ionizerLamp(lampState == 1);
    Serial.print("ST,0,RETLAMP,OK," + String(lampState) + ",ED\r\n");
    return;
  }
}

void loop() {
  if (Serial.available() > 0) {
    String data = Serial.readString();
    processCommand(data);
  }
}
