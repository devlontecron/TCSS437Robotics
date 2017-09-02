#pragma config(Sensor, S1,     ColorL,         sensorEV3_Color)
#pragma config(Sensor, S2,     ColorR,         sensorEV3_Color)
#pragma config(Sensor, S4,     Sonar,          sensorEV3_Ultrasonic)

//Sensor defaults and thresholds
#define BLACK 1
#define WHITE 0
#define COLOR_THRESH 2
#define SONAR_THRESH 90
#define SONAR_ADJUST 3
#define AVG_NUM 5
#define AVG_WEIGHT 0.5
#define OUTLIER_THRESH 4

//Driving/Turning configuration
#define TURN_AMOUNT 5
#define DEFAULT_SPEED 25
#define EOL_TURN_START -29
#define EOL_STRAIGHT -25
#define EOL_TURN_LEFT -0
#define EOL_TURN_BACK 25
#define EOL_TURN_RIGHT 48
#define EOL_TURN_END 73

//Driving modes
#define WANDER 0
#define LINE 1
#define ATTACK 2
#define END_OF_LINE 3
#define ATTACK_STOP 4


//Global vars for communicating between threads and their initial values
int leftColor = WHITE;
int rightColor = WHITE;
float sonarDistance = 0;
int sensorFlag = WANDER;
float sonarAvg = 0;
int eol = EOL_TURN_START - 1;

/**
Display the values of the sensors on the display
*/
void displaySensors() {
	displayBigTextLine(1, "Sonar    %dcm", sonarDistance);
	displayBigTextLine(4, "LeftLight   %d", leftColor);
	displayBigTextLine(7, "RightLight  %d", rightColor);
	displayBigTextLine(10, "EOL  %d", eol);
}

/**
Take the raw sensor input and decide whether that
means the sensor is detecting white or black
*/
int parseRawColor(int raw) {
	if(raw <= COLOR_THRESH) {
		return BLACK;
	} else {
		return WHITE;
	}
}

/**
Take the raw sonar input and ensure that there are
not outlier values and get a rolling average of the
last values.
*/
float parseRawSonar(float raw) {

	//Put less importance (alpha) on raw values that may be an outlier
	//and higher on values closer to the average.
	float alpha = 0.5;
	if(raw - OUTLIER_THRESH > sonarAvg || raw + OUTLIER_THRESH < sonarAvg) {
		alpha = 0.05;
	}

	//calculate moving weighted average
	sonarAvg = sonarAvg + (alpha * (raw - sonarAvg - SONAR_ADJUST));

	//since the sonar has trouble sensing items too close,
	//assume anything below that distance is a zero
	if( sonarAvg < 1.75) sonarAvg = 0;

	return sonarAvg;
}

/**
This task is responsible for controlling the motors based on the parsed sensor
data from the SensorTask. Inside the while loop, each item in the case statement
controls one iteration of the specified driving mode.

We did not break up each main control mode into their own functions because we
wanted to make it clear that each control mode only executes one iteration and
can be preempted at any time (except attack mode).
*/
task DriverTask(){
	srand(nSysTime);
	int speed = DEFAULT_SPEED;

	while(1){
		switch(sensorFlag){
			//Line following
			case LINE:
				setLEDColor(ledOrange);
				eol = EOL_TURN_START;

				if(leftColor && !rightColor){
					//right is white --> turn left to correct
					setMotorSpeed(motorA, TURN_AMOUNT + DEFAULT_SPEED);
					setMotorSpeed(motorD, 0);
				} else if(rightColor && !leftColor){
					//left is white --> turn right to correct
					setMotorSpeed(motorD, TURN_AMOUNT + DEFAULT_SPEED);
					setMotorSpeed(motorA, 0);
				} else if(leftColor && rightColor) {
					//black --> drive straight
					setMotorSpeed(motorD, DEFAULT_SPEED);
					setMotorSpeed(motorA, DEFAULT_SPEED);
				}
				break;

			//Detecting if a line has ended
			case END_OF_LINE:
				setLEDColor(ledOrangeFlash);

				//To check the end of line, the robot will turn right, go back to starting pt,
				//turn left, then back to starting point and continue wander. This state is all
				//determined by the value of 'eol'
				if(eol < EOL_STRAIGHT) {
					setMotorSpeed(motorD, DEFAULT_SPEED);
					setMotorSpeed(motorA, DEFAULT_SPEED);
				} else if(eol < EOL_TURN_LEFT) {
					setMotorSpeed(motorD, DEFAULT_SPEED);
					setMotorSpeed(motorA, -10);
				} else if(eol < EOL_TURN_BACK) {
					setMotorSpeed(motorD, -DEFAULT_SPEED);
					setMotorSpeed(motorA, 10);
				} else if(eol < EOL_TURN_RIGHT) {
					setMotorSpeed(motorD, -10);
					setMotorSpeed(motorA, DEFAULT_SPEED);
				} else {
					setMotorSpeed(motorD, 10);
					setMotorSpeed(motorA, -DEFAULT_SPEED);
				}
				sleep(75);
				eol++;

				//play sound if done, as required
				if(eol == EOL_TURN_END){
					playSound(soundBeepBeep);
				}
				break;

			//Move close to an object detected by sonar
			case ATTACK:
				setLEDColor(ledRed);
				eol = EOL_TURN_START - 1;

				//Set speed according to distance to object
				motor[motorA] = sonarDistance * 2;
				motor[motorD] = sonarDistance * 2;
				break;

			//Make sounds and do requirements for after it
			//has stopped close to the object
			case ATTACK_STOP:
				setLEDColor(ledRedFlash);

				//stop, wait, backup, turn around
				motor[motorA] = 0;
				motor[motorD] = 0;
				sleep(2000);
				motor[motorA] = -DEFAULT_SPEED;
				motor[motorD] = -DEFAULT_SPEED;
				sleep(1000);
				motor[motorA] = DEFAULT_SPEED;
				motor[motorD] = -DEFAULT_SPEED;
				sleep(2200);
				break;

			//Wander mode
			default:
				setLEDColor(ledGreen);
				eol = EOL_TURN_START - 1;

				//choose random-biased direction
				speed = abs(rand() % 20) + DEFAULT_SPEED;
				setMotorSpeed(motorA, speed);
				setMotorSpeed(motorD, ((2 * DEFAULT_SPEED + 20) - speed));
				sleep(400);
		}
	}
}

/*
This task is responsible for gather the raw data, parsing it
and determing what mode the robot should be in for driving.
*/
task SensorTask(){
	while(1){

		//parse the raw values from the sensors
		leftColor = parseRawColor(getColorReflected(S1));
		rightColor = parseRawColor(getColorReflected(S2));
		sonarDistance = parseRawSonar(getUSDistance(S4));

		//determine driving mode
		if (sonarDistance < SONAR_THRESH && sonarDistance > 0 && sensorFlag != ATTACK_STOP){
			sensorFlag = ATTACK;
		} else if (sonarDistance == 0) {
			sensorFlag = ATTACK_STOP;
		} else if(leftColor || rightColor){
			sensorFlag = LINE;
		} else if (eol >= EOL_TURN_START && eol < EOL_TURN_END) {
			sensorFlag = END_OF_LINE;
		} else {
			sensorFlag = WANDER;
		}

		//display the sensors on the display
		displaySensors();
	}
}

/**
The main start for the ROBOTC
*/
task main(){

	//Get the moving weighted average for the sonar started
	for(int k = 0; k<10; k++){
		parseRawSonar(0);
	}

	startTask(DriverTask);
	startTask(SensorTask);

	//stay awake by keep main from exiting
	while(1) {
		sleep(1000);
	}
}
