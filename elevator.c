#include "hw6.h"
#include <stdio.h>
#include<pthread.h>
#include <stdlib.h>


struct Elevator {  //Elevator struct to keep track of his contents
	int to_floor;
  pthread_mutex_t lock;
	pthread_barrier_t barrier;
	int current_floor;
	int direction;
	int occupancy;
	int passengers;
	int num_p;
	enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;	
}elevators[ELEVATORS];


struct Passenger {	//To keep track of each passengers details
	int elevator;
	int from_floor;
	int to_floor;
}passengers[PASSENGERS];

struct Floor {	//Floor struct to see how many passengers on a floor
	int num_p;
	int id;
}floors[FLOORS];

int countP = 0;

void scheduler_init() {	
 		int i = 0;
		int j = -1;
		for (i = 0;i < ELEVATORS; i++) { 	
			elevators[i].current_floor = 0;	
			elevators[i].direction = -1;
			elevators[i].occupancy = 0;
			elevators[i].passengers = -1;	
			elevators[i].state = ELEVATOR_ARRIVED;
			elevators[i].num_p = 0;
			pthread_mutex_init(&elevators[i].lock,0);
			pthread_barrier_init(&elevators[i].barrier,0,2);

			elevators[i].to_floor = -1;			
		}
		for(i = 0; i < PASSENGERS; i++) { 
			passengers[i].elevator = ++j;
			elevators[j].num_p++;
			if ( j == ELEVATORS-1) {
				j = -1;
			
			}
		}
		for (i=0; i < FLOORS; i++) {
			floors[i].num_p = 0;
		}	
}


void passenger_request(int passenger, int from_floor, int to_floor, 
											 void (*enter)(int, int), 
											 void(*exit)(int, int))
{	
	// wait for the elevator to arrive at our origin floor, then get in
	
	int waiting = 1;
	int passElevator = passengers[passenger].elevator;
	//elevators[passElevator].to_floor = to_floor;
	//log(0,"%s %d %s %d\n","The passenger ",passenger, "The to floor", elevators[passElevator].to_floor);
	passengers[passenger].from_floor = from_floor;
	passengers[passenger].to_floor = to_floor;
	//log(0,"%s %d","To FLoor: ",elevators[passElevator].to_floor);
	floors[from_floor].id = passenger;
	floors[from_floor].num_p += 1;
	countP++; 
	
	//log(0,"%s %d %s %d\n","Passenger:",passenger,"num P:",floors[from_floor].num_p);
	while(waiting) {

		pthread_barrier_wait(&elevators[passElevator].barrier);

		pthread_mutex_lock(&elevators[passElevator].lock);
		if(elevators[passElevator].current_floor == from_floor && elevators[passElevator].state == ELEVATOR_OPEN && elevators[passElevator].occupancy==0) {
			enter(passenger, passElevator);
			elevators[passElevator].to_floor = to_floor;
			elevators[passElevator].occupancy++;
			waiting=0;
		}
		
		pthread_mutex_unlock(&elevators[passElevator].lock);

		pthread_barrier_wait(&elevators[passElevator].barrier);
	}

	// wait for the elevator at our destination floor, then get out
	int riding=1;
	while(riding) {
		pthread_barrier_wait(&elevators[passElevator].barrier);

		pthread_mutex_lock(&elevators[passElevator].lock);
		//log(0,"%s %d %s %d %d\n", "Current Floor",elevators[passElevator].current_floor,"to_floor",to_floor,elevators[passElevator].state);
		if(elevators[passElevator].current_floor == to_floor && elevators[passElevator].state == ELEVATOR_OPEN) {
			//log(0,"%s", "R u at ur floor");
			exit(passenger, passElevator);
			elevators[passElevator].occupancy--;
			floors[from_floor].num_p -= 1;
			elevators[passElevator].to_floor = -1;
			countP--;
			elevators[passElevator].num_p--;
			riding=0;
		}

		pthread_mutex_unlock(&elevators[passElevator].lock);

		pthread_barrier_wait(&elevators[passElevator].barrier);

	}
}

void elevator_ready(int elevator, int at_floor, 
										void(*move_direction)(int, int), 
										void(*door_open)(int), void(*door_close)(int)) {
	
	
	//log(0,"%s %d\n","CountP:",countP);
  //if (countP <= 0) { 			//If no more passengers to serve than exit
	//	exit(0);
		//return;
 // }
	if (elevators[elevator].num_p <= 0) {	// If their is no more passengers that need to be served for this elevator return
    return;
  }
	
	pthread_barrier_wait(&elevators[elevator].barrier);
	//pthread_mutex_lock(&elevators[elevator].lock);
	//log(0,"%s %d %s %d %d\n","Elevator to floor",elevators[elevator].to_floor,"At_floor",at_floor,elevators[elevator].state);

	
	if((elevators[elevator].state == ELEVATOR_ARRIVED) && 
	((floors[at_floor].num_p > 0 && elevators[elevator].occupancy == 0) || (elevators[elevator].to_floor == at_floor )))  {	//open door if their is a passenger at that floor and the elevator does not have any passengers on it or if the passengers floor is reached
			//log(0,"%s","Inside door open\n");
			door_open(elevator);
			elevators[elevator].state=ELEVATOR_OPEN;
	}
		
	else if(elevators[elevator].state == ELEVATOR_OPEN) {
		door_close(elevator);
		elevators[elevator].state=ELEVATOR_CLOSED;
	}	

	else {
		if(at_floor==0 || at_floor==FLOORS-1 ) {
			elevators[elevator].direction*=-1;
		}
		else {		//Saves the elevator time by changing directions if it is going away from the pasengers floor
     		if(elevators[elevator].to_floor > at_floor && elevators[elevator].to_floor != -1 ) {
     			if (elevators[elevator].direction > 0) {
    
      		}   
      		else{
        		elevators[elevator].direction *= -1; 
      		}		   
    		}   
    		else if (elevators[elevator].to_floor < at_floor && elevators[elevator].to_floor != -1 ) {
       		if (elevators[elevator].direction < 0) {
   
    	    }   
      	  else {
        	  elevators[elevator].direction *= -1; 
       	 	}	   
   	 		}	   
 	 		}  
		move_direction(elevator,elevators[elevator].direction);
		elevators[elevator].current_floor=at_floor+elevators[elevator].direction;
		elevators[elevator].state=ELEVATOR_ARRIVED; 
	}
	pthread_mutex_unlock(&elevators[elevator].lock);
	pthread_barrier_wait(&elevators[elevator].barrier);
	//if (elevators[elevator].to_floor < 0) {
	//	return;
	//}
	
}




