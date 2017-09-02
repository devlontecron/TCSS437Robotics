//we are going to need a energyTimerThirty
/*
if(time1[T2] >= 30000){
energyTimerThirty++;
clearTimer(T2);
if(energyTimerThirty == 2 && energyState < ENERGY_FULL){
energyState--;
energyTimerThirty=0;
}else if(energyTimerThirty == 4 && energyState == ENERGY_FULL){
energyState--;
energyTimerThirty=0;
}
}
motorA = right
motorB=Left
*/

task feed(){
	playSound(soundFastUpwardTones);


	while(energyState != ENERGY_FULL){
		//Stays in patch
		if(leftColorSensor ==BLACK && rightColorSensor ==WHITE){
			motor[motorA] = 0;
			motor[motorB] = DEFAULT_SPEED;
		}else if(leftColorSensor ==WHITE && rightColorSensor ==BLACK){
			motor[motorB] = 0;
			motor[motorA] = DEFAULT_SPEED;
		}else{
			//wanders in patch
			int speed = abs(rand() % 20) + DEFAULT_SPEED;
			motor[motorA] = speed;
			motor[motorB] = ((2 * DEFAULT_SPEED + 20) - speed);

			//moreEnergy
			if(energyState<ENERGY_HUNGRY && energyTimerThirty >= 1){
				energyState++;
				clearTimer(T2);
				energyTimerThirty =0;
				}else if(energyState==ENERGY_HUNGRY && energyTimerThirty>=2){
				energyState++;
				clearTimer(T2);
				energyTimerThirty =0;
			}
		}
	}
	playSound(soundLowBuzz);
}
