#ifndef SIMULATION_EXEC_H
#define SIMULATION_EXEC_H

#include <float.h>


// defining time
typedef double Time;
#define TIME_MAX DBL_MAX

// defining event action
class EventAction {
public:
	EventAction() {
		// random event id
		_eventId = rand();
	}

	// Executes event to change system state
	virtual void Execute() = 0;

	// Gets buffer size for data to serialize
	virtual const int GetBufferSize() = 0;

	// serializes data into buffer
	virtual void Serialize(int* dataBuffer, int& index) = 0;

	// deserializes data from buffer
	virtual void Deserialize(int* dataBuffer, int& index) = 0;

	// get the event class Id
	virtual const int GetEventClassId() { return INT_MIN; }

	// schedules event in future
	void ScheduleEventIn(Time deltaT, EventAction* ea, int LP);

	// Returns event's ID (not class ID)
	unsigned int GetEventId() { return _eventId; }

	// Sets event id
	void SetEventId(unsigned int& eventId) { _eventId = eventId; }

	// Adds data from variable to buffer (serializes data)
	template <class T>
	static void AddToBuffer(int* dataBuffer, int* dataRef, int& index, T obj)
	{
		for (int i = 0; i < sizeof(T) / sizeof(int); i++) {
			dataBuffer[index++] = dataRef[i];
		}
	}

	// removes data from buffer and adds to variable (deserialization)
	template <class T>
	static void TakeFromBuffer(int* dataBuffer, int* dataRef, int& index, T obj)
	{
		for (int i = 0; i < sizeof(T) / sizeof(int); i++) {
			dataRef[i] = dataBuffer[index++];
		}
	}

	~EventAction();
private:
	unsigned int _eventId;
};

// Unique Event ID
/// Each event action will declare this within class arg so that class has unique class ID
#define UNIQUE_EVENT_ID(ID) \
public: \
	static const int _EventClassID =  ID;\
	const int GetEventClassId() { return _EventClassID; };


typedef EventAction* (*NewFunctor)();

// returns current simulation time
Time GetSimulationTime();

// schedules event in future
void InitialScheduleEventIn(Time deltaT, EventAction* ea, int LP);

// Starts the simulation
void RunSimulation(Time T);

// Initializes Simulation
void InitializeSimulation();

// Sets system wide lookahead
void SetSimulationLookahead(Time lookahead);

// Register EA class
void RegisterEventActionClass(unsigned int classId, NewFunctor newFunctor);

// returns process id 
int CommunicationRank();

// Returns number of processes
int CommunicationSize();

#endif // !SIMULATION_EXEC_H


