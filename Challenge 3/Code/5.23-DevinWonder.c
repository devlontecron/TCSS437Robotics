#pragma config(Sensor, S1,     touchSensor,    sensorTouch)


//Sensor defaults and thresholds
#define BLACK 1
#define WHITE 0
#define COLOR_THRESH 2
#define SONAR_THRESH 90
#define SONAR_ADJUST 3
#define AVG_NUM 5
#define AVG_WEIGHT 0.5
#define OUTLIER_THRESH 4
#define FEAR_MAX 4
#define ENERGY_MAX 100

//Driving/Turning configuration
#define TURN_AMOUNT 5
#define DEFAULT_SPEED 25

//ENERGY STATES
#define ENERGY_FULL 3
#define ENERGY_HUNGRY 2
#define ENERGY_DANGER 1
#define ENERGY_DEAD 0

//DRIVING STATES
#define DEATH	0
#define AVOID_BUMPS 1
#define ESCAPE 2
#define APPROACH 3
#define FEED 4
#define GRADIENT_FOLLOW 5
#define WANDER 6

//Global vars for communicating between threads and their initial values
//processed inputs
int leftColor = WHITE;
int rightColor = WHITE;
int sonarDistance = SONAR_THRESH;
int motorState = WANDER;
int energyState = ENERGY_FULL;
bool taskComplete = true;

/**
Display the values of the sensors on the display
*/
void displaySensors(int energyValue, int fearValue, int drivingState) {

	string energy;
	if( energyState == ENERGY_FULL ) energy = "FULL";
	else if( energyState == ENERGY_HUNGRY) energy = "HUNG";
	else if( energyState == ENERGY_DANGER) energy = "DANG";
	else energy = "DEAD";

	string state;
	switch(drivingState){
		//Line following
	case DEATH:
		state = "DEAD";
		break;
	case AVOID_BUMPS:
		state = "BUMP";
		break;
	case ESCAPE:
		state = "ESCP";
		break;
	case APPROACH:
		state = "APPR";
		break;
	case FEED:
		state = "FEED";
		break;
	case GRADIENT_FOLLOW:
		state = "GRAD";
		break;
	default:
		state = "WAND";
		break;
	}

	nxtDisplayBigTextLine(0, "E: %s", energy);
	nxtDisplayBigTextLine(2, "   %d", energyValue);
	//nxtDisplayBigTextLine(4, "F: %d", fearValue);
	nxtDisplayBigTextLine(6, "S: %s", state);
}

/**
Take the raw sensor input and decide whether that
means the sensor is detecting white or black
*/
int parseRawColor(int raw) {

	//TODO Verify with NXT

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
float parseRawSonar(float raw, float prev) {

	//TODO Verify with NXT
	int avg = prev;

	//Put less importance (alpha) on raw values that may be an outlier
	//and higher on values closer to the average.
	float alpha = 0.5;
	if(raw - OUTLIER_THRESH > avg || raw + OUTLIER_THRESH < avg) {
		alpha = 0.05;
	}

	//calculate moving weighted average
	avg = avg + (alpha * (raw - avg - SONAR_ADJUST));

	//since the sonar has trouble sensing items too close,
	//assume anything below that distance is a zero
	if( avg < 1.75) avg = 0;

	return avg;
}

task wonder(){
	while(1){
		int speed = abs(rand() % 20) + DEFAULT_SPEED;
		setMotor(motorA, speed);
		setMotor(motorB, ((2 * DEFAULT_SPEED + 20) - speed));
		sleep(500);
	}
}


task Meh() {
	int i = 0;
	while(1) {
		nxtDisplayBigTextLine(4, "I: %d", i);
		i = i + 1;
		sleep(100);
	}
}

task Fuckit() {
	int i = 0;
	while(1) {
		nxtDisplayBigTextLine(4, "I: %d", i);
		i = i - 1;
		sleep(100);
	}
}

/*
This task is responsible for gather the raw data, parsing it
and determing what mode the robot should be in for driving.
*/
task Sensors(){
	//unparsed/raw values
	int fearVal = FEAR_MAX;
	int energyVal = ENERGY_MAX;
	float sonarVal = 0;

	startTask(Meh);
	startTask(Fuckit);

	while(1){

		//determine driving mode
		if (SensorValue(touchSensor)) {
			motorState = AVOID_BUMPS;
			suspendTask(Meh);
			resumeTask(Fuckit);
			} else { //wander
			motorState = WANDER;
			suspendTask(Fuckit);
			resumeTask(Meh);
		}

		//display the sensors on the display
		displaySensors(energyVal, fearVal, motorState);
	}
}

task main()
{
	//Get the moving weighted average for the sonar started
	for(int k = 0; k<10; k++){
		//parseRawSonar(0);
	}

	startTask(Sensors);
	//startTask(Motors);

	//stay awake by keep main from exiting
	while(1) {
		sleep(1000);
	}
}
