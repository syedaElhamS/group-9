/**
   This sketch was created to control the robot car, initialize the serials attached on the car as the raspberry pi, sensors and bluetooth module.
   There are data exchange between the pi and the mobile application (Andriod code).The version described below 
   were not related to sprints, they are related to versions delivered.
    
   @author - Nina (Version 1), Elham (Version 1), Laiz (Version 1, 2, 4 and Final) and Rema (Version 2, 3 and 4)
   @editor - Elham and Rema: Bluetooth connection with mobile application 
   @editor - Elham: fix code for identify red object and fix Serial connection bug 
   @editor - Isak: Serial3 connection with the application when the car faces an obstacle in order to prompt the user for a new command.
   @editor - Kosara: Serial connection with the raspberry pi and the car in order to send and receive data for the Identify red object feature.
   @editor - Nina: Serial connection with the pi to send the data when the red object is faced to the App.
   @editor - Melinda and Nina: expanded the methods for registering front and back obstacle to fit the needs of the app.
   
**/

#include<Smartcar.h>

/*===============================================
              Hardware initialization
  ===============================================
*/
SR04 sensorFront;
SR04 sensorBack;
Gyroscope gyro(6);
Car car;

/*===============================================
            Pin numbers initialization
  ===============================================
*/
const int TRIGGER_PIN_F = A15; // <---- the number of the ultrasound sensor pin for the front
const int ECHO_PIN_F = A14;    // <---- the number of the ultrasound sensor pin for the front
const int TRIGGER_PIN_B = 45; // <---- the number of the ultrasound sensor pin for the back
const int ECHO_PIN_B = 44;    // <---- the number of the ultrasound sensor pin for the back
const int ledRight = 48;      // <---- the number of the LED pin
const int ledLeft = 49;       // <---- the number of the LED pin

/*===============================================
            Variables initialization
  ===============================================
*/
char input = 0;                     // <---- for the bluetooth connection
char output = 0;                    // <---- for the bluetooth connection
char outputN= 0;
int val = 0;
char piInput = 0;                   // <---- for the pi connection
unsigned int tempSpeed = 0;         // <---- for setting the velocity
int ledStateLeft = LOW;             // <---- led state used to set the LED
int ledStateRight = LOW;            // <---- led state used to set the LED
unsigned long intervalLeft = 1000;  // <---- interval to blink (milliseconds)
unsigned long intervalRight = 1000; // <---- interval to blink (milliseconds)
unsigned long blinkDuration = 500;  // <---- number of millisecs that Led's are on - all three leds use this
unsigned long currentMillis = 0;    // <---- stores the value of millis() in each iteration of loop()
unsigned long previousMillisL = 0;  // <---- to store last time LED at the left side was updated
unsigned long previousMillisR = 0;  // <---- to store last time LED at the right side was updated
unsigned int distanceObF = 0;       // <---- to verify the distance the front sensor are from the obstacle
unsigned int distanceObB = 0;       // <---- to verify the distance the back sensor are from the obstacle
boolean goAuto1 = false;            // <---- boolean value to verify the mode selected
boolean canDriveForward = true;     // <---- boolean value to allow the car to move in the oposite direction
boolean canDriveBackward = true;    // <---- boolean value to allow the car to move in the oposite direction

/*===============================================
                    SETUP
  ===============================================
*/
void setup() {

  /* Initialize the Bluetooth serial */
  Serial3.begin(9600);        // <--  Opens serial port to the App, set data rate to 9600 bps
  Serial.begin(9600);         // <--  Opens serial port to the Pi, set data rate to 9600 bps
  
  /* Check if the front and back sensor and the gyroscope are attached to the Pin */
  sensorFront.attach(TRIGGER_PIN_F, ECHO_PIN_F);
  sensorBack.attach(TRIGGER_PIN_B, ECHO_PIN_B);
  gyro.attach();

  /* Initialize the car with the gyroscope */
  car.begin(gyro);

  /* Initialize the digital pin as an output */
  pinMode(ledLeft, OUTPUT);
  pinMode(ledRight, OUTPUT);

}

