#pragma config(Sensor, S2,     rightBumper,    sensorTouch)
#pragma config(Sensor, S3,     leftBumper,     sensorTouch)

//Sensor defaults and thresholds
#define BLACK 1
#define WHITE 0
#define COLOR_THRESH 2
#define SONAR_THRESH 90
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
int fearSensor = FEAR_SENSOR_DARK;
bool leftBump = false;
bool rightBump = false;

int motorState = -1;
int energyState = ENERGY_FULL;
int fearValue = FEAR_MAX;

TSemaphore taskCompleteLock;
bool taskComplete = true;

int fearTimeMinute = 0;

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
	nxtDisplayBigTextLine(4, "F: %d", fearValue);
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

void stopMotors() {
	motor[motorA] = 0;
	motor[motorB] = 0;
}

// moveBackwards moves the robot backwards for a given amount of time.
// int power the power of the motors
// int time  the amount of time the motors will run.
void moveBackwards(int power, int time) {
	motor[motorA] = -1 * power;
	motor[motorB] = -1 * power;
	sleep(time);
}

// turnLeft turns the robot left for a given amount of time.
void turnLeft(int power, int time) {
	motor[motorA] = -1 * power;
	motor[motorB] = 1 * power;
	sleep(time);
}

// turnRight turns the robot right for a given amount of time.
void turnRight(int power, int time) {
	motor[motorA] = 1 * power;
	motor[motorB] = -1 * power;
	sleep(time);
}

//kill the robot when it runs out of energy. It must be sacrificed for the greater good.
void killRobot(){
	//sleep(100);
	//play sound
	playSound(soundDownwardTones);
	stopMotors();

	//sleep long enough to play the sound
	sleep(500);

	//stop everything
	stopAllTasks();
}

/** Check if the a "nextID" task can preempt the current running one.
The previous task must be completed or the next task is a higher priority*/
bool canPreempt(int nextID, bool prevTaskComplete) {
	if (prevTaskComplete) return true;
	else return nextID <= motorState;
}

/**
Get the next possible state */
int nextState(bool prevComplete) {

	//This is a workaround for a wierd corner case with the bumpers being
	//pressed multiple times that completely kills the robot
	if(motorState == AVOID_BUMPS && prevComplete) {
		leftBump = false;
		rightBump = false;
	}

	if( energyState == ENERGY_DEAD) {
		//DEATH - can't avoid it bro
		return DEATH;

	} else if (canPreempt(AVOID_BUMPS, prevComplete) && (leftBump || rightBump)) {
		//AVOID BUMPS - preempt and bumpers
		return AVOID_BUMPS;

	} else if (canPreempt(ESCAPE, prevComplete) && fearSensor == FEAR_SENSOR_LIGHT) {
		//ESCAPE - preempt, fear sensor, and TODO fear state, TODO energy (FULL & HUNGRY)
		return ESCAPE;
		/*
	} else if (canPreempt(APPROACH, prevComplete)) {
		//APPROACH - preempt, TODO sonar distance
		return APPROACH;

	} else if (canPreempt(FEED, prevComplete)) {
		//FEED -- preempt, TODO on a feeding patch, TODO energy state (HUNGRY, DANGER)
		return FEED;

	} else if (canPreempt(GRADIENT_FOLLOW, prevComplete)) {
		//GRADIENT FOLLOW - preempt, TODO energy state (HUNGRY, DANGER)
		return GRADIENT_FOLLOW;
		*/
	} else {
		//WANDER if everything else fails
		return WANDER;
	}
}

void setComplete(bool val) {
		semaphoreLock(taskCompleteLock);
		taskComplete = val;
		semaphoreUnlock(taskCompleteLock);
}

bool getComplete() {
		semaphoreLock(taskCompleteLock);
		bool val = taskComplete;
		semaphoreUnlock(taskCompleteLock);
		return val;
}

/////////////////////////////////////////////////////////////////
//                                                             //
//                  DRIVING & SENSING TASKS                    //
//                                                             //
/////////////////////////////////////////////////////////////////


