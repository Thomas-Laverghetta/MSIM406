#pragma once
#include "mpi.h"
#include <thread>
#include "Barrier.h"
#include <chrono>
#include <fstream>

/* Tests Butterfly Barrier by implementing a rand work delay before calling barrier. 
Therefore, should see barrier not finish until all the random delays are over*/
void Butterfly_Barrier_Delay_Tester() {
	int _processID;
	int _numProcess;

	// init MPI
	MPI_Init(NULL, NULL);

	// get this process' ID
	MPI_Comm_rank(MPI_COMM_WORLD, &_processID);

	// get the number of processors
	MPI_Comm_size(MPI_COMM_WORLD, &_numProcess);

	/* initialize random seed with process ID*/
	srand(_processID * 4);

	unsigned int time_delay;
	for (int it = 0; it < 5; it++) {
		// calculate rand time delay between (0, 9)-seconds
		time_delay = 2 + rand() % 6;

		// sleeping for time delay
		std::this_thread::sleep_for(std::chrono::seconds(time_delay));

		// displaying time delay 
		printf("P %i | FINISHED WORK\n", _processID); fflush(stdout);

		// calling barrier to sync
		Butterfly_Barrier();

		// displaying time delay 
		printf("P %i | FINISHED BARRIER\n", _processID); fflush(stdout);
	}
}

