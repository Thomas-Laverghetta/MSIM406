#include "SimulationExecutive.h"
#include "EventSet.hpp"
#include "mpi.h"
#include <iostream>
#include <unordered_map>

using namespace std;
// Null msg
class NullEA : public EventAction {
public:
	NullEA() {}
	void Execute() {}
	const int GetBufferSize() { return 0; }
	void Serialize(int* dataBuffer) {}
	void Deserialize(int* dataBuffer) {}
};


//----------------Comm-------------------
int processID = -1;
int numProcess = -1;

void CommunicationInitialize()
{
	int* argc = 0;
	char*** argv = 0;
	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &processID);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcess);
}

void CommunicationFinalize()
{
	cout << processID << " in finalize" << endl;
	MPI_Finalize();
	cout << processID << " done finalize" << endl;
}

int CommunicationRank() { return processID; }

int CommunicationSize(){ return numProcess; }

bool CheckForComm(int& tag, int& source)
{
	MPI_Status status;
	int flag;
	MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
	if (flag) {
		tag = status.MPI_TAG;
		source = status.MPI_SOURCE;
	}
	return(flag == 1);
}

void Send(int dest, int tag, int* dataBuffer, int bufferSize)
{
	MPI_Request request;
	MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, dest, tag, MPI_COMM_WORLD, &request);
	delete[] dataBuffer;
}

void Broadcast(int tag, int* dataBuffer, int bufferSize)
{
	MPI_Request request;
	for (int i = 0; i < CommunicationSize(); i++) {
		if (i != CommunicationRank()) {
			MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, i, tag, MPI_COMM_WORLD, &request);
		}
	}
	delete[] dataBuffer;
}

void Receive(int source, int tag, int* dataBuffer, int bufferSize)
{

	MPI_Request request;
	MPI_Irecv(dataBuffer, bufferSize, MPI_INTEGER, source, (tag == -1 ? MPI_ANY_TAG : tag), MPI_COMM_WORLD, &request);
}

//-------------SIMULATION EXEC--------------------
// Simulation Executive private variables:
Time SimulationTime;			// simulation time
Time Lookahead;					// simulation lookahead
Time* LastEventTimeSent;		// times of last sent times
unordered_map<unsigned int, NewFunctor> EventClassMap; // mapping of class id to new methods
EventSet InternalQ;				// this process internal Q
EventSet* IncomingQ;			// incoming event queues for all LPs


// Simulation Executive public Methods:
Time GetSimulationTime(){ return SimulationTime; }

void ScheduleEventIn(const Time& deltaT, EventAction* ea, int LP)
{

}

void RunSimulation(Time T)
{
}

void InitializeSimulation()
{
	// initializing communication
	CommunicationInitialize();

	// creating history for all LP except for this LP
	LastEventTimeSent = new Time[CommunicationSize() - 1];

	// creating array of incoming queues for each LP
	IncomingQ = new EventSet[CommunicationSize() - 1];
}

void SetLookahead(Time lookahead) { Lookahead = lookahead; }

void RegisterEventActionClass(unsigned int classId, NewFunctor newFunctor)
{
}
