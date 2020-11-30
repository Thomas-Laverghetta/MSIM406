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
struct ExecutedEvents {
	EventAction* _ea;
	Time _t;
	ExecutedEvents(EventAction* ea, Time t) : _ea(ea), _t(t) {}
	~ExecutedEvents() { delete _ea; }
};


// Simulation Executive private variables:
Time SimulationTime = 0;		// simulation time
unordered_map<unsigned int, NewFunctor> EventClassMap; // mapping of class id to new methods
EventSet ActiveEventSet;		// active events to execute
Stack<ExecutedEvents*> ExecutedSet;// Set of executed Events

// GVT 
Time GVT;
enum MsgColor { Green, Red, Blue };
MsgColor CurrMsgColor = MsgColor::Green;
unsigned int* GreenOutCounter = 0;
unsigned int* RedOutCounter = 0;
unsigned int* BlueOutCounter = 0;
unsigned int GreenRecvCounter = 0;
unsigned int RedRecvCounter = 0;
unsigned int BlueRecvCounter = 0;

unsigned int* GlobalMsgCounter;
unsigned int* GlobalRedCounter;

bool GreenPre = true;

bool RedWait = false;
bool BGWait = false;
bool GVT_init = true;

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
	int bufferSize = ea->GetBufferSize() + (sizeof(t) + sizeof(CurrMsgColor))/ sizeof(int);
	int* dataBuffer = new int[bufferSize];
	int index = 0;

	// Serializing msg color 
	EventAction::AddToBuffer(dataBuffer, (int*)&CurrMsgColor, index, CurrMsgColor);

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

	if (tag != -1) {
		// tag is class Id
		EventAction* ea = EventClassMap[tag]();	// create new event action using tag == classId 
		int bufferSize = ea->GetBufferSize() + (sizeof(Time) + sizeof(MsgColor)) / sizeof(int);
		int* dataBuffer = new int[bufferSize];
		MPI_Request request;

		MPI_Irecv(dataBuffer, bufferSize, MPI_INTEGER, source, tag, MPI_COMM_WORLD, &request);

		// deserialize time and event
		int index = 0;
		
		// deserializing color of msg
		MsgColor msg_color;
		EventAction::TakeFromBuffer(dataBuffer, (int*)&msg_color, index, msg_color);
		switch (msg_color) {
		case MsgColor::Blue:
			BlueRecvCounter++;
			break;
		case MsgColor::Green:
			GreenRecvCounter++;
			break;
		case MsgColor::Red:
			RedRecvCounter++;
			break;
		}

		// deserializing time
		Time t = 0;
		EventAction::TakeFromBuffer(dataBuffer, (int*)&t, index, t);

		// deserializing event
		ea->Deserialize(dataBuffer, index);

		// deserializing event id
		unsigned int eventId = 0;
		EventAction::TakeFromBuffer(dataBuffer, (int*)&eventId, index, eventId);
		ea->SetEventId(eventId);

		// add to queue
		ActiveEventSet.AddEvent(t, ea);

		// waiting for transitant
		if (RedWait && GlobalRedCounter[PROCESS_RANK] == RedRecvCounter) {
			// resetting counters
			RedRecvCounter = 0;
			index = 0;

			// deserialing Red msg send
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::AddToBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
			}

			for (int i = 0; i < NUM_PROCESS; i++) {
				RedOutCounter[i] = 0;

				// number of out msg sent
				if (CurrMsgColor == MsgColor::Blue) {
					GlobalMsgCounter[i] += BlueOutCounter[i];
					BlueOutCounter[i] = 0;
				}
				else {
					GlobalMsgCounter[i] += GreenOutCounter[i];
					GreenOutCounter[i] = 0;
				}

				// Serializing Counter
				EventAction::AddToBuffer(dataBuffer, (int*)&GlobalMsgCounter[i], index, GlobalMsgCounter[i]);
			}

			// chaning to RED region
			CurrMsgColor = MsgColor::Red;

			// sending to next processor
			MPI_Request request;
			MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, -1, MPI_COMM_WORLD, &request);
			RedWait = false;
		}
		// if waiting for transitant blue or green messages, check if new msg was last transitant
		else if (BGWait && (GreenPre ? GlobalMsgCounter[0] == GreenRecvCounter : true) && (!GreenPre ? GlobalMsgCounter[0] == BlueRecvCounter : true)) {
			// Changing from blue to green or green to blue 
			GreenPre = !GreenPre;

			// resetting counters
			GreenRecvCounter = GreenRecvCounter * (GreenPre);
			BlueRecvCounter = BlueRecvCounter * (!GreenPre);

			// Changing color
			CurrMsgColor = (GreenPre ? MsgColor::Green : MsgColor::Blue);

			// Finding Min Time to send
			if (SimulationTime < GVT && SimulationTime < ActiveEventSet.GetEventTime()) {
				GVT = SimulationTime;
			}
			else if (ActiveEventSet.GetEventTime() < GVT && ActiveEventSet.GetEventTime() < SimulationTime) {
				GVT = ActiveEventSet.GetEventTime();
			}

			// Sending GVT
			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS + sizeof(GVT)) / sizeof(int);
			int* dataBuffer2 = new int[bufferSize];
			index = 0;

			// serializing GVT
			EventAction::AddToBuffer(dataBuffer2, (int*)&GVT, index, GVT);


			// serializing msg counters
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::AddToBuffer(dataBuffer2, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
			}

			MPI_Request request;

			MPI_Isend(dataBuffer2, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, -1, MPI_COMM_WORLD, &request);
			delete[] dataBuffer2;
			BGWait = false;
		}
		delete[] dataBuffer;

