// Built-in libraries
#include <Stepper.h>
#include <binary.h>
#include <EEPROM.h>

// 3rd-party libraries
//LIBARIES FOR LED AND SEVEN SEG DISPLAY
#include <LedControlMS.h>
#include <SevenSeg.h>



//LIBARIES FOR OLED DISPLAY
//#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
//
//#define SCREEN_WIDTH 128 // OLED display width,  in pixels
//#define SCREEN_HEIGHT 64 // OLED display height, in pixels
//
//// declare an SSD1306 display object connected to I2C
//Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);





//HERE WE LISTED THE IR SESNOR PINS
//int upButtons[] = {22, 23, 24, 25};   // Red buttons pin no
int upIRsensor[] = {22, 23, 24, 25};
// int irsensor[] = { 44, 45, 46, 47, 48, 49};

int upLights[] = {26, 27, 28, 29}; //LED lights indicattion
//Here IR sesnor for down side
//int downButtons[] = {30, 31, 32, 33};  // Green Buttons pin no.
int downIRsensor[] = {30, 31, 32, 33};  // Green Buttons pin no.
int downLights[] = {34, 35, 36, 37};

//int floorButtons[] = {44, 45, 46, 47, 48}; //Floor buttons
//Thses are IR sesnors inside the Elavator
int floorIRsensor[] = {44, 45, 46, 47, 48}; //Floor buttons
int floorLights[] = {49, 50, 51, 52, 53};

//Here we initilizing pins for stepper motor
Stepper mainMotor(4096, 9, 10, 11, 12);

SevenSeg floorDisplay(2, 3, 4, 5, 6, 7, 8); // Instantiate a seven segment controller object
LedControl directionMatrix = LedControl(39, 43, 41, 1); // (from left to right from the back: red/blk/yellow/brown/green)


// Logic constants
const int FLOORS = 3;
const int FLOOR_TIMER_START = 30000; // Number of loops to pause at a floor
const int STEPS_PER_TICK = 100;


const int UP = 1;//UpIR for up scalling // Red button for up scalling
const int IDLING = 0;
const int DOWN = -1; //DownIR for down scalling //green button for down scalling
const int BOTH = 2; // In case some floor requests both directions
const int EITHER = 3; // In case no call requests, but need to go there from inside

// Logic variables
int floorRequests[] = {IDLING, IDLING, IDLING, IDLING, IDLING};

//Saving the current data of Elevator
int currentDirection = IDLING;
int destinationFloor = -1;
int floorTimer = 0;



// HERE STRUCTURE USING FOR SAVING CURERRENT POSITION OF LIFT
struct PersistentData {
  int floorLevels[FLOORS];
  int currentPosition = 0; // (measured in steps)
  int currentFloor = 0;
} persistentData;






