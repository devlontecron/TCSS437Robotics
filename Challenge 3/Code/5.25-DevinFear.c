#pragma config(Sensor, S1,     right,          sensorLightActive)

task Escape(){
	//i need a timer that is always keeps tracks of minutes
  //every myTimeMinute+1 increment, we needs to incrmenet fearValue+1

/*
	if(timer1[T1]==30000){
	myTimerThirty++;
	clearTimer(T1)
	if(myTimerThirty==2){
		myTimeMinute++;
		if(fearValue<4){
			fearValue++;
			}
		myTimerThirty=0;
	}
}

	*/
	//please put this^ in sensorsTask





	if(fearValue>0){
		motor[motorA]=-DEFAULT_SPEED;;
		motor[motorB]=-DEFAULT_SPEED;;
		sleep(500);
		motor[motorA]=abs(rand() % 20) + DEFAULT_SPEED;
		motor[motorB]=-DEFAULT_SPEED;
		sleep(1000);
		motor[motorA]=(fearValue*25);
		motor[motorB]=(fearValue*25);
		sleep(fearValue*1000);
	}

	if(myTimerMinute<1 && fearValue>0){
		fearValue= fearValue-1;
	}

	clearTimer(T1);
	myTimeMinute =0;

	taskComplete = true;
}
