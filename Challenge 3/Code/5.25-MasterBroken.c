#pragma config(Sensor, S1,touchSensor, sensorTouch)

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
#define FEAR_SENSOR_LIGHT 1
#define FEAR_SENSOR_DARK 0

//Driving/Turning configuration
#define TURN_AMOUNT 5
#define DEFAULT_SPEED 25
#define MAX_SPEED 100

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
int fearSensor =FEAR_SENSOR_DARK;
int motorState = WANDER;
int energyState = ENERGY_FULL;
int fearValue = FEAR_MAX;
bool taskComplete = true;


/////////////////////////////////////////////////////////////////
//                                                             //
//                      HELPER FUNCTIONS                       //
//                                                             //
/////////////////////////////////////////////////////////////////

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
			case WANDER:
				state = "WAND";
				break;
	}

	nxtDisplayBigTextLine(0, "E: %s", energy);
	nxtDisplayBigTextLine(2, "   %d", energyValue);
	nxtDisplayBigTextLine(4, "F: %d", fearSensor);
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
Check, read and assign the values sent from the slave brick
*/
void readBluetoothMessages() {

		//Something got through if the signals are not all zero
		if(messageParm[0] != 0 || messageParm[1] != 0 || messageParm[2] != 0)
		{
			sonarDistance = messageParm[0];
			fearSensor = messageParm[1];

			//Clear the messages, set them to 0
			ClearMessage();
		}
}

/////////////////////////////////////////////////////////////////
//                                                             //
//                  DRIVING & SENSING TASKS                    //
//                                                             //
/////////////////////////////////////////////////////////////////

task Escape(){
	if(time1[T1]<1000){
		fearValue= fearValue-1;
	}

	if(time1[T1]>4000){
		fearValue++;
	}

	clearTimer(T1);
	//(fearValue==4){
	while(1){
		motor[motorA]=0;
		motor[motorB]=0;
		//sleep(500);
		//motor[motorA]=abs(rand() % 20) + DEFAULT_SPEED;
		//motor[motorB]=-DEFAULT_SPEED;
		//sleep(1000);
		//motor[motorA]=MAX_SPEED;
		//motor[motorB]=MAX_SPEED;
		//sleep(4000);
		//}else{
		//keep track of last flash
		//reset timer
		//do nothing
	//}
	}
	//taskComplete = true;
}

task Wander(){
	while(1){
		int speed = abs(rand() % 20) + DEFAULT_SPEED;
		//setMotor(motorA, speed);
		motor[motorA] = speed;
		motor[motorB] = ((2 * DEFAULT_SPEED + 20) - speed);
		//setMotor(motorB, );
		sleep(500);
	}
}

/** Out of all the driving tasks, set only one to be
in control of the robot at a time (except for the
sensors tasks -- that is always running) */
void makeTaskExclusive(int taskID) {


	//Suspend all but the current task
	int i = 0;
	while( i <= WANDER) {
		if( i != taskID) {
			switch(i) {
				//case DEATH: suspendTask(Death); break;
				//case AVOID_BUMPS: suspendTask(Death); break;
				case ESCAPE: stopTask(Escape); break;
				//case APPROACH: suspendTask(Death); break;
				//case FEED: suspendTask(Death); break;
				//case GRADIENT_FOLLOW: suspendTask(Escape); break;
				case WANDER: suspendTask(Wander); break;
			}
		}
		i++;
	}

	//Resume the current task
	switch(taskID) {
		//case DEATH: resumeTask(Death); break;
		//case AVOID_BUMPS: resumeTask(AvoidBumps); break;
		case ESCAPE: startTask(Escape); break;
		//case APPROACH: resumeTask(Approach); break;
		//case FEED: resumeTask(Feed); break;
		//case GRADIENT_FOLLOW: resumeTask(Escape); break;
		case WANDER: resumeTask(Wander); break;
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

	//init all driving tasks
	//startTask(Meh);
	//startTask(Fuckit);
	startTask(Wander);
	startTask(Escape);

	//continuouly check sensors and control driving task exclusivity
	while(1){
		readBluetoothMessages();

		//determine driving mode
		if (fearSensor == FEAR_SENSOR_LIGHT) {
			taskComplete = false;
			motorState = ESCAPE;
			makeTaskExclusive(ESCAPE);
		} else { //wander
			motorState = WANDER;
			makeTaskExclusive(WANDER);
		}

		//show the sensors and state values on the display
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
