#pragma config(Sensor, S1,     ColorL,         sensorEV3_Color)
#pragma config(Sensor, S2,     ColorR,         sensorEV3_Color)
#pragma config(Sensor, S4,     Sonar,          sensorEV3_Ultrasonic)


#define BLACK 1
#define WHITE 0
#define COLORTHRES 4
#define SONARTHRES 90
#define SONARADJUST 3
#define AVGNUM 5
#define AVGWEIGHT 0.5

#define WANDER 0
#define LINE 1
#define ATTACK 2
#define ENDOFLINE 3
#define ATTACKSTOP 4

#define TURNAMOUNT 5
#define DEFAULTSPEED 25
#define EOLTURNSTART -15
#define EOLTURNEND 30
#define OUTLIERTHRESH 7


//global vars for communicating between threads
int leftColor = WHITE;
int rightColor = WHITE;
float sonarDistance = 0;
int sensorFlag = WANDER;
float sonarAvg = 0;

int eol = -5;

void displaySensors() {
	displayBigTextLine(1, "Sonar    %dcm", sonarDistance);
	displayBigTextLine(4, "LeftLight   %d", leftColor);
	displayBigTextLine(7, "RightLight  %d", rightColor);
	displayBigTextLine(10, "EOL  %d", eol);
}

void lineFollow(){
	if(leftColor && !rightColor){
		setMotorSpeed(motorA, TURNAMOUNT + DEFAULTSPEED);
		setMotorSpeed(motorD, 0);
		} else if(rightColor && !leftColor){
		setMotorSpeed(motorD, TURNAMOUNT + DEFAULTSPEED);
		setMotorSpeed(motorA, 0);
		} else if(leftColor && rightColor) {
		//if both are black
		setMotorSpeed(motorD, DEFAULTSPEED);
		setMotorSpeed(motorA, DEFAULTSPEED);
	}
}

void endOfLine() {
	if( eol < 0 ) {
		setMotorSpeed(motorD, DEFAULTSPEED);
		setMotorSpeed(motorA, -DEFAULTSPEED + 5);
		} else {
		setMotorSpeed(motorD, -DEFAULTSPEED + 5);
		setMotorSpeed(motorA, DEFAULTSPEED);
	}
	sleep(50);

	eol++;
	if(eol == EOLTURNEND){
		playSound(soundBeepBeep);
	}
}

void attackMode(){
	motor[motorA] = sonarDistance * 2;
	motor[motorD] = sonarDistance * 2;
}

int parseRawColor(int raw) {
	if(raw <= COLORTHRES) {
		return BLACK;
		} else {
		return WHITE;
	}
}

float parseRawSonar(float raw) {

	float alpha = 0.8;
	if(raw - OUTLIERTHRESH > sonarAvg || raw + OUTLIERTHRESH < sonarAvg) {
		alpha = 0.1;
	}
	//alpha = 0 ignore new value
	//alpha = 1 ignore old value

	sonarAvg = sonarAvg + (alpha*(raw - sonarAvg - SONARADJUST));

	//handle stuff thats too close to call
	if( sonarAvg < 1.75) sonarAvg = 0;

	return sonarAvg;
}

task RyanG(){
	srand(nSysTime);
	int speed = DEFAULTSPEED;

	while(1){
		switch(sensorFlag){
		case LINE:
			setMotorSpeed(motorB, 50);
			eol = EOLTURNSTART;
			setLEDColor(ledOrange);
			lineFollow();
			break;

		case ENDOFLINE:
			setLEDColor(ledOrangeFlash);
			endOfLine();
			break;
			//make sound after task

		case ATTACK:
			//sonar stuff
			setMotorSpeed(motorB, 100);
			eol = EOLTURNSTART - 1;
			setLEDColor(ledRed);
			attackMode();
			break;

		case ATTACKSTOP:
			//after sonarr has stopped stuff
			//sleep(613);
			setLEDColor(ledRedFlash);
			motor[motorA] = 0;
			motor[motorD] = 0;
			sleep(2000);
			motor[motorA] = -DEFAULTSPEED;
			motor[motorD] = -DEFAULTSPEED;
			sleep(1000);
			motor[motorA] = DEFAULTSPEED;
			motor[motorD] = -DEFAULTSPEED;
			sleep(2200);
			break;

		default:
			//wonder
			eol = EOLTURNSTART - 1;
			setLEDColor(ledGreen);
			setMotorSpeed(motorB, 10);
			speed = abs(rand() % 20) + DEFAULTSPEED;
			setMotorSpeed(motorA, speed);
			setMotorSpeed(motorD, ((2 * DEFAULTSPEED + 20) - speed));
			sleep(500); //handle this later
		}
	}
}

task Salsa(){
	while(1){
		leftColor = parseRawColor(getColorReflected(S1));
		rightColor = parseRawColor(getColorReflected(S2));
		sonarDistance = parseRawSonar(getUSDistance(S4));

		//Color stuff
		//TODO Order this so priorities work yo
		if (sonarDistance < SONARTHRES && sonarDistance > 0 && sensorFlag != ATTACKSTOP){
			sensorFlag = ATTACK;
			} else if (sonarDistance == 0) {
			sensorFlag = ATTACKSTOP;
			} else if(leftColor || rightColor){
			sensorFlag = LINE;
			} else if (eol >= EOLTURNSTART && eol < EOLTURNEND) {
			sensorFlag = ENDOFLINE;
			} else {
			sensorFlag = WANDER;
		}

		//sonar sesor
		//light sensors

		displaySensors();
	}
}

task main(){
	for(int k = 0; k<10; k++){
		parseRawSonar(0);
	}
	startTask(RyanG);
	startTask(Salsa);

	while(1) {
		//stay awake
		sleep(500);
	}
}
