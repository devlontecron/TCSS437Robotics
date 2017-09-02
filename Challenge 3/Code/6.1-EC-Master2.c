#pragma config(Sensor, S2,     rightBumper,    		sensorTouch)
#pragma config(Sensor, S3,     leftBumper,     		sensorTouch)
#pragma config(Sensor, S4,     leftColorSensor,   sensorLightActive)
#pragma config(Sensor, S1,     rightColorSensor,  sensorLightActive)

//Sensor defaults and thresholds
#define BLACK 0
#define WHITE 1
#define COLOR_THRESH 42
#define SONAR_THRESH 100
#define SONAR_ADJUST 19
#define FEAR_MAX 4
#define FEAR_SENSOR_LIGHT 1
#define FEAR_SENSOR_DARK 0

//Driving/Turning configuration
#define TURN_AMOUNT 5
#define DEFAULT_SPEED 25
#define MAX_SPEED 100
#define SEND_MOTOR 1
#define ARBITRARY_NUMBER_BECAUSE_I_DONT_KNOW_WHAT_ELSE_TO_SEND 4693234

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
int leftColor = BLACK;
int rightColor = BLACK;
int sonarDistance = SONAR_THRESH;
int fearSensor = FEAR_SENSOR_DARK;
bool leftBump = false;
bool rightBump = false;

int motorState = -1;
int lastBTMotorSpeed = 0;
int energyState = ENERGY_HUNGRY;
int fearValue = FEAR_MAX;
bool canFeed = false;

TSemaphore taskCompleteLock;
bool taskComplete = true;

int fearTimerThirty = 0;
int energyTimerThirty = 0;

/////////////////////////////////////////////////////////////////
//                                                             //
//                      HELPER FUNCTIONS                       //
//                                                             //
/////////////////////////////////////////////////////////////////

/**
Display the values of the sensors on the display
*/
void displaySensors(int fearValue, int drivingState) {

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
			case -1:
				state = "ERRR";
				break;
	}

	nxtDisplayBigTextLine(0, "E: %s", energy);
	//nxtDisplayBigTextLine(2, "   %d", energyValue);
	nxtDisplayBigTextLine(4, "F: %d", fearValue);
	nxtDisplayBigTextLine(6, "S: %s", state);
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
Check, read and assign the values sent from the slave brick
*/
void readBluetoothMessages() {

		//Something got through if the signals are not all zero
		if(messageParm[0] != 0 || messageParm[1] != 0 || messageParm[2] != 0)
		{
			sonarDistance = messageParm[0];
			fearSensor = messageParm[1] || fearSensor;

			//Clear the messages, set them to 0
			ClearMessage();
		}
}

/**
Send the other motor speed to other brick
*/
void sendMotor(int speed) {
	if(speed != lastBTMotorSpeed){
		sendMessageWithParm(SEND_MOTOR, speed, ARBITRARY_NUMBER_BECAUSE_I_DONT_KNOW_WHAT_ELSE_TO_SEND );
		lastBTMotorSpeed = speed;
	}
}


/**
Bring both motors to a complete stopAllTasks
*/
void stopMotors() {
	sendMotor(0);
	motor[motorB] = 0;
}

// moveBackwards moves the robot backwards for a given amount of time.
// int power the power of the motors
// int time  the amount of time the motors will run.
void moveBackwards(int power, int time) {
	sendMotor(-1 * power);
	motor[motorB] = -1 * power;
	sleep(time);
}

// turnLeft turns the robot left for a given amount of time.
void turnLeft(int power, int time) {
	sendMotor(-1 * power);
	motor[motorB] = 1 * power;
	sleep(time);
}

