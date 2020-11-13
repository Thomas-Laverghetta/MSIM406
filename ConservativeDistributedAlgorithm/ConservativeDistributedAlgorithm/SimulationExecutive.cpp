#include "SimulationExecutive.h"
#include "EventSet.hpp"
#include "OutputEventSet.hpp"
#include "mpi.h"
#include <iostream>
#include <unordered_map>
#include <thread>



using namespace std;

// init global Event action id
int EventAction::GlobalClassId = 0;

// Null msg
class NULL_MSG : public EventAction {
public:
	NULL_MSG() {}
	void Execute() {}
	const int GetBufferSize() { return 0; }
	void Serialize(int* dataBuffer, int& index) {}
	void Deserialize(int* dataBuffer, int& index) {}

	static int _classId;

	int GetClassId() {
		return _classId;
	}

	static EventAction* New() { return new NULL_MSG; }
};
int NULL_MSG::_classId = EventAction::GlobalClassId++;

//-------------SIMULATION EXEC--------------------
// Simulation Executive private variables:
Time SimulationTime = 0;		// simulation time
Time Lookahead = 0;				// simulation lookahead
Time* LastEventTimeSent;		// times of last sent times
unordered_map<unsigned int, NewFunctor> EventClassMap; // mapping of class id to new methods
EventSet InternalQ;				// this process internal Q
EventSet* IncomingQ;			// incoming event queues for all LPs
EventSet ExecutionSet;			// Set for executing events
OutEventSet outputQ;			// events to be sent out

//----------------Comm-------------------
int PROCESS_RANK = -1;
int NUM_PROCESS = -1;

void CommunicationInitialize()
{
	int* argc = 0;
	char*** argv = 0;
	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &PROCESS_RANK);
	MPI_Comm_size(MPI_COMM_WORLD, &NUM_PROCESS);
}

void CommunicationFinalize()
{
	cout << PROCESS_RANK << " in finalize" << endl;
	MPI_Finalize();
	cout << PROCESS_RANK << " done finalize" << endl;
}

int CommunicationRank() { return PROCESS_RANK; }

int CommunicationSize(){ return NUM_PROCESS; }

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

void Send(int dest, const Time& t, EventAction * ea)
{
	int bufferSize = ea->GetBufferSize() + sizeof(t)/sizeof(int);
	int* dataBuffer = new int[bufferSize];
	int index = 0;

	// serialize time and event
	EventAction::AddToBuffer(dataBuffer, (int*)&t, index , t);
	ea->Serialize(dataBuffer, index);

	MPI_Request request;
	MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, dest, ea->GetClassId(), MPI_COMM_WORLD, &request);
	delete[] dataBuffer;
	delete ea;

	LastEventTimeSent[(dest >= PROCESS_RANK ? dest - 1 : dest)] = t;
}

void Broadcast(Time t, EventAction * ea)
{
	int bufferSize = ea->GetBufferSize() + sizeof(t) / sizeof(int);
	int* dataBuffer = new int[bufferSize];
	int index = 0;

	EventAction::AddToBuffer(dataBuffer, (int*)&t, index, t);
	ea->Serialize(dataBuffer, index);

	MPI_Request request;
	for (int i = 0; i < CommunicationSize(); i++) {
		if (i != CommunicationRank()) {
			MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, i, ea->GetClassId(), MPI_COMM_WORLD, &request);
			LastEventTimeSent[(i >= CommunicationRank() ? i - 1 : i)] = t;
		}
	}
	delete ea;
	delete[] dataBuffer;
}

void Receive(int source, int tag)
{
	// tag is class Id
	EventAction* ea = EventClassMap[tag]();	// create new event action using tag == classId 
	int bufferSize = ea->GetBufferSize() + sizeof(Time) / sizeof(int);
	int * dataBuffer = new int[bufferSize];
	MPI_Request request;

	MPI_Irecv(dataBuffer, bufferSize, MPI_INTEGER, source, tag, MPI_COMM_WORLD, &request);

	// deserialize time and event
	int index = 0;
	Time t = 0;
	EventAction::TakeFromBuffer(dataBuffer, (int*)&t, index, t);
	ea->Deserialize(dataBuffer, index);

	// add to queue
	IncomingQ[(source >= CommunicationRank() ? source - 1 : source)].AddEvent(t, ea);

	delete[] dataBuffer;

#ifdef DEBUG
	cout << "MSG recv d : " << source << " : " << t << endl; fflush(stdout);
	this_thread::sleep_for(2s);
#endif
}

// Simulation Executive public Methods:
Time GetSimulationTime(){ return SimulationTime; }

