//from http://robotsquare.com/2012/03/21/tutorial-robotc-bluetooth/
task main()
{
	int my_message;

	while(true)
	{

		//Read the message to variable
		my_message = message;

		//Let’s say something got through if the signal is not zero
		if(my_message != 0)
		{
			//Display the value as stored in the variable
			nxtDisplayBigTextLine(2,"1: %d",my_message);

			//Clear the message, set it to 0
			ClearMessage();
		}

		//Checking for a message every half a second is
		//fine for this simple test

		wait1Msec(500);

	}
}