#ifdef DEBUG
		cout << "RECV MSG : CURR=" << CommunicationRank() << " : FROM=" << source << " : T=" << t << endl; fflush(stdout);
		this_thread::sleep_for(2s);
#endif
	}
	else {
		// GVT init came back around
		if (PROCESS_RANK == 0 && CurrMsgColor == MsgColor::Red) { 
			// deserializing counter array
			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS * 2) / sizeof(int);
			int* dataBuffer = new int[bufferSize];
			MPI_Request request;

			MPI_Irecv(dataBuffer, bufferSize, MPI_INTEGER, source, tag, MPI_COMM_WORLD, &request);

			// deserialize global msg counter
			int index = 0;
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
			}

			// deserialing Red msg send
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
			}

			delete[] dataBuffer; 

			// testing if recv all msg
			if ((GreenPre ? GlobalMsgCounter[0] == GreenRecvCounter : true) && (!GreenPre ? GlobalMsgCounter[0] == BlueRecvCounter : true)) {
				// Changing from blue to green or green to blue 
				GreenPre = !GreenPre;

				// resetting counters
				GreenRecvCounter = GreenRecvCounter * (GreenPre);
				BlueRecvCounter = BlueRecvCounter * (!GreenPre);

				// Changing color
				CurrMsgColor = (GreenPre ? MsgColor::Green : MsgColor::Blue);
				
				// Finding Min Time to send
				if (SimulationTime < ActiveEventSet.GetEventTime()) {
					GVT = SimulationTime;
				}
				else {
					GVT = ActiveEventSet.GetEventTime();
				}

				// Sending GVT
				int bufferSize = (sizeof(GVT)) / sizeof(int);
				int* dataBuffer = new int[bufferSize];
				index = 0;

				// serializing GVT
				EventAction::AddToBuffer(dataBuffer, (int*)&GVT, index, GVT);

				// serializing msg counters
				for (int P = 0; P < NUM_PROCESS; P++) {
					EventAction::AddToBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
				}
				MPI_Request request;

				MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, -1, MPI_COMM_WORLD, &request);
				delete[] dataBuffer;
			}
			else { // waiting for msgs
				BGWait = true;
				GVT = TIME_MAX;
			}
		}

		// Recv GVT init
		else if (GVT_init && (CurrMsgColor == MsgColor::Green || CurrMsgColor == MsgColor::Blue)) { // recv GVT init msg
			// deserializing counter array
			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS * 2) / sizeof(int);
			int* dataBuffer = new int[bufferSize];
			MPI_Request request;

			MPI_Irecv(dataBuffer, bufferSize, MPI_INTEGER, source, tag, MPI_COMM_WORLD, &request);
			
			// deserialize global msg counter
			int index = 0;
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
			}

			// deserialing Red msg send
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
			}

			if (RedRecvCounter == GlobalRedCounter[PROCESS_RANK]) {
				// resetting counters
				RedRecvCounter = 0;
				index = 0;

				for (int i = 0; i < NUM_PROCESS; i++) {
					RedOutCounter[i] = 0;
					
					// number of out msg sent
					if (CurrMsgColor == MsgColor::Blue) {
						GlobalMsgCounter[i] += BlueOutCounter[i];
						BlueOutCounter[i] = 0;
					}
					else {
						GlobalMsgCounter[i] += GreenOutCounter[i];
						GreenOutCounter[i] = 0;
					}

					// Serializing Counter
					EventAction::AddToBuffer(dataBuffer, (int*)&GlobalMsgCounter[i], index, GlobalMsgCounter[i]);
				}

				// chaning to RED region
				CurrMsgColor = MsgColor::Red;

				// sending to next processor
				MPI_Request request;
				MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, -1, MPI_COMM_WORLD, &request);
			}
			else { // red msgs
				RedWait = true;
			}
			delete[] dataBuffer;
			GVT_init = false;
		}

		// recv GVT calculation
		else if (CurrMsgColor == MsgColor::Red) { // Calculate GVT
			// Sending GVT
			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS + sizeof(Time)) / sizeof(int);
			int* dataBuffer = new int[bufferSize];
			int index = 0;

			// Deserializing GVT
			EventAction::TakeFromBuffer(dataBuffer, (int*)&GVT, index, GVT);

			// Deserializing msg counters
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
			}
			
			if ((GreenPre ? GlobalMsgCounter[0] == GreenRecvCounter : true) && (!GreenPre ? GlobalMsgCounter[0] == BlueRecvCounter : true)) {
				// Changing from blue to green or green to blue 
				GreenPre = !GreenPre;

				// resetting counters
				GreenRecvCounter = GreenRecvCounter * (GreenPre);
				BlueRecvCounter = BlueRecvCounter * (!GreenPre);

				// Changing color
				CurrMsgColor = (GreenPre ? MsgColor::Green : MsgColor::Blue);

				if (SimulationTime < GVT && SimulationTime < ActiveEventSet.GetEventTime()) {
					GVT = SimulationTime;
				}
				else if (ActiveEventSet.GetEventTime() < GVT && ActiveEventSet.GetEventTime() < SimulationTime) {
					GVT = ActiveEventSet.GetEventTime();
				}

				index = 0;
				// serializing GVT
				EventAction::AddToBuffer(dataBuffer, (int*)&GVT, index, GVT);

				// serializing msg counters
				for (int P = 0; P < NUM_PROCESS; P++) {
					EventAction::AddToBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
				}

				// Sending to next Processor
				MPI_Request request;

				MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, -1, MPI_COMM_WORLD, &request);
			}
			else {
				BGWait = true;
			}

			delete[] dataBuffer;
		}

		// recv GVT and sending red counter
		else {
			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS + sizeof(Time)) / sizeof(int);
			int* dataBuffer = new int[bufferSize];
			int index = 0;

			// Deserializing GVT
			EventAction::TakeFromBuffer(dataBuffer, (int*)&GVT, index, GVT);

			// Deserializing msg counters
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
			}

			delete dataBuffer;

			// sending official GVT and red counter
			bufferSize = (sizeof(unsigned int) * NUM_PROCESS + sizeof(Time)) / sizeof(int);
			dataBuffer = new int[bufferSize];
			index = 0;

			// Serializing Red msg counter
			for (int P = 0; P < NUM_PROCESS; P++) {
				if (PROCESS_RANK == 0) {
					GlobalRedCounter[P] = 0;
					GlobalRedCounter[P] = RedOutCounter[P];
				}
				else {
					GlobalRedCounter[P] += RedOutCounter[P];
				}
				EventAction::AddToBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
			}

			// Serializing GVT
			EventAction::AddToBuffer(dataBuffer, (int*)&GVT, index, GVT);

			MPI_Request request;

			MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, -1, MPI_COMM_WORLD, &request);
			delete[] dataBuffer;
			GVT_init = true;


			// removing old events
			while (!ExecutedSet.empty() && ExecutedSet.bottom()->_t <= GVT) {
				// delete event then pop bottom
				delete ExecutedSet.bottom();// deleting will cause any anti-msg to be sent
				ExecutedSet.popLeft();
			}
		}
	}
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

