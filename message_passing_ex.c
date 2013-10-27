// message passing example for QNX, 
// Filename: message_passing_ex.c 

#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>

pthread_barrier_t barrier;  // for synchronizing threads

// data to be passed
typedef struct _parameters
{ 
	float t_current;
	float alpha;
	float beta;
} parameters;


void* Thread1( void* );
void* Thread2( void* );

//  Start processes 
int main( void )
{
    // give threads IDs 
   pthread_t Thread1ID;
   pthread_t Thread2ID;

   /* Create a barrier, each thread executes until it reaches the barrier,
		and then it blocks until all threads have reached the barrier. This 
		provides a way of synchronizing threads. In this case, two threads so
		the count is 2. */

   pthread_barrier_init( &barrier, NULL, 2);
 
   pthread_create(&Thread1ID, NULL, Thread1, NULL);
   pthread_create(&Thread2ID, NULL, Thread2, NULL);
	
   pthread_join( Thread1ID, NULL );
   pthread_join( Thread2ID, NULL );
   exit(0);
}


//Thread1 will be used to send data to Thread2. 

void * Thread1( void * unUsed)
{
	/* Create the message queue to the other thread. Thread 1 is set as 
	nonblocking, so it continues running once it sends data to the queue */
	mqd_t T1_to_T2_Queue;
	int i, retVal;
	parameters param1;  // structure to hold data, local to Thread 1
	float t;
	uint64_t cps, cycle0;  // accesses a 64 bit machine counter to determine the clock cycle
	           
	T1_to_T2_Queue = mq_open( "T1_to_T2_Q", O_WRONLY|O_CREAT|O_NONBLOCK, S_IRWXU, NULL );
	if( T1_to_T2_Queue < 0 )
	{
		printf( "Thread1 has failed to create queue\n" );
	}
	else
	{    
		printf( "Thread1 has created queue!\n" );
	}
   
	// wait here until Thread 2 has created its message queue
	pthread_barrier_wait( &barrier );

    /* find out how many cycles per second for this machine */
    cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
    printf("This system has %lld cycles/sec. \n",cps);
        /* determine the initial cycle */
    cycle0 = ClockCycles();
    i = 0;
    while( i < 50 )
      {
	param1.t_current = (float)(ClockCycles()-cycle0)/(float)(cps);  //time since initial cycle
	t = param1.t_current;
	param1.alpha = t+10;
	param1.beta = 2*t;
        printf("Thread 1 running, i = %d, t = %f \n", i, t);
        retVal = mq_send( T1_to_T2_Queue, ( char * )&param1, sizeof( param1 ), 0 );
	if( retVal < 0 )
	{
		printf( "sensor mq_send to Thread 2 failed!\n" );
	}
	i++;
	delay( 500 );  // sleep for 500 ms
      }
      
      pthread_exit(0);
      return( NULL );
  
}

void * Thread2 ( void * unUsed)
{
	/* Create the message queue to the other thread. Thread 2 is set as 
	blocking, so it waits until data is sent to the queue */
	mqd_t T1_to_T2_Queue;
	int i, retVal;
	parameters param2;
	float t=0;
	float x, y;
	           
	T1_to_T2_Queue = mq_open( "T1_to_T2_Q", O_RDONLY|O_CREAT, S_IRWXU, NULL );
	// could have made both O_RDWR for flexibility

	if( T1_to_T2_Queue < 0 )
	{
		printf( "Thread2 has failed to create queue\n" );
	}
	else
	{    
		printf( "Thread2 has created queue!\n" );
	}
   
	// wait here until Thread 1 has created its message queue
	pthread_barrier_wait( &barrier );
        i = 0;
	while( i < 50 )
	{
		retVal = mq_receive( T1_to_T2_Queue, (char*) &param2,4096,NULL );
		if( retVal < 0 )
		{
			printf("Error receiving from queue\n" );
			perror( "Thread2" );
		}
            
		t = param2.t_current;
		x = 2* param2.alpha;
		y = 2* param2.beta;
		printf( "Thread 2: %f %f %f %f %f\n", t, param2.alpha, param2.beta, x, y );
		i++;
	}
	pthread_exit(0);
	return( NULL );
 }