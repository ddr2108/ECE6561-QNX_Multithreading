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

//Structures of data to be passed
typedef struct _parametersTrajectory{ 
	float t_current;
	float alpha;
	float beta;
} parametersTrajectory;

typedef struct _parametersVelocity{ 
	float t_current;
	float alpha;
	float beta;
} parametersVelocity;

pthread_barrier_t barrier;  // for synchronizing threads

//Function definitions
void* trajectoryThread(void*);
void* velocityThread(void*);
void* sensorThread(void*);

//  Start processes 
int main(void){
   
   //G ive threads IDs 
   pthread_t trajectoryThreadID;
   pthread_t velocityThreadID;
   pthread_t sensorThreadID;

   //Create barrier for synchronization
   pthread_barrier_init( &barrier, NULL, 2);
 
   //Create threads
   pthread_create(&trajectoryThreadID;, NULL, trajectoryThread, NULL);
   pthread_create(&velocityThreadID;, NULL, velocityThread, NULL);
   pthread_create(&sensorThreadID;, NULL, sensorThread, NULL);

   //Finish threads
   pthread_join(trajectoryThreadID, NULL );
   pthread_join(velocityThreadID, NULL );
   pthread_join(sensorThreadID, NULL );
   exit(0);
}



void* trajectoryThread(void* unUsed){
	int retVal;						//Return value from message queue
	mqd_t sensorToTrajectoryQueue;	//Messeging queue
	parametersTrajectory param;		//Parameters to be passed
	FILE *fpWrite;					//File pointer

	//Open file
	fpWrite = fopen("Trajectory.txt", "w");
	           
	//Open messaging queue           
	sensorToTrajectoryQueue = mq_open( "sensorToTrajectoryQ", O_WRONLY|O_CREAT|O_NONBLOCK, S_IRWXU, NULL);
	//Error check messaging queue
	if(sensorToTrajectoryQueue < 0){
		printf("Trajectory Thread has Failed to Create Queue\n");
	}else{    
		printf("Trajectory Thread has Created Queue!\n");
	}
   
	//Wait here until sensor thread has created its message queue
	pthread_barrier_wait(&barrier);

	while(1){
		//Block until data recieved
		retVal = mq_receive(sensorToTrajectoryQueue, (char*)&param, sizeof(param), NULL);
		//Error on recieve
		if(retVal< 0){
			printf("Error receiving from queue\n" );
			perror("Trajectory Thread");
		}
            
        //Do processing 
		param;

        //Output data
        fprintf(fpWrite, "%f %f %f %f %f\n", 1,1,1,1,1);
	}

	//Exit thread
	pthread_exit(0);
	return(NULL);
}

void* velocityThread(void* unUsed){
	int retVal;						//Return value from message queue
	mqd_t sensorToVelocityQueue;	//Messeging queue
	parametersVelocity param;		//Parameters to be passed
	FILE *fpWrite;					//File pointer

	//Open file
	fpWrite = fopen("Velocity.txt", "w");
	           
	//Open messaging queue           
	sensorToVelocityQueue = mq_open( "sensorToVelocityQ", O_WRONLY|O_CREAT|O_NONBLOCK, S_IRWXU, NULL);
	//Error check messaging queue
	if(sensorToVelocityQueue < 0){
		printf("Velocity Thread has Failed to Create Queue\n");
	}else{    
		printf("Velocity Thread has Created Queue!\n");
	}
   
	//Wait here until sensor thread has created its message queue
	pthread_barrier_wait(&barrier);

	while(1){
		//Block until data recieved
		retVal = mq_receive(sensorToVelocityQueue, (char*)&param, sizeof(param), NULL);
		//Error on recieve
		if(retVal< 0){
			printf("Error receiving from queue\n" );
			perror("Velocity Thread");
		}
            
        //Do processing 
		param;

        //Output data
        fprintf(fpWrite, "%f %f %f %f %f\n", 1,1,1,1,1);
	}

	//Exit thread
	pthread_exit(0);
	return(NULL);
 }


void* sensorThread(void* unUsed){
	int retVal;
	int ticks = 0;		//Clock
	uint64_t cps, cycle0;
	float timeCur;
	parametersVelocity paramToVelocity;			//Parameters to be passed
	parametersTrajectory paramToTrajectory; 	
	mqd_t sensorToVelocityQueue, sensorToTrajectoryQueue;	//Message Queues

	//Do some initial clock setup
    cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
    cycle0 = ClockCycles();

	//Open messaging queues	           
	sensorToTrajectoryQueue = mq_open("sensorToTrajectoryQ", O_RDONLY|O_CREAT, S_IRWXU, NULL);
	sensorToVelocityQueue = mq_open("sensorToVelocityQ", O_RDONLY|O_CREAT, S_IRWXU, NULL);
	//Error checking on message queue creation
	if(sensorToTrajectoryQueue < 0){
		printf("Sensor Thread has Failed to Create Trajectory Queue\n");
	}else{    
		printf("Sensor Thread has Created Trajectory Queue!\n");
	}
	if(sensorToVelocityQueue < 0){
		printf("Sensor Thread has Failed to Create Velocity Queue\n");
	}else{    
		printf("Sensor Thread has Created Velocity Queue!\n");
	}

	// wait here until sensor thread has created its message queue
	pthread_barrier_wait(&barrier);

	while (1){
		//Clock
		timeCur = (float)(ClockCycles()-cycle0)/(float)(cps);

		//Data Processing
		paramToVelocity;
		paramToTrajectory;

		//At 10Hz
		if (ticks%10 == 0){
			//Send data to velocity controller
			retVal = mq_send(sensorToVelocityQueue, (char*)&paramToVelocity, sizeof(paramToVelocity), 0);
			if(retVal < 0){
					printf("Sensor Send to Velocity Thread Failed!\n");
			}		
		}

		//At 100Hz
		//Send data to velocity controller
		retVal = mq_send(sensorToTrajectoryQueue, (char*)&paramToTrajectory, sizeof(paramToTrajectory), 0);
		if(retVal < 0){
			printf( "Sensor Send to Velocity Thread Failed!\n" );
		}	

		//Clock
		ticks++;		
		delay(10);				//Delay for 10ms
	}
}