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
#define REFER_R 2
#define BOWL_R 3
#define SPOON_R 4
#define OVEN_R 5

sem_t *sems[RESOURCE_CT]; // one semaphore per resource

struct res{ // parallel array to sems
  int ct;
  char * name ;
} res_arr[] = { { 2, "Mixer" }, 
                { 1, "Pantry" },
                { 2, "Refrigerator" },
                { 3, "Bowl" },
                { 5, "Spoon" },
                { 1, "Oven" } } ;

struct recipe {
  char * name ;
  char * p_ingr[5];
  char p_ct ;
  char * r_ingr[3];
  char r_ct ;
} recipes[] = { { "Cookies", { "Flour", "Sugar" }, 2, { "Milk", "Butter" }, 2 },
                 { "Pancakes", { "Flour", "Sugar", "Baking soda", "Salt" }, 4, { "Egg", "Milk", "Butter" }, 3 }, 
                 { "Homemade pizza dough", { "Yeast", "sugar", "salt" }, 3, { }, 0 },
                 { "Soft Pretzels", { "Flour", "sugar", "salt", "yeast", "baking soda" }, 5, { "Egg" }, 1 },
                 { "Cinnamon rolls", { "Flour", "sugar", "salt", "cinnamon" }, 4, { "butter", "eggs" }, 2 } };

void get( int n, int res ){
  printf( "Baker %d waiting for %s.\n", n, res_arr[res].name );
  
  if ( sem_wait( sems[res] ) == -1){ // get resource
    perror( "sem_wait" );
    exit( 1 );
  }
  
  printf( "Baker %d got %s.\n", n, res_arr[res].name );
}

void release( int n, int res ){
  printf( "Baker %d releasing %s.\n", n, res_arr[res].name );
  
  if ( sem_post( sems[res] ) == -1){ // return resource
    perror( "sem_post" );
    exit( 1 );
  }
  
  poll( 0, 0, 50 );  // 50 ms = edge of visual perception
}

void pantry( int n, int rec ){
  int i;
  
  if ( ! recipes[rec].p_ct ){
    return ;
  }
  
  get( n, PANTRY_R );
  
  for ( i = 0 ; i < recipes[rec].p_ct ; i++ ){
    printf( "Baker %d gets %s.\n", n, recipes[rec].p_ingr[i] );
  }
  
  sleep( 1 );
  
  release( n, PANTRY_R );
}

void refer( int n, int rec ){
  int i;
  
  if ( ! recipes[rec].r_ct ){
    return ;
  }
  
  get( n, REFER_R );
  
  for ( i = 0 ; i < recipes[rec].r_ct ; i++ ){
    printf( "Baker %d gets %s.\n", n, recipes[rec].r_ingr[i] );
  }
  
  sleep( 1 );
  
  release( n, REFER_R );
}    
  
void bake( int n, int rec ){
  printf( "Baker %d Begins baking %s.\n", n, recipes[rec].name );  
  get( n, BOWL_R); // get bowl
  get( n, SPOON_R ); // get spoon
  pantry( n, rec );
  refer( n, rec );
  get( n, MIXER_R );
  sleep( 5 );
  release( n, MIXER_R );
  release( n, BOWL_R );
  release( n, SPOON_R );
  get( n, OVEN_R );
  sleep ( 10 );
  release( n, OVEN_R );
  printf( "Baker %d finished baking %s.\n\n", n, recipes[rec].name );
}

void * baker( void * a ){
  long n = (long)a ;
  int i;
  
  printf( "Baker %ld starting.\n", n );

  for ( i = 0 ; i < RECIPE_CT ; i++ ){
    bake( n, (i + n) % RECIPE_CT );
  }
  
  printf( "Baker %ld done.\n", n );
  return NULL;
}

int main(){
  int j ;
  long bakers = -1L, i ;
  pthread_t * ptt ;
  
  for (int i = 0; i < RESOURCE_CT; i++) {
    sem_unlink( res_arr[i].name );
    sems[i] = sem_open( res_arr[i].name, O_CREAT | O_EXCL, 0700, res_arr[i].ct);

    if (sems[i] == SEM_FAILED) {
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
  
  free( ptt ); // unnecessary but nice
  ptt = NULL ; // unnecessary but nice

  // Close and unlink the named semaphores when they're no longer needed
  for (int i = 0; i < RESOURCE_CT; i++) {
    sem_close(sems[i]);
    sem_unlink( res_arr[i].name );
  }
  return 0;
}
