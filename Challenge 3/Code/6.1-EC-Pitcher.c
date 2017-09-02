#pragma config(Sensor, S1,     lightSensor,    sensorLightInactive)
#pragma config(Sensor, S4,     sonarSensor,    sensorSONAR)

task main()
{

	//stay awake by keep main from exiting
	while(1) {

		if(messageParm[0] != 0)
		{
			motor[motorB] = messageParm[1];
			//Clear the messages, set them to 0
			ClearMessage();
		}
	}
}
