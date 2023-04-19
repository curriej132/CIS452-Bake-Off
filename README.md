## Overview 
This is a program that simulates multiple bakers working together to bake different recipes. The program uses POSIX threads and semaphores to synchronize access to shared resources (e.g., mixer, pantry, oven, etc.) among the bakers.

The main function initializes the semaphores and prompts the user to enter the number of bakers to create. Then, it creates the specified number of threads, each representing a baker. The thread function, baker, executes a loop that bakes all the recipes in a predetermined order. Within the loop, the baker calls the bake function to prepare a specific recipe.

The bake function performs the following steps:
	1.	Acquires the resources needed for baking: a bowl, a spoon, and the 	ingredients from the pantry & the refrigerator.
	2.	Uses the mixer to mix the ingredients.
	3.	Puts the mixture in the bowl and uses the spoon to form the dough.
	4.	Places the dough in the oven and waits for it to bake.
	5.	Releases the resources used in the baking process.

	The get and release functions are used to acquire and release resources. The pantry and refer functions are used to acquire the ingredients from the pantry and refrigerator, respectively.

Overall, the program provides an example of how semaphores can be used to synchronize access to shared resources among multiple threads.


## How to compile for bakeOff.c and bakeOffTwo.c
- gcc -g -pthread -o bake bakeOff.c

- gcc -g -pthread -o bake2 bakeOffTwo.c

## How to run for bakeOff.c and bakeOffTwo.c
./bake

./bake2