// 
void RunSimulation(Time T)
{
	// recieving variables
	int tag, source;

	Time tempTime;

	unsigned int eventsExeuted = 0;
	while (true) {
		// wait for events
		do {
			while (CheckForComm(tag, source)) { Receive(source, tag); }
		} while (ActiveEventSet.isEmpty());

		tempTime = ActiveEventSet.GetEventTime();

		// if simulation time is greater than termination time then terminate
		if (tempTime > T) {
			break;
		}
		// testing for rollback
		else if (tempTime < SimulationTime) {
			ROLLBACK = true;
			while (!ExecutedSet.empty() && ExecutedSet.top()->_t < tempTime) {
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
		EventAction* ea = ActiveEventSet.GetEventAction();
		ea->Execute();
		ExecutedSet.push(new ExecutedEvents(ea, SimulationTime));

		// every 4-events executed, run GVT
		if (eventsExeuted == 5 && RedRecvCounter == GlobalRedCounter[PROCESS_RANK]) {
			RedRecvCounter = 0;
			eventsExeuted = 0;

			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS * 2) / sizeof(int);
			int* dataBuffer = new int[bufferSize];
			// deserialize global msg counter
			int index = 0;
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
			}

			// deserialing Red msg send
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
			}

			// chaning to RED region
			CurrMsgColor = MsgColor::Red;

			// sending to next processor
			MPI_Request request;
			MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, -1, MPI_COMM_WORLD, &request);
		}
		
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

	GlobalMsgCounter = new unsigned[NUM_PROCESS];
	GlobalRedCounter = new unsigned[NUM_PROCESS];

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

EventAction::~EventAction()
{
	Stack<AntiMsgStruct*> * anti_msg_stack = &AntiMsg_Map[this->_eventId];
	if (ROLLBACK) {
		while (!anti_msg_stack->empty()) {
			// determine if simultanious event
			if (anti_msg_stack->top()->_LP == PROCESS_RANK && anti_msg_stack->top()->_t == SimulationTime) {
				// event set will determine if simultaneous event
				ActiveEventSet.isAntiMsgSimultaneous(anti_msg_stack->top()->_t, anti_msg_stack->top()->_eventId);
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