// turnRight turns the robot right for a given amount of time.
void turnRight(int power, int time) {
	sendMotor(1 * power);
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
	if(motorState == ESCAPE && prevComplete) {
		fearSensor = false;
	}

	if( energyState == ENERGY_DEAD) {
		//DEATH - can't avoid it bro
		return DEATH;

	} else if (canPreempt(AVOID_BUMPS, prevComplete)
		&& (leftBump || rightBump)) {
		//AVOID BUMPS - preempt and bumpers
		return AVOID_BUMPS;

	} else if (canPreempt(ESCAPE, prevComplete)
		&& fearSensor == FEAR_SENSOR_LIGHT
		&& (energyState == ENERGY_FULL || energyState == ENERGY_HUNGRY)
		&& fearValue > 0) {
		//ESCAPE - preempt, fear sensor, and TODO fear state, TODO energy (FULL & HUNGRY)
		return ESCAPE;

	} else if (canPreempt(APPROACH, prevComplete)
		&& sonarDistance < SONAR_THRESH
		&& (energyState == ENERGY_FULL || energyState == ENERGY_HUNGRY)) {
		return APPROACH;

	} else if (canPreempt(FEED, prevComplete)
		&& canFeed
		&& (energyState == ENERGY_HUNGRY || energyState == ENERGY_DANGER)) {
		//FEED -- preempt, TODO on a feeding patch, TODO energy state (HUNGRY, DANGER)
		return FEED;

	} else if (canPreempt(GRADIENT_FOLLOW, prevComplete)
		&& (leftColor || rightColor)
		&& (energyState == ENERGY_HUNGRY || energyState == ENERGY_DANGER)) {
		//GRADIENT FOLLOW - preempt, TODO energy state (HUNGRY, DANGER)
		return GRADIENT_FOLLOW;

	} else {
		//WANDER if everything else fails
		return WANDER;
	}
}

/**
Set whether a task is complete with its action*/
void setComplete(bool val) {
		semaphoreLock(taskCompleteLock);
		taskComplete = val;
		semaphoreUnlock(taskCompleteLock);
}

/*
Check whether the running task is complete
*/
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

/**
Avoid bumps as defined in challenge 1
*/
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

/**
Approach an object as defined in challenge 2
*/
task Approach()
{
	setComplete(false);

	while(sonarDistance >= SONAR_ADJUST && sonarDistance < SONAR_THRESH) {
			sendMotor(1.0 * sonarDistance);
			motor[motorB] = 1.0 * sonarDistance;
	}

	if (sonarDistance < SONAR_ADJUST) {
		stopMotors();
		sleep(2000);
		moveBackwards(DEFAULT_SPEED, 1000);
		if(random(1)) {
			turnLeft(DEFAULT_SPEED, 400);
		} else {
			turnRight(DEFAULT_SPEED, 400);
		}
	}

	setComplete(true);
}

/**
Follow the gradients to feed */
task GradFollow(){
	//color sensors need to equal 1 when they see white. 0 otherwise
	int wrongWayCounter = 0;
	clearTimer(T2);

	while(!canFeed){

		if(leftColor && rightColor){
			motor[motorB] = DEFAULT_SPEED;
			sendMotor(DEFAULT_SPEED);
			clearTimer(T2);
			while(leftColor && rightColor && time1[T2]<1200){
				//waiting for timer to increment to ensure correct direction
			}

			if(time1[T2]>=1200){
				canFeed = true;
			}else if(time1[T2]<=1200 && wrongWayCounter<=3){
				wrongWayCounter++;
			}else if(time1[T2]<=1200 && wrongWayCounter>=3){
				//probably going in the wrong way,
				//turn around
				motor[motorB] = DEFAULT_SPEED;
				sendMotor(-DEFAULT_SPEED);
				wrongWayCounter = 0;
				sleep(2000);
			}
		}else if(leftColor && !rightColor){
			sendMotor(TURN_AMOUNT + DEFAULT_SPEED);
			motor[motorB] =  0;
		}else if(!leftColor && rightColor){
			motor[motorB] = TURN_AMOUNT + DEFAULT_SPEED;
			sendMotor(0);
		}
	}
	//call feedding task here
	//set gloable var "IsFeedable" to switch tasks
}

/**
Run away from the light, if there fear value is in the correct range
*/
task Escape(){
	setComplete(false);

	if(fearValue>0){
		sendMotor(-DEFAULT_SPEED);
		motor[motorB]=-DEFAULT_SPEED;
		sleep(500);
		sendMotor(abs(rand() % 20) + DEFAULT_SPEED);
		motor[motorB]=-DEFAULT_SPEED;
		sleep(1000);
		sendMotor(fearValue*25);
		motor[motorB]=(fearValue*25);
		sleep(fearValue*1000);
	}

	if((fearTimerThirty/2)<1 && fearValue>0){
		fearValue= fearValue-1;
	}

	clearTimer(T1);
	fearTimerThirty = 0;

	fearSensor = false;
	setComplete(true);
}

/*
Wander as defined in challenge 1
*/
task Wander(){
	while(1){
		int speed = abs(rand() % 20) + DEFAULT_SPEED;
		//setMotor(motorA, speed);
		sendMotor(speed);
		motor[motorB] = ((2 * DEFAULT_SPEED + 20) - speed);
		//setMotor(motorB, );
		sleep(500);
	}
}