void setup() {
  Serial.begin(9600); // initializing serial monitor

  // // initialize OLED display with address 0x3C for 128x64
  //  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
  //    Serial.println(F("SSD1306 allocation failed"));
  //    while (true);
  //  }
  //
  //delay(2000);         // wait for initializing
  //  oled.clearDisplay(); // clear display
  //
  //  oled.setTextSize(1);          // text size
  //  oled.setTextColor(WHITE);     // text color
  //  oled.setCursor(0, 10);        // position to display
  //  oled.println("Hello World!"); // text to display
  //  oled.display();               // show on OLED


  mainMotor.setSpeed(4);//Here we describe the speed of the stepper motor side


  for (int i = 0; i < FLOORS - 1; i++) {
    //pinMode(upButtons[i], INPUT_PULLUP);
    pinMode(upIRsensor[i], INPUT); // takig input for upIR sensor
    // pinMode(downButtons[i], INPUT_PULLUP);
    pinMode(downIRsensor[i], INPUT); // taking input for downIR sensor
    //Initilizing LEDS
    pinMode(upLights[i], OUTPUT);
    pinMode(downLights[i], OUTPUT);
  }
  for (int i = 0; i < FLOORS; i++) {
    //pinMode(floorButtons[i], INPUT_PULLUP);
    //Inside Eleveator IR sensor
    pinMode(floorIRsensor[i], INPUT);
    pinMode(floorLights[i], OUTPUT);
  }

  floorDisplay.setCommonCathode();

  EEPROM.get(0, persistentData);
  floorDisplay.writeDigit(persistentData.currentFloor + 1);
  // oled.println(persistentData.currentFloor + 1); // text to display on OLED


  for (int i = 0; i < FLOORS; i++) {
    Serial.println("Loaded data:");
    Serial.print("Floor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(persistentData.floorLevels[i]);
  }
  Serial.print("Motor at: ");
  Serial.println(persistentData.currentPosition);

  //calling fuction for self testing
  selfTest();
}


void selfTest() {
  for (int i = 0; i < FLOORS - 1; i++) {
    Serial.print("Up ");
    Serial.println(i);
    quickLight(upLights[i]);
    Serial.print("Down ");
    Serial.println(i);
    quickLight(downLights[i]);
  }

  for (int i = 0; i < FLOORS; i++) {
    Serial.print("Floor ");
    Serial.println(i);
    quickLight(floorLights[i]);
  }

  for (int i = 0; i <= 9; i++) {
    Serial.print("Setting 7-segment to ");
    Serial.println(i);
    floorDisplay.writeDigit(i);//displaying data on seven segment
    //oled.println(i); // text to display on OLED
    delay(200);
  }
  
  floorDisplay.writeDigit(persistentData.currentFloor + 1);//displaying data on seven segment
  //oled.println(persistentData.currentFloor + 1); // text to display on OLED

  Serial.println("Up arrow");
  showUpArrow();
  delay(400);
  Serial.println("Down arrow");
  showDownArrow();
  delay(400);
  Serial.println("Clear arrows");
  turnOffArrows();
}

//Initilizing  function the led which show quick reaction on actvation IR sensor 
void quickLight(int pin) {
    digitalWrite(pin, HIGH);
    delay(200);
    digitalWrite(pin, LOW);
    delay(200);
}

void loop() {
    // If we're pausing at a floor, blink the corners of the display
    if (floorTimer != 0) {
        if ((floorTimer % 1000) == 0) {
            setMatrixCorners((floorTimer % 2000) == 0);
        }
        floorTimer--;
    }
    
    // If any call buttons have been pressed, register that and light their lights
    for (int i = 0; i < FLOORS - 1; i++) {
        //if (digitalRead(upButtons[i]) == HIGH)
        if (digitalRead(upIRsensor[i]) == HIGH){
            if (floorRequests[i] == IDLING || floorRequests[i] == UP) {
                floorRequests[i] = UP;
            } else {
                floorRequests[i] = BOTH;
            }
            digitalWrite(upLights[i], HIGH);
            calcDestination();
            printStatus("UP PRESSED");
        }
        //if (digitalRead(downButtons[i]) == HIGH) 
        if (digitalRead(downIRsensor[i]) == HIGH){
            if (floorRequests[i + 1] == IDLING || floorRequests[i + 1] == DOWN) {
                floorRequests[i + 1] = DOWN;
            } else {
                floorRequests[i + 1] = BOTH;
            }
            digitalWrite(downLights[i], HIGH);
            calcDestination();
            printStatus("DOWN PRESSED");
        }
    }
    

    // If any inside floor buttons have been pressed, register that and light their lights
    for (int i = 0; i < FLOORS; i++) {
        //if (digitalRead(floorButtons[i]) == HIGH)
        if (digitalRead(floorIRsensor[i]) == HIGH){
     
            if (i == 0 && digitalRead(floorIRsensor[4]) == HIGH) {
                // EASTER EGG! Calibration mode.
                enterCalibrationMode();
            } else {
                if (floorRequests[i] == IDLING) {
                    floorRequests[i] = EITHER;
                }
                digitalWrite(floorLights[i], HIGH);
            }
            calcDestination();
            printStatus("FLOOR PRESSED");
        }
    }

    // Go towards our destination
    if (floorTimer == 0 && currentDirection != IDLING) {
        if (currentDirection == UP) {
            mainMotor.step(STEPS_PER_TICK);
            persistentData.currentPosition++;
        } else {
            mainMotor.step(-STEPS_PER_TICK);
            persistentData.currentPosition--;
        }
        EEPROM.put(0, persistentData);

        // If we've reached a floor, open the door and turn off the appropriate lights. This would be
        // more efficient if we didn't loop, but rather, checked the "next expected" floor.
        for (int i = 0; i < FLOORS; i++) {
            if (persistentData.currentPosition == persistentData.floorLevels[i]) {
                floorDisplay.writeDigit(i + 1);
                persistentData.currentFloor = i;
                EEPROM.put(0, persistentData);
                Serial.print("At floor ");
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.println(persistentData.currentPosition);

                printStatus("AT FLOOR");
                
    
                // Check to see if someone wanted to go here; if so, stop
                if (floorRequests[i] == EITHER || floorRequests[i] == BOTH ||
                    (currentDirection == UP && floorRequests[i] == UP) ||
                    (currentDirection == DOWN && floorRequests[i] == DOWN) ||
                    i == destinationFloor) {
                    Serial.println("Stopping here!");
                    floorTimer = FLOOR_TIMER_START;

                    // Figure out which lights to turn off
                    digitalWrite(floorLights[i], LOW);
                    
                    if (currentDirection == UP) {
                        digitalWrite(upLights[i], LOW);
                        if (i == destinationFloor) {
                            digitalWrite(downLights[i - 1], LOW);
                        }
                        if (floorRequests[i] == BOTH) {
                            floorRequests[i] = DOWN;
                        } else {
                            floorRequests[i] = IDLING;
                        }
                    } else if (currentDirection == DOWN) {
                        digitalWrite(downLights[i - 1], LOW);
                        if (i == destinationFloor) {
                            digitalWrite(upLights[i], LOW);
                        }
                        if (floorRequests[i] == BOTH) {
                            floorRequests[i] = UP;
                        } else {
                            floorRequests[i] = IDLING;
                        }
                    }

                    calcDestination();
                    printStatus("AFTER STOP");
                }
                break; // Found a floor, no need to check the rest of them
            }
        }
    } else if (currentDirection == IDLING) {
        // No need to go anywhere
        powerOffStepper();
    }
}

void calcDestination() {
    destinationFloor = -1;

    switch (currentDirection) {
    case UP:
    case IDLING:
        for (int i = FLOORS - 1; i > 0; i--) {
            if (floorRequests[i]) {
                destinationFloor = i;
                break;
            }
        }
        break;

    case DOWN:
        for (int i = 0; i < FLOORS; i++) {
            if (floorRequests[i]) {
                destinationFloor = i;
                break;
            }
        }
        break;
    }

    if (destinationFloor == -1 || destinationFloor == persistentData.currentFloor) {
        // Nowhere to go
        currentDirection = IDLING;
        turnOffArrows();
    } else if (destinationFloor > persistentData.currentFloor) {
        currentDirection = UP;
        showUpArrow();
    } else if (destinationFloor < persistentData.currentFloor) {
        currentDirection = DOWN;
        showDownArrow();
    }
}


//Printing function which will help to print on the serial monitor 
void printStatus(const char *str) {
    Serial.print("REQUEST STATUS: ");
    Serial.println(str);
    for (int i = FLOORS - 1; i >= 0; i--) {
        Serial.print("Floor ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(floorRequests[i]);
    }
    Serial.print("Heading towards floor ");
    Serial.println(destinationFloor + 1);
}


//Operating function of the stepper motor 
void powerOffStepper() {
    digitalWrite(9, LOW);
    digitalWrite(10, LOW);
    digitalWrite(11, LOW);
    digitalWrite(12, LOW);
}


//Callibration of the Elevator 
void enterCalibrationMode() {
    showCalibrationMode();
    digitalWrite(floorLights[0], HIGH);
    digitalWrite(floorLights[FLOORS - 1], HIGH);
    persistentData.currentPosition = 0;
    
    for (int i = 0; i < FLOORS; i++) {
        floorDisplay.writeDigit(i + 1);
        do {
            if (digitalRead(floorIRsensor[0]) == HIGH) {
                mainMotor.step(-STEPS_PER_TICK);
                persistentData.currentPosition--;
            } else if (digitalRead(floorIRsensor[FLOORS - 1]) == HIGH) {
                mainMotor.step(STEPS_PER_TICK);
                persistentData.currentPosition++;
            }
        } while (digitalRead(floorIRsensor[2]) != HIGH);
        persistentData.floorLevels[i] = persistentData.currentPosition;
        Serial.print("Floor ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(persistentData.currentPosition);

        delay(5000); // Allow button to debounce before moving on to next floor
    }
    digitalWrite(floorLights[0], LOW);
    digitalWrite(floorLights[FLOORS - 1], LOW);
    directionMatrix.clearDisplay(0);
    persistentData.currentFloor = FLOORS - 1;
    EEPROM.put(0, persistentData);

    for (int i = 0; i < FLOORS; i++) {
        floorRequests[i] = IDLING;
    }
}

//This only works in seven segment display 
//here we write function display down arrow and uparrow on seven segment display
/* 
 *  Matrix functions. Note that items are mirror-image left-to-right
 */
void showUpArrow() {
    directionMatrix.setColumn(0, 0, B00011000);
    directionMatrix.setColumn(0, 1, B00111100);
    directionMatrix.setColumn(0, 2, B01011010);
    directionMatrix.setColumn(0, 3, B10011001);
    directionMatrix.setColumn(0, 4, B00011000);
    directionMatrix.setColumn(0, 5, B00011000);
    directionMatrix.setColumn(0, 6, B00011000);
    directionMatrix.setColumn(0, 7, B00011000);
}

void showDownArrow() {
    directionMatrix.setColumn(0, 0, B00011000);
    directionMatrix.setColumn(0, 1, B00011000);
    directionMatrix.setColumn(0, 2, B00011000);
    directionMatrix.setColumn(0, 3, B00011000);
    directionMatrix.setColumn(0, 4, B10011001);
    directionMatrix.setColumn(0, 5, B01011010);
    directionMatrix.setColumn(0, 6, B00111100);
    directionMatrix.setColumn(0, 7, B00011000);
}

void showCalibrationMode() {
    directionMatrix.setColumn(0, 0, B00000000);
    directionMatrix.setColumn(0, 1, B00111100);
    directionMatrix.setColumn(0, 2, B01000010);
    directionMatrix.setColumn(0, 3, B00000010);
    directionMatrix.setColumn(0, 4, B00000010);
    directionMatrix.setColumn(0, 5, B01000010);
    directionMatrix.setColumn(0, 6, B00111100);
    directionMatrix.setColumn(0, 7, B00000000);
}

void setMatrixCorners(boolean value) {
    directionMatrix.setLed(0, 0, 0, value);
    directionMatrix.setLed(0, 0, 7, value);
    directionMatrix.setLed(0, 7, 0, value);
    directionMatrix.setLed(0, 7, 7, value);    
}

void turnOffArrows() {
    directionMatrix.clearDisplay(0);
}
