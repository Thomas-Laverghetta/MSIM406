#ifndef SIMULATION_EXEC_H
#define SIMULATION_EXEC_H

typedef double Time;

class EventAction {
public:
	// Executes events to change system state
	virtual void Execute() = 0;

	// Gets buffer size for data to serialize
	virtual const int GetBufferSize() = 0;

	// serializes data into buffer
	virtual void Serialize(int* dataBuffer) = 0;

	// deserializes data from buffer
	virtual void Deserialize(int* dataBuffer) = 0;

protected:
	// Adds data from variable to buffer (serializes data)
	template <class T>
	void AddToBuffer(int* dataBuffer, int* dataRef, int& index, T obj)
	{
		for (int i = 0; i < sizeof(T) / sizeof(int); i++) {
			dataBuffer[index++] = dataRef[i];
		}
	}

	// removes data from buffer and adds to variable (deserialization)
	template <class T>
	void TakeFromBuffer(int* dataBuffer, int* dataRef, int& index, T obj)
	{
		for (int i = 0; i < sizeof(T) / sizeof(int); i++) {
			dataRef[i] = dataBuffer[index++];
		}
	}
};

typedef EventAction* (*NewFunctor)();

// returns current simulation time
Time GetSimulationTime();

// schedules event in future
void ScheduleEventIn(const Time& deltaT, EventAction* ea, int LP);

// Starts the simulation
void RunSimulation(Time T = -1);

// Initializes Simulation
void InitializeSimulation();

// Sets system wide lookahead
void SetLookahead(Time lookahead);

// Register EA class
void RegisterEventActionClass(unsigned int classId, NewFunctor newFunctor);
#endif // !SIMULATION_EXEC_H