/**
Wander on the patch and increase the energy level
*/
task Feed(){
	setComplete(false);
	playSound(soundFastUpwardTones);
	clearTimer(T3);
	energyTimerThirty = 0;
	int wrongWayCounter = 0;
	clearTimer(T2);

	int speed = abs(rand() % 20) + DEFAULT_SPEED;

	while(energyState != ENERGY_FULL && (leftColor || rightColor) ){
		//Stays in patch

		if(wrongWayCounter >= 5 && time1[T2] <1500) {
			wrongWayCounter = 0;
			motor[motorB] = DEFAULT_SPEED;
			sendMotor(-DEFAULT_SPEED);
			sleep(750);
			clearTimer(T2);
		} else if ( time1[T2] >= 1500) {
			wrongWayCounter = 0;
			clearTimer(T2);
		}

		if(leftColor == BLACK && rightColor == WHITE){
			wrongWayCounter++;
			sendMotor(0);
			motor[motorB] = DEFAULT_SPEED;
			speed = abs(rand() % 20) + DEFAULT_SPEED;
		} else if (leftColor == WHITE && rightColor == BLACK){
			wrongWayCounter++;
			sendMotor(DEFAULT_SPEED);
			motor[motorB] = 0;
			speed = abs(rand() % 20) + DEFAULT_SPEED;
		} else {
			//wanders in patch
			sendMotor(speed);
			motor[motorB] = ((2 * DEFAULT_SPEED + 20) - speed);
		}

		//moreEnergy
		if(energyState<ENERGY_HUNGRY && energyTimerThirty >= 1){
			energyState++;
			clearTimer(T3);
			energyTimerThirty =0;
		}else if(energyState == ENERGY_HUNGRY && energyTimerThirty>=2){
			energyState++;
			clearTimer(T3);
			energyTimerThirty =0;
		}
	}

	playSound(soundLowBuzz);
	sendMotor(DEFAULT_SPEED);
	motor[motorB] = DEFAULT_SPEED;
	canFeed = false;
	setComplete(true);
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
					case APPROACH: stopTask(Approach); break;
					case FEED: stopTask(Feed); break;
					case GRADIENT_FOLLOW: stopTask(GradFollow); break;
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
			case APPROACH: startTask(Approach); break;
			case FEED: startTask(Feed); break;
			case GRADIENT_FOLLOW: startTask(GradFollow); break;
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
	startTask(GradFollow);

	//continuouly check sensors and control driving task exclusivity
	while(1){
		readBluetoothMessages();
		leftBump = SensorValue(leftBumper) || leftBump;
		rightBump = SensorValue(rightBumper) || rightBump;
		leftColor = parseRawColor(SensorValue[leftColorSensor]);
		rightColor = parseRawColor(SensorValue[rightColorSensor]);

		//fear timer
		if(time1[T1] >= 30000){
			fearTimerThirty++;
			clearTimer(T1);
			if(fearTimerThirty == 2){
				if(fearValue < 4){
					fearValue++;
				}
				fearTimerThirty=0;
			}
		}

		//energy timer -- decrement when not feeding
		if(time1[T3] >= 30000){
			energyTimerThirty++;
			clearTimer(T3);
			if(!canFeed) {
				if(energyTimerThirty == 4 && energyState == ENERGY_FULL){
					energyState--;
					energyTimerThirty=0;
				}else if(energyTimerThirty == 2 && energyState < ENERGY_FULL){
					energyState--;
					energyTimerThirty=0;
				}
			}
		}

		//determine and set driving mode
		switch(nextState(getComplete())) {
			case DEATH:
				motorState = DEATH;
				displaySensors(fearValue, motorState);
				killRobot();
				break;
			case AVOID_BUMPS:
				makeTaskExclusive(AVOID_BUMPS);
				break;
			case ESCAPE:
				makeTaskExclusive(ESCAPE);
				break;
			case APPROACH:
				makeTaskExclusive(APPROACH);
				break;
			case FEED:
				makeTaskExclusive(FEED);
				break;
			case GRADIENT_FOLLOW:
				makeTaskExclusive(GRADIENT_FOLLOW);
				break;
			case WANDER:
				makeTaskExclusive(WANDER);
				break;
		}

		//show the sensors and state values on the display
		displaySensors(fearValue, motorState);
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
