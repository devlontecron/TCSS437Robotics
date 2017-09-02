
#pragma config(Sensor, S2,     rightBumper,    		sensorTouch)
#pragma config(Sensor, S3,     leftBumper,     		sensorTouch)

//Driving/Turning configuration
#define TURN_AMOUNT 5
#define DEFAULT_SPEED 25
#define MAX_SPEED 100
#define ARBITRARY_NUMBER_BECAUSE_I_DONT_KNOW_WHAT_ELSE_TO_SEND 4234
#define SEND_MOTOR 1

//DRIVING STATES
#define AVOID_BUMPS 1
#define WANDER 6

//Global vars for communicating between threads and their initial values
//processed inputs
bool leftBump = false;
bool rightBump = false;
int motorState = -1;

TSemaphore taskCompleteLock;
bool taskComplete = true;

/////////////////////////////////////////////////////////////////
//                                                             //
//                      HELPER FUNCTIONS                       //
//                                                             //
/////////////////////////////////////////////////////////////////

/**
Display the values of the sensors on the display
*/
void displaySensors(int fearValue, int drivingState) {

	string state;
	switch(drivingState){
			case AVOID_BUMPS:
				state = "BUMP";
				break;
			case WANDER:
				state = "WAND";
				break;
			case -1:
				state = "ERRR";
				break;
	}

	nxtDisplayBigTextLine(6, "S: %s", state);
}

void stopMotors() {
	sendMessageWithParm(SEND_MOTOR, 0, ARBITRARY_NUMBER_BECAUSE_I_DONT_KNOW_WHAT_ELSE_TO_SEND );
	motor[motorB] = 0;
}

// moveBackwards moves the robot backwards for a given amount of time.
// int power the power of the motors
// int time  the amount of time the motors will run.
void moveBackwards(int power, int time) {
	sendMessageWithParm(SEND_MOTOR, -1 * power, ARBITRARY_NUMBER_BECAUSE_I_DONT_KNOW_WHAT_ELSE_TO_SEND );
	motor[motorB] = -1 * power;
	sleep(time);
}

// turnLeft turns the robot left for a given amount of time.
void turnLeft(int power, int time) {
	sendMessageWithParm(SEND_MOTOR, -1 * power, ARBITRARY_NUMBER_BECAUSE_I_DONT_KNOW_WHAT_ELSE_TO_SEND );
	motor[motorB] = 1 * power;
	sleep(time);
}

// turnRight turns the robot right for a given amount of time.
void turnRight(int power, int time) {
	sendMessageWithParm(SEND_MOTOR, 1 * power, ARBITRARY_NUMBER_BECAUSE_I_DONT_KNOW_WHAT_ELSE_TO_SEND );
	motor[motorB] = -1 * power;
	sleep(time);
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

	if (canPreempt(AVOID_BUMPS, prevComplete)
		&& (leftBump || rightBump)) {
		//AVOID BUMPS - preempt and bumpers
		return AVOID_BUMPS;
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
	sleep(100);

	if(leftBump && rightBump)
	{
		moveBackwards(50, 500);
		playSound(soundBeepBeep);

		stopMotors();
		sleep(2000);

		if (random(1))
		{
			turnLeft(50, 800);
		} else {
			turnRight(50, 800);
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

task Wander(){
	while(1){
		int speed = abs(rand() % 20) + DEFAULT_SPEED;
		//sendMotorSpeed;
		//send the values via bluetooth
		sendMessageWithParm(SEND_MOTOR, speed, ARBITRARY_NUMBER_BECAUSE_I_DONT_KNOW_WHAT_ELSE_TO_SEND );
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
					case AVOID_BUMPS: stopTask(AvoidBumps); break;
					case WANDER: suspendTask(Wander); break;
				}
			}
			i++;
		}

		//Resume the current task
		switch(taskID) {
			case AVOID_BUMPS: if(motorState != AVOID_BUMPS) startTask(AvoidBumps); break;
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
	clearTimer(T1);
	clearTimer(T2);
	clearTimer(T3);

	//init all driving tasks
	startTask(Wander);

	//continuouly check sensors and control driving task exclusivity
	while(1){
		leftBump = SensorValue(leftBumper) || leftBump;
		rightBump = SensorValue(rightBumper) || rightBump;

		//determine and set driving mode
		switch(nextState(getComplete())) {
			case AVOID_BUMPS:
				makeTaskExclusive(AVOID_BUMPS);
				break;
			case WANDER:
				makeTaskExclusive(WANDER);
				break;
		}

		//show the sensors and state values on the display
		displaySensors(0, motorState);
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
