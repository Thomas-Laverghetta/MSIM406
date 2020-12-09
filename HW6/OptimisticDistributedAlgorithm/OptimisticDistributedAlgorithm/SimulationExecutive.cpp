#include "SimulationExecutive.h"
#include "EventSet.h"
#include "mpi.h"
#include <iostream>
#include <unordered_map>
#include <thread>
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
		unsigned int eventId = 0;
		EventAction::TakeFromBuffer(dataBuffer, (int*)&eventId, index, eventId);
		SetEventId(eventId);
	}
};

//-------------SIMULATION EXEC--------------------
// Simulation Executive private variables:
Time SimulationTime = 0;		// simulation time
unordered_map<unsigned int, NewFunctor> EventClassMap; // mapping of class id to new methods
EventSet ES;		// active events to execute

// GVT 
#define GVT_TAG -1
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

bool wasGreenPrev = true;
bool RedWait = false;
bool BGWait = false;
bool GVT_init = true;

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
	int bufferSize = ea->GetBufferSize() + (sizeof(t) + sizeof(CurrMsgColor) + sizeof(unsigned int))/ sizeof(int);
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

	if (tag != GVT_TAG) {
		// tag is class Id
		EventAction* ea = EventClassMap[tag]();	// create new event action using tag == classId 
		int bufferSize = ea->GetBufferSize() + (sizeof(Time) + sizeof(MsgColor) + sizeof(unsigned int)) / sizeof(int);
		int* dataBuffer = new int[bufferSize];
		MPI_Request request;

		MPI_Irecv(dataBuffer, bufferSize, MPI_INTEGER, source, tag, MPI_COMM_WORLD, &request);

		// deserialize time and event
		int index = 0;
		
		// deserializing color of msg
		MsgColor msg_color = MsgColor::Blue;
		EventAction::TakeFromBuffer(dataBuffer, (int*)&msg_color, index, msg_color);
		// adding color msg to counter
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
		ES.AddEvent(t, ea);

		// memory management
		delete[] dataBuffer;

		// if waiting for red transitant msg
		if (RedWait && GlobalRedCounter[PROCESS_RANK] == RedRecvCounter) {
			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS) / sizeof(int);
			int* dataBuffer = new int[bufferSize];

			// setting wait to false (do not have to wait anymore)
			RedWait = false;

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
			MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, GVT_TAG, MPI_COMM_WORLD, &request);
			delete[] dataBuffer;
		}
		// if waiting for transitant blue or green messages, check if new msg was last transitant
		else if (BGWait && (wasGreenPrev ? GlobalMsgCounter[PROCESS_RANK] == GreenRecvCounter : true) && (!wasGreenPrev ? GlobalMsgCounter[PROCESS_RANK] == BlueRecvCounter : true)) {
			// not waiting for msgs anymore
			BGWait = false;
			
			// Changing from blue to green or green to blue 
			wasGreenPrev = !wasGreenPrev;

			// resetting counters
			GreenRecvCounter = GreenRecvCounter * (wasGreenPrev);
			BlueRecvCounter = BlueRecvCounter * (!wasGreenPrev);

			// Changing color
			CurrMsgColor = (wasGreenPrev ? MsgColor::Green : MsgColor::Blue);

			// Finding Min Time to send
			if (SimulationTime < GVT && SimulationTime < ES.GetEventTime()) {
				GVT = SimulationTime;
			}
			else if (ES.GetEventTime() < GVT && ES.GetEventTime() < SimulationTime) {
				GVT = ES.GetEventTime();
			}

			// Last process to calculate GVT, therefore, only need to send GVT and red counter
			if (PROCESS_RANK == NUM_PROCESS - 1) {
				// resetting msg size
				delete[] dataBuffer;
				int bufferSize = (sizeof(Time) + sizeof(unsigned int) * NUM_PROCESS) / sizeof(int);
				int* dataBuffer = new int[bufferSize];

				// serializing GVT
				EventAction::AddToBuffer(dataBuffer, (int*)&GVT, index, GVT);

				// serializing Red msg sent
				for (int P = 0; P < NUM_PROCESS; P++) {
					GlobalRedCounter[P] += RedOutCounter[P];

					// resetting counter
					RedOutCounter[P] = 0;

					EventAction::AddToBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
				}
			}
			else {
				// Sending GVT
				int bufferSize = (sizeof(unsigned int) * NUM_PROCESS * 2 + sizeof(GVT)) / sizeof(int);
				int* dataBuffer = new int[bufferSize];
				index = 0;

				// serializing GVT
				EventAction::AddToBuffer(dataBuffer, (int*)&GVT, index, GVT);


				// serializing msg counters
				for (int P = 0; P < NUM_PROCESS; P++) {
					EventAction::AddToBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
				}

				// serializing Red msg sent
				for (int P = 0; P < NUM_PROCESS; P++) {
					// if process 0, set to your red out counter
					if (PROCESS_RANK == 0) {
						GlobalRedCounter[P] = RedOutCounter[P];
					}
					else { // else, add too red counter 
						GlobalRedCounter[P] += RedOutCounter[P];
					}

					// resetting counter
					RedOutCounter[P] = 0;

					EventAction::AddToBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
				}
			}
			MPI_Request request;

			MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, GVT_TAG, MPI_COMM_WORLD, &request);
			delete[] dataBuffer;
		}

