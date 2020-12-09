#ifndef TESTHARNESS_H
#define TESTHARNESS_H

#include "SimulationExecutive.h"
#include "Distribution.h"
#include <random>
#include <fstream>
#include <Windows.h>

using namespace std;

/*
	Tests optimstic time management simulation executive 
	following assignment requirements.
*/
void TestOptimisticSimulation();

/*
	A simple event action that when executed sleeps for 
	uniformly distributed amount of time (simulate work) 
	and then schedules an event with the same event action
	on itself of another LP.
*/
class SimpleEA : public EventAction
{
	UNIQUE_EVENT_ID(1);
public:

	SimpleEA(){}

	void Execute()
	{
		Sleep((unsigned long)(tw->GetRV() * 1e3));
		ScheduleEventIn(td->GetRV(), new SimpleEA, rand() % CommunicationSize());

		// process, event id, simulation time
		outputFile << CommunicationRank() << ',' << this->GetEventId() << ',' << GetSimulationTime() << '\n';
		printf("%i,%i,%f\n", CommunicationRank(), this->GetEventId(), GetSimulationTime()); fflush(stdout);
	}

	const int GetBufferSize() { return 0; }

	void Serialize(int* databuffer, int& index) {}

	void Deserialize(int* databuffer, int& index) {	}

	static EventAction* New() { return new SimpleEA; }

	~SimpleEA() {}

	static ofstream outputFile;
	static double td_mean;
	static Distribution* td;
	static double tw_max;
	static Distribution* tw;
};

#endif // !TESTHARNESS_H