#pragma config(Sensor, S1,     touchSensor,         sensorEV3_Touch)
#pragma config(Sensor, S2,     touchSensor1,         sensorEV3_Touch)

bool canDrive;
int leftBumper = 0; // this will be the value from S1
int rightBumper = 0; // this will be the value from S2
int flag = 0;
int initialMotorSpeed = 40;
int randomSpeed = 30;
int randomTurnSpeed = 250;

task bumper()
{
	while(1){
		leftBumper = getBumpedValue(S1); // poll for bumper activity
		rightBumper = getBumpedValue(S2);
		while(getBumpedValue(S1) == 0 && getBumpedValue(S2) == 0)
		{
		}
		canDrive = false;
		if(getBumpedValue(S1) == 1)
		{
			randomTurnSpeed = random(749)+250;
			clearTimer(T1);
			while(time1[T1] < 50)
			{
				rightBumper = getBumpedValue(S2);
			}//this is just a little polling to check if both bumpers are hit
			if (rightBumper >= 1)
			{
				flag = 1;
				setLEDColor(ledRedFlash);
				playImmediateTone(20, 20);//make a sound
				motor[motorB] = -30;
				motor[motorC] = -30;
				wait1Msec(1000);
				motor[motorB] = 0;
				motor[motorC] = 0;
				wait1Msec(2000);
				int newDirection = random(2); //changed from rand()%3
				displayCenteredBigTextLine(6, "Direction %d", newDirection);
				if(newDirection == 0){
					motor[motorB] = -30;
					motor[motorC] = 30;
					delay(500);
					}else if(newDirection == 1){
					motor[motorB] = 30;
					motor[motorC] = -30;
					delay(500);
					}else if(newDirection == 2){
					motor[motorB] = -30;
					motor[motorC] = 30;
					delay(1000);
				}

			} else flag =0;

			if(flag ==0)
			{
				motor[motorB] = -30;
				motor[motorC] = -30;
				wait1Msec(1000);
				motor[motorB] = randomSpeed;
				motor[motorC] = -randomSpeed;
				wait1Msec(randomTurnSpeed);
			}
			setLEDColor(ledGreen);
			rightBumper = 0;
			leftBumper = 0;
			resetBumpedValue(S1);
			resetBumpedValue(S2);

		}
		if(getBumpedValue(S2) == 1)
		{
			randomTurnSpeed = random(749)+250;
			clearTimer(T1);
			while(time1[T1] < 50)
			{
				leftBumper = getBumpedValue(S1);
			}//this is just a little polling to check if both bumpers are hit
			if (leftBumper >= 1)
			{
				flag = 1;
				setLEDColor(ledRedFlash);
				playImmediateTone(20, 20);//make a sound
				motor[motorB] = -30;
				motor[motorC] = -30;
				wait1Msec(750);
				motor[motorB] = 0;
				motor[motorC] = 0;
				wait1Msec(2000);

				int newDirection = random(2); //changed from rand()%3
				displayCenteredBigTextLine(6, "Direction %d", newDirection);
				if(newDirection == 0){
					motor[motorB] = -30;
					motor[motorC] = 30;
					delay(500);
					}else if(newDirection == 1){
					motor[motorB] = 30;
					motor[motorC] = -30;
					delay(500);
					}else if(newDirection == 2){
					motor[motorB] = -30;
					motor[motorC] = 30;
					delay(1000);
				}
			}else flag =0;


			if(flag ==0)
			{
				motor[motorB] = -30;
				motor[motorC] = -30;
				wait1Msec(1000);
				motor[motorB] = -randomSpeed;
				motor[motorC] = randomSpeed;
				wait1Msec(randomTurnSpeed);
			}//if flag =0
			setLEDColor(ledGreen);
			rightBumper = 0;
			leftBumper = 0;
			resetBumpedValue(S1);
			resetBumpedValue(S2);


		}
		motor[motorB] = initialMotorSpeed;
		motor[motorC] = initialMotorSpeed;
		randomSpeed = random(15) + 20;
		canDrive = true;
	}
}//end bumper

void drive(){
	int LorR = random(1);
	int timeInterval = 1000;
	int randoTime = 0;
	int maxIncrease = 20;
	int delta = 0;
	int bias = 0;

	while(canDrive){
		clearTimer(T3);
		while(time1[T3] < timeInterval){
			displayCenteredBigTextLine(2, "Left %d", motor[motorB]);
			displayCenteredBigTextLine(4, "Right %d", motor[motorC]);
			wait1Msec(200);
			if (LorR == 0){
				bias--;
				if(delta <= maxIncrease)
				{
					motor[motorC] = motor[motorC]+1;
					delta++;
				}
				if(motor[motorB] >= initialMotorSpeed){
					motor[motorB]  = motor[motorB]-1;
				}

			} else if (LorR ==1){
				bias++;
				if(delta <= maxIncrease)
				{
					motor[motorB] = motor[motorB]+1;
					delta++;
				}
				if(motor[motorC] >= initialMotorSpeed){
					motor[motorC]  = motor[motorC]-1;
				}
			}

		}

		delta = 0;
		randoTime = random(3);
		timeInterval = randoTime * 1000;
			LorR = random(1);
		if(bias>1){
			LorR = 0;
			bias--;
		}else if(bias<-1){
		LorR = 1;
		bias++;
	}

		while(motor[motorB] >= initialMotorSpeed || motor[motorC] >= initialMotorSpeed){
			wait1Msec(200);
			if(motor[motorB] >= initialMotorSpeed){
				motor[motorB]  = motor[motorB]-2;
			}
			if(motor[motorC] >= initialMotorSpeed){
				motor[motorC]  = motor[motorC]-2;
			}
		}
	}

}




task main()
{
	resetBumpedValue(S1);
	resetBumpedValue(S2);
	motor[motorB] = initialMotorSpeed;
	motor[motorC] = initialMotorSpeed;

	canDrive = true;
	startTask(bumper);
	while(1)
	{
		displayCenteredBigTextLine(2, "Left %d", motor[motorB]);
		displayCenteredBigTextLine(4, "Right %d", motor[motorC]);
		drive();
	}


}
