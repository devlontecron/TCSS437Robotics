//from http://robotsquare.com/2012/03/21/tutorial-robotc-bluetooth/
task main()
{
	wait1Msec(500);

	int my_number = 23;

	//This sends through one value: 23
	sendMessage(my_number);

	//Let’s wait a bit and send something else
	wait1Msec(3000);
	my_number = 17;
	sendMessage(my_number);

}
