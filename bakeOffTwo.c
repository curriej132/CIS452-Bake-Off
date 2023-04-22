#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>  
#include <sys/stat.h>  


#define RESOURCE_CT 6
#define RECIPE_CT 5

#define MIXER_R 0
#define PANTRY_R 1
#define FRIDGE_R 2
#define BOWL_R 3
#define SPOON_R 4
#define OVEN_R 5

sem_t *resSems[RESOURCE_CT]; // one semaphore per resource

struct Resource{ // parallel array to resSems
  int count;
  char * name ;
} res_arr[] = { { 2, "Mixer" }, 
                { 1, "Pantry" },
                { 2, "Refrigerator" },
                { 3, "Bowl" },
                { 5, "Spoon" },
                { 1, "Oven" } } ;

struct Recipe {
  char * name ;
  char * pantryIngr[5];
  char pIngrCt ;
  char * refrigIngr[3];
  char rIngrCt ;
} recipes[] = { { "Cookies", { "Flour", "Sugar" }, 2, { "Milk", "Butter" }, 2 },
                 { "Pancakes", { "Flour", "Sugar", "Baking soda", "Salt" }, 4, { "Egg", "Milk", "Butter" }, 3 }, 
                 { "Homemade pizza dough", { "Yeast", "sugar", "salt" }, 3, { }, 0 },
                 { "Soft Pretzels", { "Flour", "sugar", "salt", "yeast", "baking soda" }, 5, { "Egg" }, 1 },
                 { "Cinnamon rolls", { "Flour", "sugar", "salt", "cinnamon" }, 4, { "butter", "eggs" }, 2 } };

void getRes( int n, int res ){
  printf( "Baker %d waiting for %s.\n", n, res_arr[res].name );
  
  if ( sem_wait( resSems[res] ) == -1){ // get resource
    perror( "sem_wait" );
    exit( 1 );
  }
  
  printf( "Baker %d got %s.\n", n, res_arr[res].name );
}

void release( int n, int res ){
  printf( "Baker %d releasing %s.\n", n, res_arr[res].name );
  
  if ( sem_post( resSems[res] ) == -1){ // return resource
    perror( "sem_post" );
    exit( 1 );
  }
  
  poll( 0, 0, 50 );  // 50 ms = edge of visual perception
}

void pantry( int n, int rec ){
  int i;
  
  if ( ! recipes[rec].pIngrCt ){
    return ;
  }
  
  getRes( n, PANTRY_R );
  
  for ( i = 0 ; i < recipes[rec].pIngrCt ; i++ ){
    printf( "Baker %d gets %s.\n", n, recipes[rec].pantryIngr[i] );
  }
  
  sleep( 1 );
  
  release( n, PANTRY_R );
}

void refrigerator( int n, int rec ){
  int i;
  
  if ( ! recipes[rec].rIngrCt ){
    return ;
  }
  
  getRes( n, FRIDGE_R );
  
  for ( i = 0 ; i < recipes[rec].rIngrCt ; i++ ){
    printf( "Baker %d gets %s.\n", n, recipes[rec].refrigIngr[i] );
  }
  
  sleep( 1 );
  
  release( n, FRIDGE_R );
}    
  
int bake( int n, int rec ){
  printf( "Baker %d Begins baking %s.\n", n, recipes[rec].name );  
  getRes( n, BOWL_R); // get bowl
  getRes( n, SPOON_R ); // get spoon
  pantry( n, rec );
  refrigerator( n, rec );
  getRes( n, MIXER_R );
  sleep( 5 );
  release( n, MIXER_R );
  release( n, BOWL_R );
  release( n, SPOON_R );

  //if (n == 0 && (time(NULL) & 1)){ // baker 0 has 50% chance of starting over
  // printf("Baker %d is Ramsied.\n\n", n);
    //return 0;
  //}

  srand(time(NULL)); // Seed the random number generator with the current time

  if (n == 0 && rand() % 2 == 1) { // 50% chance of starting over
    printf("Baker %d is Ramsied.\n\n", n);
    return 0;
  }

  getRes( n, OVEN_R );
  sleep ( 10 );
  release( n, OVEN_R );
  printf( "Baker %d finished baking %s.\n\n", n, recipes[rec].name );
  return 1;
}

void * baker( void * a ){
  long n = (long)a ;
  int i;
  
  printf( "Baker %ld starting.\n", n );

  for ( i = 0 ; i < RECIPE_CT ; i++ ){
    while ( ! bake( n, (i + n) % RECIPE_CT )); // if ramsied repeat 
  }
  
  printf( "Baker %ld done.\n", n );
  return NULL;
}

int main(){
  //int j ;
  long bakers = -1L, i ;
  pthread_t * ptt ;
  
  for (int i = 0; i < RESOURCE_CT; i++) {
    sem_unlink( res_arr[i].name );
    resSems[i] = sem_open( res_arr[i].name, O_CREAT | O_EXCL, 0700, res_arr[i].count);

    if (resSems[i] == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
  }
  
  printf( "How many bakers should I create? " );
  scanf( "%ld", &bakers );
  
  if ( bakers < 1 ){
    printf( "Invalid count %ld.\n", bakers );
    return 1;
  }
  
  if ( !(ptt = calloc( bakers, sizeof( pthread_t )))){
    perror( "calloc bakers" );
    return 1;
  }
  
  i = bakers; // make a copy for the join loop
  
  while ( bakers ){ // spin off all bakers
    int e;
    
    bakers--;
    
    if ( 0 > (e = pthread_create( ptt + bakers, NULL, baker, (void *)bakers ))){
      printf( "pthread_create returns %d\n", e );
      return 1;
    }
  }
  
  while ( i ){ // wait for all bakers
    pthread_join( ptt[--i], NULL );
  }
  
  free( ptt ); 
  ptt = NULL ; 

  // Close and unlink the named semaphores when they're no longer needed
  for (int i = 0; i < RESOURCE_CT; i++) {
    sem_close(resSems[i]);
    sem_unlink( res_arr[i].name );
  }
  return 0;
}
