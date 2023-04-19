#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>  
#include <sys/stat.h>  


#define RESOURCE_CT		6
#define RECIPE_CT		5

#define MIXER_R		0
#define PANTRY_R	1
#define FRIDGE_R	2
#define BOWL_R		3
#define SPOON_R		4
#define OVEN_R		5

sem_t *resSems[RESOURCE_CT]; // one semaphore per resource

typedef struct Resource{
	int count;
	char * name ;
} resource_t;

typedef struct Recipe {
	char * name ;
	char * pantryIngr[5];
	char pIngrCt ;
	char * refrigIngr[3];
	char rIngrCt ;
} recipe_t;

// Parallel array to resSems
resource_t resources[] = {
	{ 2, "Mixer" }, 
	{ 1, "Pantry" },
	{ 2, "Refrigerator" },
	{ 3, "Bowl" },
	{ 5, "Spoon" },
	{ 1, "Oven" } 
};

// All recipes bakers must make
recipe_t recipes[] = {
	{ "Cookies", { "Flour", "Sugar" }, 2, { "Milk", "Butter" }, 2 },
	{ "Pancakes", { "Flour", "Sugar", "Baking soda", "Salt" }, 4, { "Egg", "Milk", "Butter" }, 3 }, 
	{ "Homemade pizza dough", { "Yeast", "sugar", "salt" }, 3, { }, 0 },
	{ "Soft Pretzels", { "Flour", "sugar", "salt", "yeast", "baking soda" }, 5, { "Egg" }, 1 },
	{ "Cinnamon rolls", { "Flour", "sugar", "salt", "cinnamon" }, 4, { "butter", "eggs" }, 2 } 
};

// Gets a resource
void get(int bakerNum, int res){
	printf("Baker %d waiting for %s.\n", bakerNum, resources[res].name);
	
	if (sem_wait(resSems[res]) == -1){ // get resource
		perror("sem_wait");
		exit(1);
	}
	
	printf("Baker %d got %s.\n", bakerNum, resources[res].name);
}

// Releases resource
void release(int bakerNum, int res){
	printf("Baker %d releasing %s.\n", bakerNum, resources[res].name);
	
	if (sem_post(resSems[res]) == -1){ // return resource
		perror("sem_post");
		exit(1);
	}
	
	poll(0, 0, 50);  // 50 ms = edge of visual perception
}

void pantry(int bakerNum, int recipe){
	
	if (!recipes[recipe].pIngrCt){
		return ;
	}
	
	get(bakerNum, PANTRY_R);
	
	for (int i = 0 ; i < recipes[recipe].pIngrCt ; i++){
		printf("Baker %d gets %s.\n", bakerNum, recipes[recipe].pantryIngr[i]);
	}
	
	sleep(1);
	
	release(bakerNum, PANTRY_R);
}

void refrigerator(int bakerNum, int recipe){

	if (!recipes[recipe].rIngrCt){
		return ;
	}
	
	get(bakerNum, FRIDGE_R);
	
	for (int i = 0 ; i < recipes[recipe].rIngrCt ; i++){
		printf("Baker %d gets %s.\n", bakerNum, recipes[recipe].refrigIngr[i]);
	}
	
	sleep(1);
	
	release(bakerNum, FRIDGE_R);
}    
	
void bake(int bakerNum, int recipe){
	printf("Baker %d Begins baking %s.\n", bakerNum, recipes[recipe].name); 

	// Gather resources
	get(bakerNum, BOWL_R);
	get(bakerNum, SPOON_R);
	
	// Gather ingredients from pantry and fridge
	pantry(bakerNum, recipe);
	refrigerator(bakerNum, recipe);
	
	// Mix
	get(bakerNum, MIXER_R);
	sleep(5);
	
	// Release unneeded resources
	release(bakerNum, MIXER_R);
	release(bakerNum, BOWL_R);
	release(bakerNum, SPOON_R);

	// Ov in the cold food
	get(bakerNum, OVEN_R);
	sleep (10);

	// Ov out the hot food
	release(bakerNum, OVEN_R);
	printf("Baker %d finished baking %s.\n\n", bakerNum, recipes[recipe].name);
}


void * baker(void * a){
	long bakerNum = (long) a;

	printf("Baker %ld starting.\n", bakerNum);

	for (int i = 0 ; i < RECIPE_CT; i++){
		bake(bakerNum, (i + bakerNum) % RECIPE_CT);
	}
	
	printf("Baker %ld done.\n", bakerNum);
	//return NULL;
}

int main(){
	//int j;
	long bakersCt = -1L, bkr;
	pthread_t * threadPtr;
	
	for (int i = 0; i < RESOURCE_CT; i++) {
		sem_unlink(resources[i].name);
		resSems[i] = sem_open(resources[i].name, O_CREAT | O_EXCL, 0700, resources[i].count);

		if (resSems[i] == SEM_FAILED) {
				perror("sem_open");
				return 1;
		}
	}
	
	printf("How many bakers should I create? ");
	scanf("%ld", &bakersCt);
	
	if (bakersCt < 1){
		printf("Invalid count %ld.\n", bakersCt);
		return 1;
	}
	
	if (!(threadPtr = calloc(bakersCt, sizeof(pthread_t)))){
		perror("calloc bakersCt");
		return 1;
	}
	
	bkr = bakersCt; // make a copy for the join loop
	
	while (bakersCt){ // spin off all bakers
		int e;
		
		bakersCt--;
		
		if (0 > (e = pthread_create(threadPtr + bakersCt, NULL, baker, (void *)bakersCt))){
			printf("pthread_create returns %d\n", e);
			return 1;
		}
	}
	
	while (bkr){ // wait for all bakers
		pthread_join(threadPtr[--bkr], NULL);
	}
	
	free(threadPtr); // unnecessary but nice
	threadPtr = NULL ; // unnecessary but nice

	// Close and unlink the named semaphores when they're no longer needed
	for (int i = 0; i < RESOURCE_CT; i++) {
		sem_close(resSems[i]);
		sem_unlink(resources[i].name);
	}
	return 0;
}
