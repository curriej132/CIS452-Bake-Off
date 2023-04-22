#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Total amount of resources and recipes*/
#define RESOURCE_CT	6
#define RECIPE_CT	5

/* Constant ints to represent the different resources */
#define MIXER_R		0
#define PANTRY_R	1
#define FRIDGE_R	2
#define BOWL_R		3
#define SPOON_R		4
#define OVEN_R		5

sem_t *resSems[RESOURCE_CT]; // Pointers to each resource semaphore

typedef struct Resource
{
	int count;
	char *name;
} resource_t;

typedef struct Recipe
{
	char *name;
	char *pantryIngr[5]; // Ingrediants from pantry
	char pantryCt;		 // Number of pantry ingrediants
	char *r_ingr[3];	 // Ingrediants from refridgerator
	char r_ct;			 // Number of refridgerator ingrediants
} recipe_t;

/* Resources structs with available amount */
resource_t resources[] = {
	{2, "Mixer"},
	{1, "Pantry"},
	{2, "Refrigerator"},
	{3, "Bowl"},
	{5, "Spoon"},
	{1, "Oven"}};

/* All recipes bakers must make */
recipe_t recipes[] = {
	{"Cookies", {"Flour", "Sugar"}, 2, {"Milk", "Butter"}, 2},
	{"Pancakes", {"Flour", "Sugar", "Baking soda", "Salt"}, 4, {"Egg", "Milk", "Butter"}, 3},
	{"Homemade pizza dough", {"Yeast", "sugar", "salt"}, 3, {}, 0},
	{"Soft Pretzels", {"Flour", "sugar", "salt", "yeast", "baking soda"}, 5, {"Egg"}, 1},
	{"Cinnamon rolls", {"Flour", "sugar", "salt", "cinnamon"}, 4, {"butter", "eggs"}, 2}};

void getRes(int bakerNum, int res)
{
	printf("Baker %d waiting for %s.\n", bakerNum, resources[res].name);

	if (sem_wait(resSems[res]) == -1)
	{ 	// getting resource
		perror("sem_wait");
		exit(1);
	}

	printf("Baker %d got %s.\n", bakerNum, resources[res].name);
}

void release(int bakerNum, int res)
{
	printf("Baker %d releasing %s.\n", bakerNum, resources[res].name);

	if (sem_post(resSems[res]) == -1)
	{ 	// release resource
		perror("sem_post");
		exit(1);
	}

	poll(0, 0, 50); // 50 ms = edge of visual perception
}

void pantry(int bakerNum, int rec)
{
	if (!recipes[rec].pantryCt)
	{ 	// checks if the specified recipe has no pantry ingrediants
		return;
	}

	getRes(bakerNum, PANTRY_R);

	for (int i = 0; i < recipes[rec].pantryCt; i++)
	{
		printf("Baker %d gets %s.\n", bakerNum, recipes[rec].pantryIngr[i]);
	}

	sleep(1);

	release(bakerNum, PANTRY_R);
}

void refer(int bakerNum, int rec)
{ 	// checks if the specified recipe has no refridgerator ingrediants

	if (!recipes[rec].r_ct)
	{
		return;
	}

	getRes(bakerNum, FRIDGE_R); //  to acquire the FRIDGE_R semaphore,

	for (int i = 0; i < recipes[rec].r_ct; i++)
	{
		printf("Baker %d gets %s.\n", bakerNum, recipes[rec].r_ingr[i]);
	}

	sleep(1);

	release(bakerNum, FRIDGE_R);
}

void bake(int bakerNum, int rec)
{
	printf("Baker %d Begins baking %s.\n", bakerNum, recipes[rec].name);
	getRes(bakerNum, BOWL_R);
	getRes(bakerNum, SPOON_R);
	pantry(bakerNum, rec);
	refer(bakerNum, rec);
	getRes(bakerNum, MIXER_R);
	sleep(5);
	release(bakerNum, MIXER_R);
	release(bakerNum, BOWL_R);
	release(bakerNum, SPOON_R);
	getRes(bakerNum, OVEN_R);
	sleep(10);
	release(bakerNum, OVEN_R);
	printf("Baker %d finished baking %s.\n\n", bakerNum, recipes[rec].name);
}

void *baker(void *a)
{
	long bakerNum = (long)a;

	printf("Baker %ld starting.\n", bakerNum);

	for (int i = 0; i < RECIPE_CT; i++)
	{
		bake(bakerNum, (i + bakerNum) % RECIPE_CT);
	}

	printf("Baker %ld done.\n", bakerNum);
	return NULL;
}

int main()
{
	int j;
	long bakers, bkr;
	pthread_t *threadPtr;

	for (int i = 0; i < RESOURCE_CT; i++)
	{
		sem_unlink(resources[i].name);
		resSems[i] = sem_open(resources[i].name, O_CREAT | O_EXCL, 0700, resources[i].count);

		if (resSems[i] == SEM_FAILED)
		{
			perror("sem_open");
			return 1;
		}
	}

	printf("How many bakers should I create? ");
	scanf("%ld", &bakers);

	// Validate user input
	if (bakers < 1)
	{
		printf("Invalid count %ld.\n", bakers);
		return 1;
	}

	// Memory allocation for threads
	if (!(threadPtr = calloc(bakers, sizeof(pthread_t))))
	{
		perror("calloc bakers");
		return 1;
	}

	bkr = bakers;	// make a copy for the join loop

	while (bakers)
	{ 	// spin off all bakers
		int e;

		bakers--;

		if (0 > (e = pthread_create(threadPtr + bakers, NULL, baker, (void *)bakers)))
		{
			printf("pthread_create returns %d\n", e);
			return 1;
		}
	}

	while (bkr)
	{	// wait for all bakers
		pthread_join(threadPtr[--bkr], NULL);
	}

	free(threadPtr);
	threadPtr = NULL;

	// Close and unlink the named semaphores when they're no longer needed
	for (int i = 0; i < RESOURCE_CT; i++)
	{
		sem_close(resSems[i]);
		sem_unlink(resources[i].name);
	}
	return 0;
}