task AvoidBumps() {
	setComplete(false);
	if(leftBump && rightBump)
	{
		moveBackwards(50, 500);
		playSound(soundBeepBeep);

		if (random(1))
		{
			turnLeft(50, 500);
		} else {
			turnRight(50, 500);
		}
	} else if (leftBump)
	{
		moveBackwards(50, 500);
		playSound(soundBeepBeep);
		turnRight(50, 500);
	} else if (rightBump)
	{
		moveBackwards(50, 500);
		playSound(soundBeepBeep);
		turnLeft(50, 500);
	}
	stopMotors();

	//cleanup
	leftBump = false;
	rightBump = false;
	setComplete(true);

}

task Escape(){
	setComplete(false);

	if(fearValue>0){
		motor[motorA]=-DEFAULT_SPEED;
		motor[motorB]=-DEFAULT_SPEED;
		sleep(500);
		motor[motorA]=abs(rand() % 20) + DEFAULT_SPEED;
		motor[motorB]=-DEFAULT_SPEED;
		sleep(1000);
		motor[motorA]=(fearValue*25);
		motor[motorB]=(fearValue*25);
		sleep(fearValue*1000);
	}

	if(fearTimeMinute<1 && fearValue>0){
		fearValue= fearValue-1;
	}

	clearTimer(T1);
	fearTimeMinute =0;

	setComplete(true);
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

	if( motorState != taskID) {
		//Suspend all but the current task
		int i = 0;
		while( i <= WANDER) {
			if( i != taskID) {
				switch(i) {
					case DEATH: break; //DO NOTHING. DEATH CANNOT BE INTERRUPTED
					case AVOID_BUMPS: stopTask(AvoidBumps); break;
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
			case DEATH: break; //handled elsewhere
			case AVOID_BUMPS: if(motorState != AVOID_BUMPS) startTask(AvoidBumps); break;
			case ESCAPE: startTask(Escape); break;
			//case APPROACH: resumeTask(Approach); break;
			//case FEED: resumeTask(Feed); break;
			//case GRADIENT_FOLLOW: resumeTask(Escape); break;
			case WANDER: resumeTask(Wander); break;
		}
	}
	//Set the motor state
	motorState = taskID;
}

/*
This task is responsible for gather the raw data, parsing it
and determing what mode the robot should be in for driving.
*/
task Sensors(){
	semaphoreInitialize(taskCompleteLock);
	int fearTimerThirty = 0;

	//unparsed/raw values
	int energyVal = ENERGY_MAX;
	float sonarVal = 0;

	//init all driving tasks
	startTask(Wander);
	startTask(Escape);

	//continuouly check sensors and control driving task exclusivity
	while(1){
		readBluetoothMessages();
		leftBump = SensorValue(leftBumper) || leftBump;
		rightBump = SensorValue(rightBumper) || rightBump;

		if(time1[T1] >= 30000){
			fearTimerThirty++;
			clearTimer(T1);
			if(fearTimerThirty == 2){
				fearTimeMinute++;
				if(fearValue < 4){
					fearValue++;
				}
				fearTimerThirty=0;
			}
		}

		//determine and set driving mode
		switch(nextState(getComplete())) {
			case DEATH:
				motorState = DEATH;
				displaySensors(energyVal, fearValue, motorState);
				killRobot();
				break;
			case AVOID_BUMPS:
				makeTaskExclusive(AVOID_BUMPS);
				break;
			case ESCAPE:
				makeTaskExclusive(ESCAPE);
				break;
			case APPROACH:
				//blah
				break;
			case FEED:
				//blah
				break;
			case GRADIENT_FOLLOW:
				//blah
				break;
			case WANDER:
				makeTaskExclusive(WANDER);
				break;
		}

		//show the sensors and state values on the display
		displaySensors(energyVal, fearValue, motorState);
	}
}

task main()
{
	startTask(Sensors);

	//stay awake by keep main from exiting
	while(1) {
		sleep(1000);
	}
}
