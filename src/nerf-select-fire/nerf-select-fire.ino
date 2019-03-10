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
#include <splash.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <SPI.h>
#include <Wire.h>
#include "nerfLogo.h"

//pins
#define TOGGLE_FIRE_MODES_BTN_PIN 6                                       //digital inpit
#define TRIGGER_PIN 7                                                    //digital input
#define MOTOR_OUTPUT_PIN 8                                                //digital output
#define SDA_PIN 4
#define SCL_PIN 5

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
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
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
  delay(2000);
  display.clearDisplay();
}

void loop () {
  toggleFireModes();																										//constantly check for changes in firemodes
  processTriggerAction();																					      //Process any trigger actions
  updateDisplay();
}

//Toggle between Fire Modes
void toggleFireModes () {
  toggleFireModesBtn.read();																							//read button
  if (toggleFireModesBtn.wasPressed()) {																	//check if it was pressed
    fireMode = ((fireMode == 3) ? 0 : fireMode + 1);    									//increment fireMode
    //Serial.print("Firemode: ");
    Serial.println("Firemode: " + String(fireMode));
    resetDartsFired();																										//reset darts fired stuff so it doesn't get messed up later
  }
}

// Determine trigger action
void processTriggerAction () {
  trigger.read();                                                        //read trigger
  if (!trigger.isPressed()) {                                           //check of trigger is pressed
    if (fireMode == SAFETY) {                                         //if in safety mode, turn off motor
      stopMotor();
    } else if (fireMode == SINGLE_FIRE || fireMode == BURST_FIRE) {    //if in burst fire or single shot mode
      fireNonAutoDarts();                               //allow for darts to be fired, handled elsewhere
    } else if (fireMode == FULL_AUTO) {                               //if full auto turn on motor
      fireDart();
    }
  } else if (!trigger.wasPressed()) {                                   //if trigger isn't pressed
    if (fireMode == FULL_AUTO || fireMode == SAFETY) {                //if firemode is fullauto or safety, turn off motor
      stopMotor();
//      updateDisplay();
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
//  updateDisplay();
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
String nameOfFireMode () {
  switch (fireMode)
  {
    case SAFETY:
      return "SAFETY";
    case SINGLE_FIRE:
      return "SINGLE";
    case BURST_FIRE:
      return "BURST";
    case FULL_AUTO:
      return "FULL";
  }
}

void updateDisplay() {
//  display.clear();
//  display.setFont(ArialMT_Plain_16);
//  display.setTextAlignment(TEXT_ALIGN_CENTER); //// TEXT_ALIGN_LEFT, TEXT_ALIGN_CENTER, TEXT_ALIGN_RIGHT, TEXT_ALIGN_CENTER_BOTH
//  display.drawString(0, 0, "Dart Counter");
//  display.setTextAlignment(TEXT_ALIGN_LEFT);
//  display.drawString(0, 16, "Darts Fired: " + String(dartsFired));
//  display.drawString(0, 32, "Fire Mode: " + String(nameOfFireMode()));
//  display.display();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setCursor(30,0);
  display.println("Meaker MK16");
  display.setTextSize(4);
  if (numberOfDartsToFire () == 0) {
    display.setCursor(30,20);
    display.println("---");
  } else {
    display.setCursor(55,20);
    if (totalDartsFired > 9)
      display.setCursor(40,20);
    display.println(totalDartsFired);
  }
  display.setTextSize(1);
  display.println("Fire Mode: " + String(nameOfFireMode()));
  display.display();
}
