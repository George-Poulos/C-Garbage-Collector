hw4: hw4.c main.c 
	gcc -O0 -g hw4.c main.c -o hw4 --std=gnu99

debug: hw4.c debug_main.c
	gcc -O0 -g hw4.c debug_main.c -o hw4 --std=gnu99
