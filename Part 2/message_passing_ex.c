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
	float timeCur;
	float leftVel;
	float rightVel;
	float heading;
	float x;
	float y;
} parametersTrajectory;

typedef struct _parametersVelocity{ 
	float timeCur;
	float leftVel;
	float rightVel;
} parametersVelocity;

typedef struct _parametersDesired{ 
	float timeCur;
	float velocity;
	float turningRate;
} parametersDesired;


pthread_barrier_t barrier;  // for synchronizing threads

//Function definitions
void* trajectoryThread(void*);
void* velocityThread(void*);
void* sensorThread(void*);
int rightWheelSensor(void*);
int leftWheelSensor(void*);


/*
* rightWheelSensor
* Gets velocity from right wheel
*
* params:
* none
*
* returns:
* float - velocity
*/
float rightWheelSensor(void*){
	return 0.5;
}

/*
* leftWheelSensor 
* Gets velocity from left wheel
*
* params:
* none
*
* returns:
* float - velocity
*/
float leftWheelSensor(void*){
	return 0.5;
}


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
	//Final Goal
	float xGoal = 1;
	float yGoal = 1;
	float angle = 3.14/2;
	//Control System parameters
	float kp = 1;
	float kalpha = 1;
	float kphi = 1;
	//Variables for calulations
	float rho, alpha, phi, deltaX, deltaY;
	float vel, omega;

	int retVal;						//Return value from message queue
	mqd_t sensorToTrajectoryQueue, trajectoryToVelocityQueue;	//Messeging queue
	parametersTrajectory paramIn;		//Parameters to be passed
	parametersDesired paramOut; 
	FILE *fpWrite;					//File pointer

	//Open file
	fpWrite = fopen("Trajectory.txt", "w");
	           
	//Open messaging queue           
	sensorToTrajectoryQueue = mq_open("sensorToTrajectoryQ", O_RDONLY|O_CREAT, S_IRWXU, NULL);
	trajectoryToVelocityQueue = mq_open("trajectoryToVelocityQ", O_WRONLY|O_CREAT|O_NONBLOCK, S_IRWXU, NULL);

	//Error check messaging queue
	if(sensorToTrajectoryQueue < 0){
		printf("Trajectory Thread has Failed to Create Sensor Queue\n");
	}else{    
		printf("Trajectory Thread has Created Sensor Queue!\n");
	}
   //Error check messaging queue
	if(trajectoryToVelocityQueue < 0){
		printf("Trajectory Thread has Failed to Create Velocity Queue\n");
	}else{    
		printf("Trajectory Thread has Created Velocity Queue!\n");
	}
	//Wait here until sensor thread has created its message queue
	pthread_barrier_wait(&barrier);

	while(1){
		//Block until data recieved
		retVal = mq_receive(sensorToTrajectoryQueue, (char*)&paramIn, sizeof(paramIn), NULL);
		//Error on recieve
		if(retVal< 0){
			printf("Error receiving from queue\n" );
			perror("Trajectory Thread");
		}
            
        //Do processing 
        deltaX = xGoal - paramIn.x;
        deltaY = yGoal-paramIn.y;
		rho = sqrt(pow(deltaX),2) + pow((deltaY),2));
		alpha = -paramIn.heading+atan(deltaX/deltaY);
		phi = -paramIn.heading + angle;
		if (phi<-3.14/2){
			while (phi<-3.14/2){
				phi+=6.28;
			}
		}else if (phi>3.14/2){
			while (phi>3.14/2){
				phi-=6.28;
			}
		}

		//Find output values
		omega = kalpha*alpha + kphi*phi;
		vel = kp*rho;
		//Set output
		paramOut.timeCur = paramIn.timeCur;
		paramOut.velocity = vel;
		paramOut.turningRate = omega;

		//Send data
		retVal = mq_send(trajectoryToVelocityQueue, (char*)&paramOut, sizeof(paramOut), 0);
		if(retVal < 0){
				printf("Trajectory Send to Sensor Thread Failed!\n");		
		}

        //Output data
        fprintf(fpWrite, "%f %f %f\n", paramOut.timeCur,paramOut.velocity,paramOut.turningRate);
	}

	fclose(fpWrite);

	//Exit thread
	pthread_exit(0);
	return(NULL);
}

