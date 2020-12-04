#ifndef TESTHARNESS_H
#define TESTHARNESS_H

#include "SimulationExecutive.h"
#include "Distribution.h"
#include <random>
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
class SimpleEA: public EventAction
{
	UNIQUE_EVENT_ID(1);
public:

	SimpleEA() 
	{
		_td_mean = 0;
		_td = NULL;
		_p = 0;
		_tw_max = 0;
	}

	SimpleEA(double td_mean, int p, double tw_max) 
	{ 
		_td_mean = td_mean;
		_td = new Exponential(td_mean); 
		_p = p;
		_tw_max = tw_max;
	}

	void Execute()
	{
		Sleep((unsigned long)(Uniform(0.0f, _tw_max/(((float)CommunicationRank() + 1))).GetRV() * 1e3));
		ScheduleEventIn(_td->GetRV(), new SimpleEA(_td_mean, _p, _tw_max), rand() % CommunicationSize());
		printf("PROC=%i | EVENT_ID=%i | TIME=%f\n", CommunicationRank(), this->GetEventId(), GetSimulationTime()); fflush(stdout);
	}

	const int GetBufferSize() { return (sizeof(_td_mean) + sizeof(_p) + sizeof(_tw_max))/sizeof(int); }

	void Serialize(int* databuffer, int& index) 
	{
		AddToBuffer(databuffer, (int*)&_td_mean, index, _td_mean);
		AddToBuffer(databuffer, (int*)&_p, index, _p);
		AddToBuffer(databuffer, (int*)&_tw_max, index, _tw_max);
	}
	
	void Deserialize(int* databuffer, int& index) 
	{
		TakeFromBuffer(databuffer, (int*)&_td_mean, index, _td_mean);
		_td = new Exponential(_td_mean);
		TakeFromBuffer(databuffer, (int*)&_p, index, _p);
		TakeFromBuffer(databuffer, (int*)&_tw_max, index, _tw_max);
	}

	static EventAction* New() { return new SimpleEA; }

	~SimpleEA() {
		delete _td;
		_td = 0;
	}
private:
	double _td_mean;
	Distribution* _td;
	int _p;
	double _tw_max;
};

#endif // !TESTHARNESS_H