void ScheduleEventIn(const Time& deltaT, EventAction* ea, int LP)
{
	if (LP == CommunicationRank()) {
		InternalQ.AddEvent(deltaT + SimulationTime, ea);
	}
	else {
		outputQ.AddEvent(deltaT + SimulationTime, ea, LP);
	}
}

// returns true if any incoming Q is empty
bool IncomingQueuesEmpty() {
	int numEmpty = 0;
	for (int j = 0; j < NUM_PROCESS - 1; j++) {
		numEmpty += IncomingQ[j].isEmpty();
	}
	return numEmpty > 0;
}

void RunSimulation(Time T)
{
	// send null msg to all LPs
	Broadcast(Lookahead, new NULL_MSG); 

	// setting initial message send time
	for (int i = 0; i < NUM_PROCESS - 1; i++) { LastEventTimeSent[i] = Lookahead; }

	bool LOOP = true;
	while (LOOP) {
		// while a incomingQ is empty, check comms and add any new events to incomingQs 
		while (IncomingQueuesEmpty()) {
			int source, tag;
			while (!(CheckForComm(tag, source)));
			Receive(source, tag);	// will deserialize and add event to queue
		}
		
		// finding safe time
		Time safe = TIME_MAX;
		for (int i = 0; i < NUM_PROCESS - 1; i++) {
			safe = IncomingQ[i].GetEventTime() * (IncomingQ[i].GetEventTime() < safe) + safe * (IncomingQ[i].GetEventTime() >= safe);
		}

#ifdef DEBUG
		cout << "Safe : " << safe << endl; fflush(stdout);
		this_thread::sleep_for(3s);
#endif
		// getting all executable events and adding them to execution set
		while (!InternalQ.isEmpty() && InternalQ.GetEventTime() <= safe) {
			ExecutionSet.AddEvent(InternalQ.GetEventTime(), InternalQ.GetEventAction());
		}
		for (int i = 0; i < NUM_PROCESS - 1; i++) {
			while (!IncomingQ[i].isEmpty() && IncomingQ[i].GetEventTime() <= safe) {
				ExecutionSet.AddEvent(IncomingQ[i].GetEventTime(), IncomingQ[i].GetEventAction());
			}
		}

		// executing events
		while (!ExecutionSet.isEmpty()) {
			SimulationTime = ExecutionSet.GetEventTime();

			// if simulation time is greater than termination time then terminate
			if (SimulationTime > T) {
				LOOP = false;		// signals while loop to stop
				break;
			}

			EventAction * ea = ExecutionSet.GetEventAction();
			ea->Execute();
			delete ea;

			cout << "SimTime : " << SimulationTime << endl; fflush(stdout);
			this_thread::sleep_for(1s);

			// sending output queues
			while (!outputQ.isEmpty() && outputQ.GetEventTime() <= SimulationTime + Lookahead) {
				// will serialize and send to LP
				Send(outputQ.GetLP(), outputQ.GetEventTime(), outputQ.GetEventAction());
			}

			// sending null msgs
			for (int i = 0; i < NUM_PROCESS - 1; i++) {
				if (LastEventTimeSent[i] < SimulationTime + Lookahead) {
					Send((i >= CommunicationRank() ? i + 1 : i), SimulationTime + Lookahead, new NULL_MSG);
				}
			}
#ifdef DEBUG
			cout << " Sent Null msgs "<< endl; fflush(stdout);
			this_thread::sleep_for(2s);
#endif
		}
#ifdef DEBUG
		cout << "NEXT LOOP" << endl; fflush(stdout);
		this_thread::sleep_for(2s);
#endif 		
	}
	// finalizing simulation
	CommunicationFinalize();

	// mem management
	delete[] IncomingQ;
	delete[] LastEventTimeSent;

	// destoying sets
	InternalQ.~EventSet();
	ExecutionSet.~EventSet();
	outputQ.~OutEventSet();

	// resetting simulation variables
	SimulationTime = 0;
}

void InitializeSimulation()
{
	// initializing communication
	CommunicationInitialize();

	// creating history for all LP except for this LP
	LastEventTimeSent = new Time[NUM_PROCESS - 1];

	// creating array of incoming queues for each LP
	IncomingQ = new EventSet[NUM_PROCESS - 1];

	// registering null message
	RegisterEventActionClass(NULL_MSG::_classId, NULL_MSG::New);
}

void SetSimulationLookahead(Time lookahead) { Lookahead = lookahead; }

void RegisterEventActionClass(unsigned int classId, NewFunctor newFunctor) { EventClassMap[classId] = newFunctor; }
