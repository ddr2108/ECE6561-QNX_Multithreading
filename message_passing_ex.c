// message passing example for QNX, 
// Filename: message_passing_ex.c 
#include <stdlib.h> 
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>


// Parameters structure
typedef struct _parameters
{ 
	float alpha1;
	float alpha2;
	float beta1;
	float beta2;
	float beta3;
} parameters;

//Data to be shared
parameters fastFilter;
//Mutex for sharing
pthread_mutex_t paramMutex;


void* fastFilterThread(void*);
void* slowFilterThread(void*);

//  Start processes 
int main( void )
{
    // give threads IDs 
   	pthread_t fastFilterThreadID;
   	pthread_t slowFilterThreadID;

   	//Initialize parameters
   	fastFilter.alpha1 = -1.691;
   	fastFilter.alpha2 = 0.7327;
   	fastFilter.beta1 = 0.0104;
   	fastFilter.beta2 = 0.0209;
   	fastFilter.beta3 = 0.0104;

	//Initialize mutex
	pthread_mutex_init(&paramMutex, NULL);

	//Create the threads 
   	pthread_create(&fastFilterThreadID, NULL, fastFilterThread, NULL);
   	pthread_create(&slowFilterThreadID, NULL, slowFilterThread, NULL);
	
	//Wait for threads to finish
   	pthread_join(fastFilterThreadID, NULL);
   	pthread_join(slowFilterThreadID, NULL);
	exit(0);
}


//Fast filter thread
void* fastFilterThread(void* unUsed){
	//Buffer for IO
	char buffer[10];

	//Values for filter
	float x, x1, x2;
	float y, y1, y2;

	//File pointer
	FILE *fpRead, *fpWrite;			

	//Open file
	fpRead = fopen("Data1.txt", "r");
	fpWrite = fopen("Output.txt", "w");
        	
    while(fscanf(fpRead, "%f", &x) != EOF){
		
    	//Calculate new value
    	pthread_mutex_lock(&paramMutex);
		y = fastFilter.beta1*x + fastFilter.beta2*x1 + fastFilter.beta3*x2 - fastFilter.alpha1*y1 - fastFilter.alpha2*y2;
    	pthread_mutex_unlock(&paramMutex);

		//Ouput new value to text file
		fprintf(fpWrite, "%f", y);

		//Adjust values
		x2 = x1;
		x1 = x;
		y2 = y1;
		y1 = y;

		//Sleep till next sample
		usleep(50000);				
    }
    
    //Close file streams
    fclose(fpRead);
    fclose(fpWrite);

    //Exit thread and return  
    pthread_exit(0);
    return(NULL);
  
}

//Slow filter thread
void* slowFilterThread(void* unUsed){

 }