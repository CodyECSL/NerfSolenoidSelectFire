/*----------------------------------------------------------------------*
   Nerf Solenoid Select Fire
 *                                                                      *
   Nerf Solenoid Select Fire v0.1 by Cody
 *                                                                      *
   Original from Monty Choy, Nov 2017:
   https://github.com/mochoy/nerf-select-fire
 *                                                                      *
   Arduino sketch for saftey, single shot, three round burst, and full
   auto.
  ----------------------------------------------------------------------*/

#include <JC_Button.h>																											//library to deal with buttons easier
#include <Adafruit_SSD1306.h>
//#include <Adafruit_GFX.h>
//#include <gfxfont.h>
#include "nerfLogo.h"

//pins
#define TOGGLE_FIRE_MODES_BTN_PIN 6                                       //digital inpit
#define TRIGGER_PIN 7                                                    //digital input
#define MOTOR_OUTPUT_PIN 8                                                //digital output
#define SDA_PIN A4                                                        // This is a placeholder as a note.  These defined pins don't get used in code.
#define SCL_PIN A5                                                        // This is a placeholder as a note.  These defined pins don't get used in code.

//for buttons/switches
#define PULLUP true        																								//internal pullup, so we dont need to wire resistor
#define INVERT true      																									//invert required for proper readings with pullup
#define DEBOUNCE_MS 20 																										//check btn time every 20ms			

//code for fire modes. 4 modes total
#define SAFETY 0																													//SAFTEY is mode 0
#define SINGLE_FIRE 1																											//singe fire is mode 1
#define BURST_FIRE 2																											//burst fire is mode 2
#define FULL_AUTO 3																												//full auto is mode 3

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     3 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

byte fireMode = 0;   																											//keep track of fire modes.
byte dartsFired = 0;																											//keep track of how many darts fire
byte totalDartsFired = 0;                                                 //keep track of total darts fired
bool isCheckingForDartsFired = false;																			//some modes need to check if a certain number of darts to fire

Button trigger (TRIGGER_PIN, PULLUP, INVERT, DEBOUNCE_MS);														//trigger button, using the library
Button toggleFireModesBtn (TOGGLE_FIRE_MODES_BTN_PIN, PULLUP, INVERT, DEBOUNCE_MS);		//toggle fire modes button, using the librarys

void setup () {
  Serial.begin(9600);
  pinMode(MOTOR_OUTPUT_PIN, OUTPUT);																		//set motor output pin to an output pin
  stopMotor();        													//make sure motor is off
  resetDartsFired();																										//reset all dart firing values so they dont get messed up later

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();   // clears the screen and buffer
  //http://javl.github.io/image2cpp/
  display.drawBitmap(20,17, nerfLogo, 90, 44, 1);
  display.display();
  delay(1000);
  display.clearDisplay();
  displayFullScreenUpdate();
}

void loop () {
  toggleFireModes();																										//constantly check for changes in firemodes
  processTriggerAction();																					      //Process any trigger actions
}

//Toggle between Fire Modes
void toggleFireModes () {
  toggleFireModesBtn.read();																							//read button
  if (toggleFireModesBtn.wasPressed()) {																	//check if it was pressed
    fireMode = ((fireMode == 3) ? 0 : fireMode + 1);    									//increment fireMode
    //Serial.print("Firemode: ");
    Serial.println("Firemode: " + String(fireMode));
    resetDartsFired();																										//reset darts fired stuff so it doesn't get messed up later
    displayUpdateSelectFireStatus();
    displayUpdateAmmoCount();
  }
}

// Determine trigger action
void processTriggerAction () {
  trigger.read();                                                        //read trigger
  if (!trigger.isPressed()) {                                           //check of trigger is pressed
    if (fireMode == FULL_AUTO) {                               //if full auto turn on motor
      fireDart();
      processTriggerAction();
    } else if (fireMode == SINGLE_FIRE || fireMode == BURST_FIRE) {    //if in burst fire or single shot mode
      fireNonAutoDarts();                               //allow for darts to be fired, handled elsewhere
    } else if (fireMode == SAFETY) {                                         //if in safety mode, turn off motor
      stopMotor();
    }
  } else if (!trigger.wasPressed()) {                                   //if trigger isn't pressed
    if (fireMode == FULL_AUTO || fireMode == SAFETY) {                //if firemode is fullauto or safety, turn off motor
      stopMotor();
//      displayFullScreenUpdate();
    } else if ( !isCheckingForDartsFired                              //if all darts fired
                && (fireMode == SINGLE_FIRE || fireMode == BURST_FIRE) ) {      //and in burstfire
      resetDartsFired();                                              //reset darts fired stuff
    }
  }
}

void runMotor() {
  digitalWrite(MOTOR_OUTPUT_PIN, HIGH);
}

void stopMotor() {
  digitalWrite(MOTOR_OUTPUT_PIN, LOW);
}

void fireDart() {
  runMotor();
  delay(30);
  stopMotor();
  delay(90);
  Serial.println("FiredDart");
  dartsFired++;
  totalDartsFired++;
  displayUpdateAmmoCount();
}

void fireNonAutoDarts () {
  byte dartsToFire = numberOfDartsToFire();                  //determine max amounts of darts to be fired
  while (dartsFired < dartsToFire) {                                        //if can still fire (hasn't reached threshold of
    fireDart();                                                          //how many darts can fire), power pusher motor
  }
}

void resetDartsFired () {
  stopMotor();																		//turn of motor
  dartsFired = 0;																													//darts fired set to 0
  isCheckingForDartsFired = false;																				//no longer checking if darts are being fired
}

int numberOfDartsToFire () {
  switch (fireMode)
  {
    case SAFETY:
      return 0;
    case SINGLE_FIRE:
      return 1;
    case BURST_FIRE:
      return 3;
    case FULL_AUTO:
      return 50;
  }
}

// This will be used for an Ammo Counter and Display in the future
String nameOfFireMode (int i) {
  switch (i)
  {
    case 0:
      return "SAFE";
    case 1:
      return "SEMI";
    case 2:
      return "BRST";
    case 3:
      return "FULL";
  }
}

void displayFullScreenUpdate() {
  displayUpdateBlasterName();
  displayUpdateAmmoCount();
  displayUpdateSelectFireStatus();
}

void displayUpdateBlasterName () {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setCursor(30,0);
  display.println("Meaker MK16");
  display.display();
}

void displayUpdateAmmoCount () {
  display.setTextColor(WHITE);
  display.setTextSize(4);
  display.fillRect(0, 16, 128, 32, BLACK);
  if (numberOfDartsToFire () == 0) {
    display.setCursor(30,16);
    display.println("---");
  } else {
    display.setCursor(55,16);
    if (totalDartsFired > 9)
      display.setCursor(40,16);
    display.println(totalDartsFired);
  }
  display.display();
}

void displayUpdateSelectFireStatus () {
  display.setTextSize(1);
  
  int cursorPositionX = 4;
  int cursorPositionY = 56;

  display.fillRect(0, cursorPositionY -1, 128, 9, WHITE);
  display.fillRect(0, cursorPositionY, 128, 7, BLACK);

  for (int i = 0; i < 4; i++) {
    display.setCursor(cursorPositionX, cursorPositionY);
    if (i == fireMode){
      display.fillRect(cursorPositionX - 5, cursorPositionY - 1, 32, 9, WHITE);
      display.setTextColor(BLACK);
    } else {
      display.setTextColor(WHITE);
    }
    display.print(String(nameOfFireMode(i)));
    cursorPositionX += 32;
  }
  display.display();
  Serial.println("Draw: SelectFireStatus");
}
