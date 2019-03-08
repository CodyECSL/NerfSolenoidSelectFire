

/*----------------------------------------------------------------------*
 * Nerf Solenoid Select Fire                        			              *
 *                                                                      *
 * Nerf Solenoid Select Fire v0.1 by Cody                               *
 *                                                                      *
 * Original from Monty Choy, Nov 2017: 					                        *
 * https://github.com/mochoy/nerf-select-fire                           *
 *                                                                      *
 * Arduino sketch for saftey, single shot, three round burst, and full  *
 * auto.                                                    						*
 *----------------------------------------------------------------------*/

#include <JC_Button.h>																											//library to deal with buttons easier

//pins
#define IR_GATE_PIN 0																											//analog input
#define TOGGLE_FIRE_MODES_BTN_PIN 2                                       //digital inpit
#define TRIGGER_PIN 7                                                    //digital input
#define DART_COUNTER_SWITCH_PIN 11   																			//digital input
#define MOTOR_OUTPUT_PIN 12                                                //digital output

//for buttons/switches
#define PULLUP true        																								//internal pullup, so we dont need to wire resistor
#define INVERT true      																									//invert required for proper readings with pullup
#define DEBOUNCE_MS 20 																										//check btn time every 20ms

#define IR_GATE_TRIP 90																										//'trip' value for IR gate					

//code for fire modes. 4 modes total
#define SAFETY 0																													//SAFTEY is mode 0
#define SINGLE_FIRE 1																											//singe fire is mode 1
#define BURST_FIRE 2																											//burst fire is mode 2
#define FULL_AUTO 3																												//full auto is mode 3


byte fireMode = 0;   																											//keep track of fire modes. 
byte dartsFired = 0;																											//keep track of how many darts fire
bool isCheckingForDartsFired = false;																			//some modes need to check if a certain number of darts to fire

Button trigger (TRIGGER_PIN, PULLUP, INVERT, DEBOUNCE_MS);														//trigger button, using the library   
Button dartCountingSwitch (DART_COUNTER_SWITCH_PIN, PULLUP, INVERT, DEBOUNCE_MS);			//dart counting button, using the library
Button toggleFireModesBtn (TOGGLE_FIRE_MODES_BTN_PIN, PULLUP, INVERT, DEBOUNCE_MS);		//toggle fire modes button, using the librarys

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
}

void setup () {   
    Serial.begin(9600);
    pinMode(MOTOR_OUTPUT_PIN, OUTPUT);																		//set motor output pin to an output pin
    stopMotor();        													//make sure motor is off
    resetDartsFired();																										//reset all dart firing values so they dont get messed up later
}

void loop () {
    toggleFireModes();																										//constantly check for changes in firemodes
//    fire();																																//constantly check if dart is fired
//    checkForDartsFired();																									//do stuff if dart is fired
    selectFire();																													//do fancy select-fire stuff
}

//switch between the various modes
void toggleFireModes () {
	toggleFireModesBtn.read();																							//read button
	if (toggleFireModesBtn.wasPressed()) {																	//check if it was pressed
		fireMode = ((fireMode == 3) ? 0 : fireMode + 1);    									//increment fireMode
    Serial.print("Firemode: ");
    Serial.println(fireMode);
	  resetDartsFired();																										//reset darts fired stuff so it doesn't get messed up later
	}
}

//when dart fired
void fire() {
  dartCountingSwitch.read();																							//read button
  dartsFired += ( (isCheckingForDartsFired && 														//detect and keep track if dart is fired through
  	( (map(analogRead(IR_GATE_PIN), 0, 1023, 0, 100) > IR_GATE_TRIP) ||		//switch or IR gate. 
  	 dartCountingSwitch.wasPressed()) )
  	 ? 1 : 0);        
}

void checkForDartsFired () {						
  if (isCheckingForDartsFired && 																					//if checking for darts being fired. Not all 
   (fireMode == SINGLE_FIRE || fireMode == BURST_FIRE)) {									// modesneed to check if a dart is fired
    byte dartsToFire = (fireMode == SINGLE_FIRE ? 1 : fireMode == BURST_FIRE ? 3 : 50);									//determine max amounts of darts to be fired
    if (dartsFired < dartsToFire) {																				//if can still fire (hasn't reached threshold of
      fireDart();																//how many darts can fire), power pusher motor
    } else if (dartCountingSwitch.isPressed() && 													//if can't fire anymore darts and pusher 
     dartsFired >= dartsToFire) {																					//retracted
      resetDartsFired();																									//Reset darts fired stuff so it can happen again
    }
  }
}

void fireNonAutoDarts () {
  byte dartsToFire = numberOfDartsToFire();                  //determine max amounts of darts to be fired
  while (dartsFired < dartsToFire) {                                        //if can still fire (hasn't reached threshold of
    fireDart();                                                          //how many darts can fire), power pusher motor
  }
}

//do all the fancy select fire stuff
void selectFire () {
    trigger.read();																												//read trigger
    if (!trigger.isPressed()) {      																			//check of trigger is pressed
        if (fireMode == SAFETY) {       																	//if in safety mode, turn off motor
            stopMotor();													
        } else if (fireMode == SINGLE_FIRE || fireMode == BURST_FIRE) {    //if in burst fire or single shot mode
            fireNonAutoDarts();                               //allow for darts to be fired, handled elsewhere
        } else if (fireMode == FULL_AUTO) {     													//if full auto turn on motor
            fireDart();													
        }
    } else if (!trigger.wasPressed()) {    																//if trigger isn't pressed
        if (fireMode == FULL_AUTO || fireMode == SAFETY) {								//if firemode is fullauto or safety, turn off motor
            stopMotor();													
        } else if ( !isCheckingForDartsFired 															//if all darts fired
         && (fireMode == SINGLE_FIRE || fireMode == BURST_FIRE) ) {     	//and in burstfire 
        	resetDartsFired();																							//reset darts fired stuff
        }		
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
string nameOfFireMode () {
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

