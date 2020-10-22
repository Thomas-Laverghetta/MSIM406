#include <math.h>
#include "mpi.h"
#include "Barrier.h"
#include <thread>
#include <chrono>

#define MPI_BarrierTag 1000
#define BARRIER_DEBUG

void Ring_Barrier()
{
	//ring barrier
	int rank;
	MPI_Comm_rank( MPI_COMM_WORLD, &rank);
	int nproc;
	MPI_Comm_size( MPI_COMM_WORLD, &nproc);
	int source;
	MPI_Status status;

	/// Ring Barrier /// 
	if (nproc > 1) {
		if (rank == 0) {
			MPI_Send( &rank, 1, MPI_INTEGER, 1, MPI_BarrierTag, MPI_COMM_WORLD);
			MPI_Recv( &source, 1, MPI_INTEGER, nproc-1, MPI_BarrierTag, MPI_COMM_WORLD, &status);
			MPI_Send( &rank, 1, MPI_INTEGER, 1, MPI_BarrierTag, MPI_COMM_WORLD);
			MPI_Recv( &source, 1, MPI_INTEGER, nproc-1, MPI_BarrierTag, MPI_COMM_WORLD, &status); }
		else {
			MPI_Recv( &source, 1, MPI_INTEGER, (rank-1) % nproc, MPI_BarrierTag, MPI_COMM_WORLD, &status);
			MPI_Send( &rank, 1, MPI_INTEGER, (rank+1) % nproc, MPI_BarrierTag, MPI_COMM_WORLD);
			MPI_Recv( &source, 1, MPI_INTEGER, (rank-1) % nproc, MPI_BarrierTag, MPI_COMM_WORLD, &status);
			MPI_Send( &rank, 1, MPI_INTEGER, (rank+1) % nproc, MPI_BarrierTag, MPI_COMM_WORLD); }}
}


void Butterfly_Barrier() {
	//ring barrier
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);	// getting this process' ID
	int nproc;
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);	// getting number of ids
	int source;
	MPI_Status status;

	// BF == bitflip
	for (int BF = 0; BF < log(nproc) / log(2); BF++) {
		// calculate the next process to send message to
		int nextProcess = rank ^ (1 << BF);

		// initate communication to next process in butterfly barrier to signal finished with work
		MPI_Send(&rank, 1, MPI_INTEGER, nextProcess, MPI_BarrierTag, MPI_COMM_WORLD);

		// wait for response from next process to indicate they are done 
		MPI_Recv(&source, 1, MPI_INTEGER, nextProcess, MPI_BarrierTag, MPI_COMM_WORLD, &status);

#ifdef BARRIER_DEBUG
		// display layer
		// printf("P %i | RECV %i | layer %i\n", rank, source, BF); fflush(stdout);
#endif // BARRIER_DEBUG

	}	
}
