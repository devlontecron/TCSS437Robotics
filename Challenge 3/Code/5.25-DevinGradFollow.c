#pragma config(Sensor, S4,     left,           sensorLightActive)
#pragma config(Sensor, S1,     right,          sensorLightActive)

//Motor a = right


#define TURN_AMOUNT 5
#define DEFAULT_SPEED 25

int leftColorSensor = 0;
int rightColorSensor = 0;

task GradFollow(){
//color sensors need to equal 1 when they see white. 0 otherwise
	int isNotFeeding = 1;
	int wrongWayCounter = 0;

	while(isNotFeeding){
		motor[motorB] = DEFAULT_SPEED;
		motor[motorA] = DEFAULT_SPEED;
		if(leftColorSensor && rightColorSensor){
			clearTimer(T2);
			while(leftColorSensor && rightColorSensor && time1[T2]<5000){
				//waiting for timer to increment to ensure correct direction
			}
			if(time1[T2]>5000){
				isNotFeeding = 0;
			}else if(time1[T2]<1000 && wrongWayCounter<=3){
				wrongWayCounter++;
			}else if(time1[T2]<1000 && wrongWayCounter>=3){
			//probably going in the wrong way,
			//turn around
				motor[motorB] = DEFAULT_SPEED;
				motor[motorA] = -DEFAULT_SPEED;
				wrongWayCounter = 0;
				sleep(2000);
			}
		}else if(leftColorSensor && !rightColorSensor){
			motor[motorA] = TURN_AMOUNT + DEFAULT_SPEED;
			motor[motorB] =  0;
		}else if(!leftColorSensor && rightColorSensor){
			motor[motorB] = TURN_AMOUNT + DEFAULT_SPEED;
			motor[motorA] =  0;
		}else if(!leftColorSensor && !rightColorSensor){
					motor[motorB] = -TURN_AMOUNT + DEFAULT_SPEED;
			motor[motorA] =  -DEFAULT_SPEED;
		}
	}
	//call feedding task here
	//set gloable var "IsFeedable" to switch tasks
}


task ColorParse() {

	while(1){
	displaySensorValues(line1,1);
	displaySensorValues(line4,4);

	if(SensorValue[right] > 35) {
		rightColorSensor = 1;
		startTask(GradFollow);
		} else{
		rightColorSensor = 0;
	}
	if(SensorValue[left] > 35) {
		leftColorSensor = 1;

	}else
		rightColorSensor = 0;
	}
}

task main()
{

	startTask(ColorParse);


	while(1) {
		sleep(1000);
	}
}
