OBTAIN:

	#gcc harvest.c -o get
	#gcc harvest.c -lmpg123 -lao -lvlc -lpthread -o get
	gcc -Wall harvest.c player1053.c vs1053-utils.c -lwiringPi -lvlc -lpthread -o radio

