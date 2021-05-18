/*
 * sumsq.c
 *
 * CS 446.646 Project 1 (Pthreads)
 *
 * Compile with --std=c99
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// aggregate variables
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
bool done = false;
bool multiThreading = true;

////////////////////////////////////////////////////////


pthread_mutex_t data_lock = PTHREAD_MUTEX_INITIALIZER;

////////////////////////////////////////////////////////

struct Node{

    long data;
    struct Node* next;

};

struct argStruct{

    pthread_mutex_t *condMutex;
    pthread_cond_t *condVar;
    int threadNum;

};

struct Node* head = NULL;

// function prototypes//////////////////////////////////

void calculate_square(long number);
void *workerThread();

////////////////////////////////////////////////////////


void *workerThread(void *arguments){
    
    int number = 0;
    struct argStruct *args = arguments;
    
    while(1){

        
        pthread_mutex_lock( &data_lock );        
            
        //if there is a node in queue;
        if(head != NULL){
        
            number = head->data; //pull number
            
            
            calculate_square(number); //dump it into calculate square.
            
            if(head->next != NULL){ //if there is another node in queue
                
                struct Node* temp = head->next; //save next node
                free(head); //deallocate node
                head = temp; //set it to next again
                
                
            }else{
            
                free(head); //deallocate node
                head = NULL; //set to null again
            
            }
            
            pthread_mutex_unlock( &data_lock );        
            
            sleep(number);

            
        }else{
        
            pthread_mutex_unlock( &data_lock );        

        }
        
        if(done != true){   
            pthread_mutex_lock( args->condMutex );
            
            pthread_cond_wait( args->condVar, args->condMutex );
            
            pthread_mutex_unlock( args->condMutex );

            
        }else{
        
            return 0;
        
        }

    
    }
    


}


////////////////////////////////////////////////////////

/*
 * update global aggregate variables given a number
 */
void calculate_square(long number)
{

  // calculate the square
  long the_square = number * number;


  // let's add this to our (global) sum
  sum += the_square;

  // now we also tabulate some (meaningless) statistics
  if (number % 2 == 1) {
    // how many of our numbers were odd?
    odd++;
  }

  // what was the smallest one we had to deal with?
  if (number < min) {
    min = number;
  }

  // and what was the biggest one?
  if (number > max) {
    max = number;
  }
}


int main(int argc, char* argv[])
{
  // check and parse command line options
  if (argc < 2 || argc > 3) {
    printf("Usage: sumsq <infile>\n");
    exit(EXIT_FAILURE);
  }
  char *fn = argv[1];
  
  //////////////////////////////////////////////////////////////////////////////////
  //parse command line for num threads

  int numThreads = 0;
  pthread_cond_t **condVarList = NULL; //list of cond variables
  pthread_t *threadIDs = NULL;
  char *p;

  if(argc == 3){

      numThreads = strtol(argv[2],&p,10);
      if(numThreads != 0){  

      threadIDs=(pthread_t *)malloc(numThreads * sizeof(pthread_t));
  
      condVarList=(pthread_cond_t **)malloc(numThreads * sizeof(pthread_cond_t));
  
      for(int i = 0; i < numThreads;i++){

           struct argStruct* newArgs = (struct argStruct*)malloc(sizeof(struct argStruct));

  
           newArgs->condMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
           newArgs->condVar = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
           newArgs->threadNum = i;

          condVarList[i] = newArgs->condVar;  
  
          pthread_create(&threadIDs[i],NULL,workerThread,(void *)newArgs);
  
        }
    }else{
        multiThreading = false;
    }
      
  }else{
    multiThreading = false;
  }
  
  
  //////////////////////////////////////////////////////////////////////////////////
  // load numbers and add them to the queue
  FILE* fin = fopen(fn, "r");
  char action;
  long num;

  while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
    if (action == 'p' && multiThreading) {            // process, do some work
      //calculate_square(num);
      
      struct Node *tail = head;  /* used in step 5*/
      
      //make new node
      struct Node* newNode = (struct Node*) malloc(sizeof(struct Node));
      newNode->data = num; //insert number into node
      newNode->next = NULL;
      
      //if nothing in queue
      if(head == NULL){ 
        head = newNode;
      }else{
        //traverse to end
        while(tail->next != NULL){
            tail = tail->next;
        }
        tail->next = newNode;
      }
       
      for(int i = 0; i < numThreads; i++){
         pthread_cond_broadcast( condVarList[i] );      
      
      }
      
    } else if(action == 'p' && !multiThreading){
    
        calculate_square(num);
        sleep(num);
    
    }else if (action == 'w') {     // wait, nothing new happening
      sleep(num);


    } else {
      printf("ERROR: Unrecognized action: '%c'\n", action);
      exit(EXIT_FAILURE);
    }
  }
  fclose(fin);
  
  while(head != NULL){
    sleep(.00001);
    for(int i = 0; i < numThreads; i++){
         pthread_cond_broadcast( condVarList[i] );           
    }
    
  }
  
  done = true;
  for(int i = 0; i < numThreads; i++){
         pthread_cond_broadcast( condVarList[i] );      
      
  }
  
  for(int i=0; i<numThreads; i++)
    {
        pthread_join(threadIDs[i],NULL);
    }
  
  // print results
  printf("%ld %ld %ld %ld\n", sum, odd, min, max);
  
  // clean up and return
  return (EXIT_SUCCESS);
}

