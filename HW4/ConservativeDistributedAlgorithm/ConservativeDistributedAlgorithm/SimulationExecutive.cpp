#include "SimulationExecutive.h"
#include "OutputEventSet.h"
#include "EventSet.h"
#include "mpi.h"
#include <iostream>
#include <unordered_map>
#include <thread>
#include "Distribution.h"

using namespace std;

//#define DEBUG

// Null msg
class NULL_MSG : public EventAction {
	/// Defining EventAction Unique ID and New
	/// Only needed for Events getting sent (Event msgs) to other processors
	UNIQUE_EVENT_ID(0)
	static EventAction* New() { return new NULL_MSG; }
public:
	NULL_MSG() {}
	void Execute() {}
	const int GetBufferSize() { return 0; }
	void Serialize(int* dataBuffer, int& index) {}
	void Deserialize(int* dataBuffer, int& index) {}
};

//-------------SIMULATION EXEC--------------------
// Simulation Executive private variables:
Time SimulationTime = 0;		// simulation time
Time Lookahead = 0;				// simulation lookahead
Time* LastEventTimeSent;		// times of last sent times
unordered_map<unsigned int, NewFunctor> EventClassMap; // mapping of class id to new methods
EventSet InternalQ;				// this process internal Q
EventSet* IncomingQ;			// incoming event queues for all LPs
EventSet ExecutionSet;			// Set for executing events
OutputEventSet outputQ;			// events to be sent out

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

int CommunicationSize() { return NUM_PROCESS; }

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

void Send(int dest, const Time& t, EventAction* ea)
{
#ifdef DEBUG
	cout << "SEND MSG : CURR=" << CommunicationRank() << " : TO=" << dest << " : EVENT=" << ea->GetClassId() << " : T=" << t << endl; fflush(stdout);
	this_thread::sleep_for(2s);
#endif
	int bufferSize = ea->GetBufferSize() + sizeof(t) / sizeof(int);
	int* dataBuffer = new int[bufferSize];
	int index = 0;

	// serialize time and event
	EventAction::AddToBuffer(dataBuffer, (int*)&t, index, t);
	ea->Serialize(dataBuffer, index);

	MPI_Request request;

	MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, dest, ea->GetEventClassId(), MPI_COMM_WORLD, &request);
	delete[] dataBuffer;
	delete ea;

	LastEventTimeSent[(dest >= PROCESS_RANK ? dest - 1 : dest)] = t;
}

void Receive(int source, int tag)
{
	// tag is class Id
	EventAction* ea = EventClassMap[tag]();	// create new event action using tag == classId 
	int bufferSize = ea->GetBufferSize() + sizeof(Time) / sizeof(int);
	int* dataBuffer = new int[bufferSize];
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
	cout << "RECV MSG : CURR=" << CommunicationRank() << " : FROM=" << source << " : T=" << t << endl; fflush(stdout);
	this_thread::sleep_for(2s);
#endif
}

// Simulation Executive public Methods:
Time GetSimulationTime() { return SimulationTime; }

void ScheduleEventIn(Time deltaT, EventAction* ea, int LP)
{
	if (LP == CommunicationRank()) {
		InternalQ.AddEvent(deltaT + SimulationTime, ea);
	}
	else {
		if (EventClassMap.find(ea->GetEventClassId()) != EventClassMap.end()) {
			outputQ.AddEvent(deltaT + SimulationTime, ea, LP);
		}
		else {
			
		}
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
	// recieving variables
	int tag, source;

	// sending output queues
	while (!outputQ.isEmpty() && outputQ.GetEventTime() <= Lookahead) {
		// will serialize and send to LP
		Send(outputQ.GetLP(), outputQ.GetEventTime(), outputQ.GetEventAction());
	}

	// sending null msgs
	for (int i = 0; i < NUM_PROCESS - 1; i++) {
		if (LastEventTimeSent[i] != Lookahead) {
			Send((i >= CommunicationRank() ? i + 1 : i), SimulationTime + Lookahead, new NULL_MSG);
		}
	}

	bool LOOP = true;
	while (LOOP) {
		// while a incomingQ is empty, check comms and add any new events to incomingQs 
		while (IncomingQueuesEmpty()) {
			// wait for new message
			while (!(CheckForComm(tag, source)));
			Receive(source, tag);	// will deserialize and add event to queues
		}
		while (CheckForComm(tag, source)) {
			Receive(source, tag);	// will deserialize and add event to queues
		}

		// finding safe time
		Time safe = TIME_MAX;
		for (int i = 0; i < NUM_PROCESS - 1; i++) {
			if (IncomingQ[i].GetEventTime() < safe) safe = IncomingQ[i].GetEventTime();
		}

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

#ifdef DEBUG
			cout << "SIM TIME=" << SimulationTime << " : CURR=" << PROCESS_RANK << endl; fflush(stdout);
			this_thread::sleep_for(1s);
#endif

			// get and execute event action
			EventAction* ea = ExecutionSet.GetEventAction();
			ea->Execute();
			delete ea;

			// sending output queues
			while (!outputQ.isEmpty() && outputQ.GetEventTime() <= (SimulationTime + Lookahead)) {
				// will serialize and send to LP
				int LP = outputQ.GetLP();
				Time time = outputQ.GetEventTime();
				EventAction * ea = outputQ.GetEventAction();
#ifdef DEBUG
				cout << "NON-NULL MSG SENDING: CURR=" << PROCESS_RANK << " : LP=" << LP << " : TIME=" << time << " : EA=" << ea->GetClassId() << endl; fflush(stdout);
#endif

				Send(LP, time, ea);
			}

			// sending null msgs
			for (int i = 0; i < NUM_PROCESS - 1; i++) {
				if (LastEventTimeSent[i] != (SimulationTime + Lookahead)) {
					Send((i >= CommunicationRank() ? i + 1 : i), SimulationTime + Lookahead, new NULL_MSG);
#ifdef DEBUG
					cout << "NULL MSG SENT : CURR=" << PROCESS_RANK << endl; fflush(stdout);
#endif
				}
			}

			// checking for new events between execution
			while (CheckForComm(tag, source)) { Receive(source, tag); }
		}
#ifdef DEBUG
		cout << "NEXT LOOP" << endl; fflush(stdout);
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
	outputQ.~OutputEventSet();

	// resetting simulation variables
	SimulationTime = 0;
}

void InitializeSimulation()
{
	this_thread::sleep_for(20s);
	SimulationTime = 0;
	Lookahead = 0;

	// initializing communication
	CommunicationInitialize();

	// creating history for all LP except for this LP
	LastEventTimeSent = new Time[NUM_PROCESS - 1];

	// creating array of incoming queues for each LP
	IncomingQ = new EventSet[NUM_PROCESS - 1];

	// registering null message
	RegisterEventActionClass(NULL_MSG::_EventClassID, NULL_MSG::New);

	// random seed
	srand(PROCESS_RANK * 3);
	Distribution::SetSeed(PROCESS_RANK * 10);
}

void SetSimulationLookahead(Time lookahead) { Lookahead = lookahead; }

void RegisterEventActionClass(unsigned int classId, NewFunctor newFunctor) { 
	EventClassMap[classId] = newFunctor; 
}