/*===============================================
                    STATE
  ===============================================
*/
void loop() {

  currentMillis = millis(); // <--  Get the time while it is on and store on the variable
  checkSerialInput();       // <--  Get input from blutooth
  modeSelection();          // <--  Get the mode change (with bluetooth input)

  /* Enter this section when in autonomous mode */
  if (goAuto1 == true) {

    if (obstacleFront()) {  //< -- Always check for obstacle and act accordingly
      turnRight();
    }

    moveCar();        // <-- Car is always moving unless there is an obstacle in front

    /* Enter this section when in manual mode */
  } else {

    /* The car proccess the commands from user but stops in case of obstacle */
    delay(1000);
    obstacleF();            // <-- Check allways the obstacle in the front
    obstacleB();            // <-- Check allways the obstacle in the back

  }
}

/*===============================================
                    LIGHTS
  ===============================================
*/

/* Method to turnOn the right light */
void blinkRight() {

  if (ledStateRight == LOW) {     // <-- Check if the right led is off and set it to on
    if (currentMillis - previousMillisR >= intervalRight) {
      ledStateRight = HIGH;
      previousMillisR += intervalRight;
    }
  } else {        // <-- Check if the right led is on and set it to off
    if (currentMillis - previousMillisR >= blinkDuration) {
      ledStateRight = LOW;
      previousMillisR += blinkDuration;
    }
  }
  digitalWrite(ledRight, ledStateRight);  // <-- Send the values to the digital pin
}

/* Method to turnOn the left light */
void blinkLeft() {

  if (ledStateLeft == LOW) {      // <-- Check if the left led is off and set it to on
    if (currentMillis - previousMillisL >= intervalLeft) {
      ledStateLeft = HIGH;
      previousMillisL += intervalLeft;
    }
  } else {        // <-- Check if the left led is on and set it to off
    if (currentMillis - previousMillisL >= blinkDuration) {
      ledStateLeft = LOW;
      previousMillisL += blinkDuration;
    }
  }
  digitalWrite(ledLeft, ledStateLeft); // <-- Send the values to the digital pin
}

/* Method to turn Off both lights */
void blinkOff() {

  ledStateLeft = LOW;   // <-- Set off the right led
  ledStateRight = LOW;  // <-- Set off the right led
  digitalWrite(ledLeft, ledStateLeft);   // <-- Send the values to the digital pin
  digitalWrite(ledRight, ledStateRight); // <-- Send the values to the digital pin
}

/* Method to make both lights blink 4 times */
void blinkAlert() {

  blinkLeft();    // <-- Calling the methods for blink the left lights
  blinkRight();   // <-- Calling the methods for blink the right lights
}

/*===============================================
                    TURNS
  ===============================================
*/

/* Method to make the car turn right and blink the right light when facing an obstacle */
void turnRight() {
  blinkRight();    // <-- Calling the method to blink the right led
  car.rotate(84);  // <-- Make the car rotate to avoid the obstacle
  blinkOff();      // <-- Calling the method for turn off both leds
  delay(100);
  stopCar();       // <-- Calling the method for stopping the car
}

/* Method to make the car turn right + blink the right light */
void turnRightM() {

  blinkRight();    // <-- Calling the method to blink the right led
  car.rotate(84);  // <-- Make the car rotate to the right
  blinkOff();      // <-- Calling the method for turn off both leds
  delay(1000);
  stopCar();       // <-- Calling the method for stopping the car
}

/* Method to make the car turn left + blink the left light */
void turnLeftM() {

  blinkLeft();      // <-- Calling the method to blink the left led
  car.rotate(-84);  // <-- Make the car rotate to the left
  blinkOff();       // <-- Calling the method for turn off both leds
  delay(1000);
  stopCar();        // <-- Calling the method for stopping the car
}

/*===============================================
                    MOVEMENT
  ===============================================
*/

/* Method to make the car move given a speed */
void moveCar() {

  car.setMotorSpeed(50, 50);
}

/* Method to make the car stop */
void stopCar() {
 
  car.stop();
  delay(100);     // <-- The API documentation requires a 100 ms delay (Thats what I understood :P )
}

/* Method to make the car go backwards */
void goBack() {

  car.setMotorSpeed(-50, -50);  //<-- Just set the speed but in reverse
}
/*===============================================
                    OBSTACLES
  ===============================================
*/

