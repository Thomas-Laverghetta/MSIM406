#include "SimulationExecutive.h"
#include "EventSet.h"
#include "mpi.h"
#include <iostream>
#include <unordered_map>
#include <thread>
#include <stack>
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
	AntiMsg(unsigned int& eventId) { _eventId = eventId; }
	AntiMsg() {}
	void Execute() {}
	const int GetBufferSize() { return sizeof(_eventId)/sizeof(int); }
	void Serialize(int* dataBuffer, int& index) { EventAction::AddToBuffer(dataBuffer, (int*)&_eventId, index, _eventId); }
	void Deserialize(int* dataBuffer, int& index) { EventAction::TakeFromBuffer(dataBuffer, (int*)&_eventId, index, _eventId); }
private:
	unsigned int _eventId;
};

//-------------SIMULATION EXEC--------------------
struct ExecutedEvents {
	EventAction* _ea;
	Time _t;
	ExecutedEvents(EventAction* ea, Time t) : _ea(ea), _t(t) {}
	~ExecutedEvents() { delete _ea; }
};

// Simulation Executive private variables:
Time SimulationTime = 0;		// simulation time
unordered_map<unsigned int, NewFunctor> EventClassMap; // mapping of class id to new methods
EventSet InternalEventSet;		// this process internal Q
stack<ExecutedEvents*> ExecutedSet;// Set of executed Events
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
	EventAction::TakeFromBuffer(dataBuffer, (int*)&t, index, t);
	ea->Deserialize(dataBuffer, index);
	unsigned int eventId = 0;
	EventAction::TakeFromBuffer(dataBuffer, (int*)&eventId, index, eventId);
	ea->SetEventId(eventId);

	// add to queue
	InternalEventSet.AddEvent(t, ea);

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
		InternalEventSet.AddEvent(deltaT + SimulationTime, ea);
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
		} while (InternalEventSet.isEmpty());

		tempTime = InternalEventSet.GetEventTime();

		// if simulation time is greater than termination time then terminate
		if (tempTime > T) {
			break;
		}
		// testing for rollback
		else if (tempTime < SimulationTime) {
			ROLLBACK = true;
			while (ExecutedSet.top()->_t < tempTime) {
				// delete event then pop top
				delete ExecutedSet.top();	// deleting will cause any anti-msg to be sent
				ExecutedSet.pop();
			}
			ROLLBACK = false;
		}
		SimulationTime = tempTime;
		

#ifdef DEBUG
			cout << "SIM TIME=" << SimulationTime << " : CURR=" << PROCESS_RANK << endl; fflush(stdout);
			this_thread::sleep_for(1s);
#endif

		// get and execute event action
		EventAction* ea = InternalEventSet.GetEventAction();
		ea->Execute();
		ExecutedSet.push(new ExecutedEvents(ea, SimulationTime));
		
#ifdef DEBUG
		cout << "NEXT LOOP" << endl; fflush(stdout);
#endif 		
	}
	// finalizing simulation
	CommunicationFinalize();

	// destoying sets
	InternalEventSet.~EventSet();

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
unordered_map<unsigned int, stack<AntiMsgStruct*>> AntiMsg_Map;

// Event Action's schedule event, will be used
void EventAction::ScheduleEventIn(Time deltaT, EventAction* ea, int LP)
{
	// create and push new anti-msg
	AntiMsg_Map[this->_eventId].push(new AntiMsgStruct(this->_eventId, LP, deltaT));
	
	// schedule w/sim-exec
	ScheduleEventIn(deltaT, ea, LP);
}

EventAction::~EventAction()
{
	stack<AntiMsgStruct*> * anti_msg_stack = &AntiMsg_Map[this->_eventId];
	if (ROLLBACK) {
		while (!anti_msg_stack->empty()) {
			// determine if simultanious event
			if (anti_msg_stack->top()->_LP == PROCESS_RANK && anti_msg_stack->top()->_t == SimulationTime) {
				// event set will determine if simultaneous event
				InternalEventSet.isAntiMsgSimultaneous(anti_msg_stack->top()->_t, anti_msg_stack->top()->_eventId);
			}
			else if (!(anti_msg_stack->top()->_LP == PROCESS_RANK && anti_msg_stack->top()->_t < SimulationTime))
			{ // send anti-msgs for scheduled events (ignore executed on this process)
				// create anti msg then schedule anti-msg
				ScheduleEventIn(anti_msg_stack->top()->_t, new AntiMsg(anti_msg_stack->top()->_eventId), anti_msg_stack->top()->_LP);
			}

			// remove from stack
			delete anti_msg_stack->top();
			anti_msg_stack->pop();
		}
	}
	else { // remove all anti-msgs (this is used when GVT determines it is safe to delete executed events)
		while (!anti_msg_stack->empty()) {
			// remove from stack
			delete anti_msg_stack->top();
			anti_msg_stack->pop();
		}
	}
}