#ifdef DEBUG
		cout << "RECV MSG : CURR=" << CommunicationRank() << " : FROM=" << source << " : T=" << t << endl; fflush(stdout);
		this_thread::sleep_for(2s);
#endif
	}
	else {
		// GVT init came back around
		if (PROCESS_RANK == 0 && CurrMsgColor == MsgColor::Red) { 
			// deserializing counter array
			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS) / sizeof(int);
			int* dataBuffer = new int[bufferSize];
			MPI_Request request;

			MPI_Irecv(dataBuffer, bufferSize, MPI_INTEGER, source, tag, MPI_COMM_WORLD, &request);

			// deserialize global msg counter
			int index = 0;
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
			}

			delete[] dataBuffer; 

			// testing if recv all msg from green/blue (previous 
			if ((wasGreenPrev ? GlobalMsgCounter[0] == GreenRecvCounter : true) && (!wasGreenPrev ? GlobalMsgCounter[0] == BlueRecvCounter : true)) {
				// Changing from blue to green or green to blue 
				wasGreenPrev = !wasGreenPrev;

				// resetting counters
				GreenRecvCounter = GreenRecvCounter * (wasGreenPrev);
				BlueRecvCounter = BlueRecvCounter * (!wasGreenPrev);

				// Changing color (blue to green or green to blue)
				CurrMsgColor = (wasGreenPrev ? MsgColor::Green : MsgColor::Blue);
				
				// Finding Min Time to send
				if (SimulationTime < ES.GetEventTime()) {
					GVT = SimulationTime;
				}
				else {
					GVT = ES.GetEventTime();
				}

				// Sending GVT
				int bufferSize = (sizeof(GVT) + sizeof(unsigned int) * NUM_PROCESS * 2) / sizeof(int);
				int* dataBuffer = new int[bufferSize];
				index = 0;

				// serializing GVT
				EventAction::AddToBuffer(dataBuffer, (int*)&GVT, index, GVT);

				// serializing msg counters
				for (int P = 0; P < NUM_PROCESS; P++) {
					EventAction::AddToBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
				}

				// serializing Red msg sent
				for (int P = 0; P < NUM_PROCESS; P++) {
					// setting to process 0's red
					GlobalRedCounter[P] = RedOutCounter[P];
					RedOutCounter[P] = 0;

					EventAction::AddToBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
				}

				MPI_Request request;

				MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, -1, MPI_COMM_WORLD, &request);
				delete[] dataBuffer;
			}
			else { // all Green/Blue msgs haven't been recv'd, wait until msg has been recv'd
				BGWait = true;
				GVT = TIME_MAX;
			}
		}

		// Recv GVT init
		else if (GVT_init && (CurrMsgColor == MsgColor::Green || CurrMsgColor == MsgColor::Blue)) { // recv GVT init msg
			// deserializing counter array
			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS) / sizeof(int);
			int* dataBuffer = new int[bufferSize];
			MPI_Request request;

			MPI_Irecv(dataBuffer, bufferSize, MPI_INTEGER, source, tag, MPI_COMM_WORLD, &request);
			
			// deserialize global msg counter (blue or green counter)
			int index = 0;
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
			}

			// if we have recv all red msgs, then transition to red msg domain
			if (RedRecvCounter == GlobalRedCounter[PROCESS_RANK]) {
				// resetting counters
				RedRecvCounter = 0;
				index = 0;

				for (int i = 0; i < NUM_PROCESS; i++) {
					RedOutCounter[i] = 0;
					
					// adding its number of msgs sent to total number of msg sent
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
				MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, GVT_TAG, MPI_COMM_WORLD, &request);
			}
			// not all red msgs have been recv, continue processing in green/blue domain until all msgs have been recv'd
			else { 
				// red msgs
				RedWait = true;
			}
			delete[] dataBuffer;

			// 
			GVT_init = false;
		}

		// recv GVT calculation
		else if (CurrMsgColor == MsgColor::Red) { // Calculate GVT
			// Sending GVT
			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS*2 + sizeof(Time)) / sizeof(int);
			int* dataBuffer = new int[bufferSize];
			int index = 0;

			// Deserializing GVT
			EventAction::TakeFromBuffer(dataBuffer, (int*)&GVT, index, GVT);

			// Deserializing msg counters
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
			}

			// deserializing Red msg sent
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
			}
			
			if ((wasGreenPrev ? GlobalMsgCounter[0] == GreenRecvCounter : true) && (!wasGreenPrev ? GlobalMsgCounter[0] == BlueRecvCounter : true)) {
				// Changing from blue to green or green to blue 
				wasGreenPrev = !wasGreenPrev;

				// resetting counters
				GreenRecvCounter = GreenRecvCounter * (wasGreenPrev);
				BlueRecvCounter = BlueRecvCounter * (!wasGreenPrev);

				// Changing color
				CurrMsgColor = (wasGreenPrev ? MsgColor::Green : MsgColor::Blue);

				// finding min time between simulation time, ES time, and GVT time
				if (SimulationTime < GVT && SimulationTime < ES.GetEventTime()) {
					GVT = SimulationTime;
				}
				else if (ES.GetEventTime() < GVT && ES.GetEventTime() < SimulationTime) {
					GVT = ES.GetEventTime();
				}
				
				// Last process to calculate GVT, therefore, only send red counter and GVT
				if (PROCESS_RANK == NUM_PROCESS - 1) {
					// resetting msg size
					delete[] dataBuffer;
					int bufferSize = (sizeof(Time) + sizeof(unsigned int) * NUM_PROCESS) / sizeof(int);
					int* dataBuffer = new int[bufferSize];

					EventAction::AddToBuffer(dataBuffer, (int*)&GVT, index, GVT);

					// serializing Red msg sent
					for (int P = 0; P < NUM_PROCESS; P++) {
						GlobalRedCounter[P] += RedOutCounter[P];

						// resetting counter
						RedOutCounter[P] = 0;

						EventAction::AddToBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
					}
				}
				else {
					// serializing GVT
					index = 0;
					EventAction::AddToBuffer(dataBuffer, (int*)&GVT, index, GVT);

					// serializing msg counters
					for (int P = 0; P < NUM_PROCESS; P++) {
						EventAction::AddToBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
					}

					// serializing Red msg sent
					for (int P = 0; P < NUM_PROCESS; P++) {
						GlobalRedCounter[P] += RedOutCounter[P];
						
						// resetting counter
						RedOutCounter[P] = 0;

						EventAction::AddToBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
					}
				}
				// Sending to next Processor
				MPI_Request request;

				MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, GVT_TAG, MPI_COMM_WORLD, &request);
			}
			else {// all Green/Blue have not been recv'd yet, continue processing until recv'd
				BGWait = true;
			}

			delete[] dataBuffer;
		}

		// recv GVT and sending red counter
		else {
			int bufferSize = (sizeof(Time) + sizeof(unsigned int) * NUM_PROCESS) / sizeof(int);
			int* dataBuffer = new int[bufferSize];
			int index = 0;

			// Deserializing GVT
			EventAction::TakeFromBuffer(dataBuffer, (int*)&GVT, index, GVT);

			// deserializing Red msg sent
			for (int P = 0; P < NUM_PROCESS; P++) {
				EventAction::TakeFromBuffer(dataBuffer, (int*)&GlobalRedCounter[P], index, GlobalRedCounter[P]);
			}

			// send GVT and red msg counter
			MPI_Request request;

			MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, -1, MPI_COMM_WORLD, &request);
			delete[] dataBuffer;

			// set waiting for GVT again
			GVT_init = true;

			// removing old event from ES
			ES.GVT_removal(GVT);
		}
	}
}

