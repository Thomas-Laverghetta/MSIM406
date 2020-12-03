#include "SimulationExecutive.h"
#include "EventSet.h"
#include "mpi.h"
#include <iostream>
#include <unordered_map>
#include <thread>
#include "Stack.hpp"
#include "Distribution.h"

using namespace std;

//#define DEBUG

// Null msg
class AntiMsg : public EventAction {
	/// Defining EventAction Unique ID and New
	/// Only needed for Events getting sent (Event msgs) to other processors
	UNIQUE_EVENT_ID(0)
	static EventAction* New() { return new AntiMsg; }
public:
	AntiMsg(unsigned int& eventId) { SetEventId(eventId); }
	AntiMsg() {}
	void Execute() {}
	const int GetBufferSize() { return sizeof(GetEventId())/sizeof(int); }
	void Serialize(int* dataBuffer, int& index) { 
		unsigned int eventId = GetEventId();
		EventAction::AddToBuffer(dataBuffer, (int*)&eventId, index, eventId);
	}
	void Deserialize(int* dataBuffer, int& index) { 
		unsigned int eventId;
		EventAction::TakeFromBuffer(dataBuffer, (int*)&eventId, index, eventId);
		SetEventId(eventId);
	}
};

//-------------SIMULATION EXEC--------------------
// Simulation Executive private variables:
Time SimulationTime = 0;		// simulation time
unordered_map<unsigned int, NewFunctor> EventClassMap; // mapping of class id to new methods
EventSet ActiveEventSet;		// active events to execute
bool ROLLBACK = false;			// true when rollback is occuring

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
	unsigned int eventId = ea->GetEventId();
	EventAction::AddToBuffer(dataBuffer, (int*)&eventId, index, eventId);

	MPI_Request request;

	MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, dest, ea->GetEventClassId(), MPI_COMM_WORLD, &request);
	delete[] dataBuffer;
	delete ea;
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

	// deserializing time
	EventAction::TakeFromBuffer(dataBuffer, (int*)&t, index, t);

	// deserializing event action
	ea->Deserialize(dataBuffer, index);
	
	// deserializing and associating eventId
	unsigned int eventId = 0;
	EventAction::TakeFromBuffer(dataBuffer, (int*)&eventId, index, eventId);
	ea->SetEventId(eventId);

	// add to queue
	ActiveEventSet.AddEvent(t, ea);

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
		ActiveEventSet.AddEvent(deltaT + SimulationTime, ea);
	}
	else {
		if (EventClassMap.find(ea->GetEventClassId()) != EventClassMap.end()) {
			Send(LP, deltaT + SimulationTime, ea);
		}
		else {
			printf("Error Sending Undeclared Event. Please Declare Event\a\n");
			exit(0);
		}
	}
}

// scheduling initial events
void InitialScheduleEventIn(Time deltaT, EventAction* ea, int LP) {
	ScheduleEventIn(deltaT, ea, LP);
}


void RunSimulation(Time T)
{
	// recieving variables
	int tag, source;

	Time tempTime;
	while (true) {
		// wait for events
		do {
			while (CheckForComm(tag, source)) { Receive(source, tag); }
		} while (ActiveEventSet.isEmpty());

		SimulationTime = ActiveEventSet.GetEventTime();

		// if simulation time is greater than termination time then terminate
		if (SimulationTime > T) {
			break;
		}
		
#ifdef DEBUG
			cout << "SIM TIME=" << SimulationTime << " : CURR=" << PROCESS_RANK << endl; fflush(stdout);
			this_thread::sleep_for(1s);
#endif

		// get and execute event action
		EventAction* ea = ActiveEventSet.GetEventAction();
		ea->Execute();
		
#ifdef DEBUG
		cout << "NEXT LOOP" << endl; fflush(stdout);
#endif 		
	}
	// finalizing simulation
	CommunicationFinalize();

	// destoying sets
	ActiveEventSet.~EventSet();

	// resetting simulation variables
	SimulationTime = 0;
}

void InitializeSimulation()
{
	this_thread::sleep_for(20s);
	SimulationTime = 0;

	// initializing communication
	CommunicationInitialize();

	// registering null message
	EventClassMap[AntiMsg::_EventClassID] = AntiMsg::New;

	// random seed
	srand(PROCESS_RANK * 3);
	Distribution::SetSeed(PROCESS_RANK * 10);
}

void RegisterEventActionClass(unsigned int classId, NewFunctor newFunctor) { 
	if (classId > 0)
		EventClassMap[classId] = newFunctor; 
	else {
		printf("ERROR: Invalid Event Class Id index. Id must be > 0\a\n");
		exit(0);
	}
}

////// ANTI MSG 
struct AntiMsgStruct {
	Time _t;
	unsigned int _eventId;
	int _LP;
	AntiMsgStruct(unsigned int eventId, int LP, Time t) {
		_eventId = eventId;
		_LP = LP;
		_t = t;
	}
};
unordered_map<unsigned int, Stack<AntiMsgStruct*>> AntiMsg_Map;

// Event Action's schedule event, will be used
void EventAction::ScheduleEventIn(Time deltaT, EventAction* ea, int LP)
{
	// create and push new anti-msg
	AntiMsg_Map[this->_eventId].push(new AntiMsgStruct(this->_eventId, LP, deltaT + SimulationTime));
	
	// schedule w/sim-exec
	ScheduleEventIn(deltaT, ea, LP);
}

void EventAction::SendAntiMsg()
{
	Stack<AntiMsgStruct*>* anti_msg_stack = &AntiMsg_Map[this->_eventId];
	while (!anti_msg_stack->empty()) {
		// create anti msg then schedule anti-msg
		ScheduleEventIn(anti_msg_stack->top()->_t, new AntiMsg(anti_msg_stack->top()->_eventId), anti_msg_stack->top()->_LP);

		// remove from stack
		delete anti_msg_stack->top();
		anti_msg_stack->pop();
	}
}

// removing all anti-msgs
EventAction::~EventAction()
{
	Stack<AntiMsgStruct*> * anti_msg_stack = &AntiMsg_Map[this->_eventId];
	while (!anti_msg_stack->empty()) {
		// remove from stack
		delete anti_msg_stack->top();
		anti_msg_stack->pop();
	}
}