void* velocityThread(void* unUsed){
	//current targets
	float leftVel = 0;
	float rightVel = 0;
	float heading = 0;

	//Controller parameters
	float kp = 1;
	float ki = 1;

	float u, u1, e, e1;
	float leftCMD, rightCMD;

	int retVal;						//Return value from message queue
	mqd_t sensorToVelocityQueue, trajectoryToVelocityQueue;	//Messeging queue
	parametersVelocity paramInSensor;		//Parameters to be passed
	parametersDesired paramInControl; 
	FILE *fpWrite;					//File pointer

	u = u1 = e = e1 = 0;

	//Open file
	fpWrite = fopen("Velocity.txt", "w");
	           
	//Open messaging queue           
	sensorToVelocityQueue = mq_open("sensorToVelocityQ", O_RDONLY|O_CREAT, S_IRWXU, NULL);
	trajectoryToVelocityQueue = mq_open("trajectoryToVelocityQ", O_RDONLY|O_CREAT|O_NONBLOCK, S_IRWXU, NULL);

	//Error check messaging queue
	if(sensorToVelocityQueue < 0){
		printf("Velocity Thread has Failed to Create Sensor Queue\n");
	}else{    
		printf("Velocity Thread has Created Sensor Queue!\n");
	}
    if(trajectoryToVelocityQueue < 0){
		printf("Velocity Thread has Failed to Create Trajectory Queue\n");
	}else{    
		printf("Velocity Thread has Created Trajectory Queue!\n");
	}
	//Wait here until sensor thread has created its message queue
	pthread_barrier_wait(&barrier);

	while(1){
		//Block until data recieved
		retVal = mq_receive(sensorToVelocityQueue, (char*)&paramInSensor, sizeof(paramInSensor), NULL);
		//Error on recieve
		if(retVal< 0){
			printf("Error receiving from queue\n" );
			perror("Velocity Thread");
		}

		//See if any data to be recieved
		retVal = mq_receive(sensorToVelocityQueue, (char*)&paramInControl, sizeof(paramInControl), NULL);
		if(retVal >= 0){
			leftVel = paramInControl.velocity;
			rightVel = paramInControl.velocity;
			heading = paramInControl.turningRate;
		}

        //Do processing
        e =  paramInSensor.leftVel - paramInSensor.rightVel;
        u = u1 + ki(e + e1);
        e1 = e;
        u1 = u;
		leftCMD = (leftVel-paramInSensor.leftVel-u)*kp;
		rightCMD = (rightVel-paramInSensor.rightVel-u)*kp;

        //Output data
        fprintf(fpWrite, "%f %f %f %f %f\n", paramInSensor.timeCur, (velLeft+velRight)/2, heading, rightCMD, leftCMD);
	}

	fclose(fpWrite);

	//Exit thread
	pthread_exit(0);
	return(NULL);
 }


void* sensorThread(void* unUsed){
	float leftVel, rightVel;
	float distance, heading, prevTime;
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

    //Initialize Location
    prevTime = cycle0;
    distance = 0;
    heading = 3.14/4;

	//Open messaging queues	           
	sensorToTrajectoryQueue = mq_open("sensorToTrajectoryQ", O_WRONLY|O_CREAT|O_NONBLOCK, S_IRWXU, NULL);
	sensorToVelocityQueue = mq_open("sensorToVelocityQ", O_WRONLY|O_CREAT|O_NONBLOCK, S_IRWXU, NULL);
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

		//Get Data
		leftVel = rightWheelSensor();
		rightVel = leftWheelSensor();

		//Set the parameters for velocity
		parametersVelocity.timeCur = timeCur;			
		parametersVelocity.leftVel = leftVel; 					
		parametersVelocity.rightVel = rightVel;

		//calculate values for trajectory
		heading = 3.14/4;			//Calculations would happen if there was real data
		distance += (rightVel+leftVel)/2*(timeCur - prevTime);

		//Set the parameters for trajectory
		paramToTrajectory.timeCur = timeCur;
		parametersTrajectory leftVel = leftVel;
		paramToTrajectory.rightVel = rightVel;
	 	paramToTrajectory.heading = prevHeading;
	 	paramToTrajectory.x = distance * cos(heading);
		paramToTrajectory.y = distance * sin(heading);

		//At 100Hz
		//Send data to velocity controller
		retVal = mq_send(sensorToVelocityQueue, (char*)&paramToVelocity, sizeof(paramToVelocity), 0);
		if(retVal < 0){
				printf("Sensor Send to Velocity Thread Failed!\n");		
		}

		//At 10Hz
		if (ticks%10 == 0){
			//Send data to trajectory controller
			retVal = mq_send(sensorToTrajectoryQueue, (char*)&paramToTrajectory, sizeof(paramToTrajectory), 0);
			if(retVal < 0){
				printf("Sensor Send to Velocity Thread Failed!\n");
			}
		}	

		//Clock
		prevTime = timeCur;
		ticks++;		
		delay(10);				//Delay for 10ms
	}
}