/* Check for front obstacles */
void obstacleF() {

  if (obstacleFront()) {      // <-- If there is an obstacle in the front of the car, don't allow the car to moves forward
    canDriveForward = false;
  } else {                    // <-- If there isn't an obstacle in the front of the car, allow the car to moves forward
    canDriveForward = true;
  }
}

/* Check for back obstacles */
void obstacleB() {

  if (obstacleBack()) {       // <-- If there is an obstacle in the back of the car, don't allow the car to moves backward
    canDriveBackward = false;
  } else {                    // <-- If there isn't an obstacle in the back of the car, allow the car to moves backward
    canDriveBackward = true;
  }
}

/* Checks the front sensor readings for obstacles, blink the light and stop the car */
boolean obstacleFront() {

  distanceObF = sensorFront.getDistance();
  if (distanceObF > 0 && distanceObF < 30) { // <-- If an obstacle in the front is found perform accordly
    blinkAlert();   // <-- Call the method to make the lights blink
    stopCar();      // <-- Call the method to stop the car

    output = 'c';
    Serial3.println(output); // <-- Send the value 'c' to the application

    blinkOff();     // <-- Call the method to stop blinking
    return true;
  }
  else {
    output = 'x';
    Serial3.println(output);
    return false;
  }  
}

/* Checks the back sensor readings for obstacles, blink the light and stop the car */
boolean obstacleBack() {
  
  distanceObB = sensorBack.getDistance();
  if (distanceObB > 0 && distanceObB < 30) { // <-- If an obstacle in the back is found perform accordly
    blinkAlert();   // <-- Call the method to make the lights blink
    stopCar();      // <-- Call the method to stop the car

    outputN = 't';
    Serial3.println(outputN); // <-- Send the value 't' to the application

    blinkOff();     // <-- Call the method to stop blinking
    return true;
  }
  else {
    outputN = 'u';
    delay(50);
    Serial3.println(outputN);
    return false;
  }
}

/*===============================================
                    MANUAL CONTROL
  ===============================================
*/

/* Proccess the input from the bluetooth */
void goManual() {

  /* Condiftions to perform movement in the car based on the user's input in the application */
  if (input == 'q') {       // <-- Check the user command to stop the car
    stopCar();
  }

  if (input == 'f') {       // <-- Check the user command to drive forwards
    if (canDriveForward) {  // <-- Perform an obstacle check before driving
      moveCar();
    }
  }

  if (input == 'b') {        // <-- Check the user command to drive backwards
    if (canDriveBackward) {  // <-- Perform an obstacle check before driving
      goBack();
    }
  }

  if (input == 'l') {        // <-- Check the user command to turn left
    turnLeftM();
  }
  if (input == 'r') {        // <-- Check the user command to turn right
    turnRightM();
  }
  if (input == 'j') {        // <-- Check the user command to blink the alert lights (Fun stuff)
    blinkAlert();
  }
}

/*===============================================
                    MODE SELECTION
  ===============================================
*/

/* Proccess the blutooth input for autonmous mode switching */
void modeSelection() {

  switch (input) {
    case 'a':            // <-- Selecting the autonomous mode (Robots will invade us :D)
      goAuto1 = true;
      break;

    case 's':            // <-- STATIC no movement (Toggles the autonmous mode off)
      stopCar();
      goAuto1 = false;
      break;

    case 'o':            // <-- Send 'o' to the Pi to idenfity red object
     val = 1;
      Serial.println(val);        // <-- To send '1' to the pi
      delay(2000);
      readSerial();               // <-- To receive the info from the pi
      delay(1500);
      Serial3.println(piInput);   // <-- To send the info to the app
      delay(1500);
      break;

    case 'w':            // <-- To break the identify red object
      break;

    default:             // <-- The manual mode is the default mode
      goManual();
  }
}

/*===============================================
                    BLUETOOTH
  ===============================================
*/
void checkSerialInput() {

  if (Serial3.available() > 0) { // <-- Get data only when bluetooth available
    input = Serial3.read();
  }
}

/*===============================================
                PI CONNECTION
  ===============================================
*/
void readSerial() {

  if (Serial.available() > 0) { // <-- Get data only when Serial port is available
    piInput = Serial.read();
  }
}
