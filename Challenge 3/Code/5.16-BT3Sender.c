//Sender.c
//from http://robotsquare.com/2012/03/21/tutorial-robotc-bluetooth/
task main()
{
	wait1Msec(500);

	int number_1 = 1;
	int number_2 = 50;
	int number_3 = 500;

	//This sends through three values: 5, -1 and 700
	sendMessageWithParm(number_1,number_2,number_3);

	//Let’s continually send data, wait a bit and send something else
	while(true) {
		wait1Msec(450);
		number_1++;
		number_2 = random(99);
		number_3 = random(900) + 100;
		sendMessageWithParm(number_1,number_2,number_3);
	}
}
