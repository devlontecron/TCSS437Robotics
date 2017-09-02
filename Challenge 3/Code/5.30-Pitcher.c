#pragma config(Sensor, S1,     lightSensor,    sensorLightInactive)
#pragma config(Sensor, S4,     sonarSensor,    sensorSONAR)

//Sensor defaults and thresholds
#define LIGHT_THRESH 65
#define SONAR_THRESH 90
#define SONAR_ADJUST 3
#define AVG_NUM 5
#define AVG_WEIGHT 0.5
#define OUTLIER_THRESH 4

#define FEAR_SENSOR_LIGHT 1
#define FEAR_SENSOR_DARK 0

#define ARBITRARY_NUMBER_BECAUSE_I_DONT_KNOW_WHAT_ELSE_TO_SEND 4693234

//FEAR SENSOR
//SONAR

/**
Display the values of the sensors on the display
*/
void displaySensors(int sonarValue, int fearSensorValue) {

	string fearSensor;
	if( fearSensorValue == FEAR_SENSOR_LIGHT ) {
		fearSensor = "LITE";
	} else {
		fearSensor = "DARK";
	}

	nxtDisplayBigTextLine(0, "F: %s", fearSensor);
	nxtDisplayBigTextLine(2, "S: %d", sonarValue);
}

/**
Take the raw sensor input and decide whether that
means the sensor is detecting white or black
*/
int parseRawLight(int raw) {

	//TODO Verify with NXT

	if(raw <= LIGHT_THRESH) {
		return FEAR_SENSOR_DARK;
	} else {
		return FEAR_SENSOR_LIGHT;
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

task main()
{
	int sonarValue = 0;
	int fearSensorValue = FEAR_SENSOR_DARK;

	//Get the moving weighted average for the sonar started
	for(int k = 0; k<10; k++){
		sonarValue = parseRawSonar(SensorValue[sonarSensor], sonarValue);
	}

	//stay awake by keep main from exiting
	while(1) {
		//parse the values from the sensors
		sonarValue = parseRawSonar(SensorValue[sonarSensor], sonarValue);
		fearSensorValue = parseRawLight(SensorValue[lightSensor]);

		//send the values via bluetooth
		sendMessageWithParm(sonarValue, fearSensorValue, ARBITRARY_NUMBER_BECAUSE_I_DONT_KNOW_WHAT_ELSE_TO_SEND);

		//display the values on the screen
		displaySensors(sonarValue, fearSensorValue);
	}
}