// Simulation Executive public Methods:
Time GetSimulationTime() { return SimulationTime; }

void ScheduleEvent(Time time, EventAction* ea, int LP)
{
	if (LP == CommunicationRank()) {
		ES.AddEvent(time, ea);
	}
	else {
		if (EventClassMap.find(ea->GetEventClassId()) != EventClassMap.end()) {
			Send(LP, time, ea);
		}
		else {
			printf("Error Sending Undeclared Event. Please Declare Event\a\n");
			exit(0);
		}
	}
}

// scheduling initial events
void InitialScheduleEventIn(Time deltaT, EventAction* ea, int LP) {
	ScheduleEvent(deltaT + SimulationTime, ea, LP);
}

// 
void RunSimulation(Time T)
{
	// recieving variables
	int tag, source;

	unsigned int eventsExeuted = 0;
	while (true) {
		// wait for events
		do {
			while (CheckForComm(tag, source)) { Receive(source, tag); }
		} while (ES.isEmpty());

		SimulationTime = ES.GetEventTime();

		// if simulation time is greater than termination time then terminate
		if (SimulationTime > T) {
			break;
		}

#ifdef DEBUG
			cout << "SIM TIME=" << SimulationTime << " : CURR=" << PROCESS_RANK << endl; fflush(stdout);
			this_thread::sleep_for(1s);
#endif

		// get and execute event action
		EventAction* ea = ES.GetEventAction();
		ea->Execute();

		// every 4-events executed, run GVT
		if (eventsExeuted == 5 && RedRecvCounter == GlobalRedCounter[PROCESS_RANK]) {
			// resetting counters
			RedRecvCounter = 0;
			eventsExeuted = 0;

			int bufferSize = (sizeof(unsigned int) * NUM_PROCESS) / sizeof(int);
			int* dataBuffer = new int[bufferSize];

			// serailize global msg counter
			int index = 0;
			for (int P = 0; P < NUM_PROCESS; P++) {
				if (wasGreenPrev) {
					GlobalMsgCounter[P] = GreenOutCounter[P];
					GreenOutCounter[P] = 0;
				}
				else {
					GlobalMsgCounter[P] = BlueOutCounter[P];
					BlueOutCounter[P] = 0;
				}
				EventAction::AddToBuffer(dataBuffer, (int*)&GlobalMsgCounter[P], index, GlobalMsgCounter[P]);
			}

			// chaning to RED region
			CurrMsgColor = MsgColor::Red;

			// sending to next processor
			MPI_Request request;
			MPI_Isend(dataBuffer, bufferSize, MPI_INTEGER, (PROCESS_RANK + 1) % NUM_PROCESS, GVT_TAG, MPI_COMM_WORLD, &request);
		}
		
#ifdef DEBUG
		cout << "NEXT LOOP" << endl; fflush(stdout);
#endif 		
	}
	// finalizing simulation
	CommunicationFinalize();

	// destoying sets
	ES.~EventSet();

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

	GlobalMsgCounter =	new unsigned int[NUM_PROCESS];
	GlobalRedCounter =	new unsigned int[NUM_PROCESS];
	GreenOutCounter = new unsigned int[NUM_PROCESS];
	RedOutCounter = new unsigned int[NUM_PROCESS];
	BlueOutCounter = new unsigned int[NUM_PROCESS];

	// initializing counter to 0
	for (int i = 0; i < NUM_PROCESS; i++) {
		GreenOutCounter[i] = 0;
		RedOutCounter[i] = 0;
		BlueOutCounter[i] = 0;
		GlobalMsgCounter[i] = 0;
		GlobalRedCounter[i] = 0;
	}

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

// Event Action's schedule event, will be used
void EventAction::ScheduleEventIn(Time deltaT, EventAction* ea, int LP)
{
	// create and push new anti-msg
	_antiMsgs.push(new EventAction::AntiMsgStruct(this->_eventId, LP, deltaT + SimulationTime));

	// schedule w/sim-exec
	ScheduleEvent(deltaT + SimulationTime, ea, LP);
}

void EventAction::SendAntiMsg()
{
	while (!_antiMsgs.empty()) {
		// create anti msg then schedule anti-msg
		ScheduleEvent(_antiMsgs.top()->_t, new AntiMsg(_antiMsgs.top()->_eventId), _antiMsgs.top()->_LP);

		// remove from stack
		delete _antiMsgs.top();
		_antiMsgs.pop();
	}
}

// removing all anti-msgs
EventAction::~EventAction()
{
	while (!_antiMsgs.empty()) {
		// remove from stack
		delete _antiMsgs.top();
		_antiMsgs.pop();
	}
}

