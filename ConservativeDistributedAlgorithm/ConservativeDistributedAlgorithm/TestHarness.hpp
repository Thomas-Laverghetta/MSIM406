#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H
#include "SimulationExecutive.h"
#include "Distribution.h"

class TestEA : public EventAction {
	/// Defining EventAction Unique ID and New
	/// Only needed for Events getting sent (Event msgs) to other processors
	UNIQUE_EVENT_ID(2)
		static EventAction* New() { return new TestEA; }
public:
	TestEA() {
		_dist = new Triangular(5, 7, 8);
	}
	void Execute() {
		printf("\tR EVENT EXEC CURR=%i @ %f\n", CommunicationRank(), GetSimulationTime()); fflush(stdout);

		Time nextTime = _dist->GetRV();
		int nextProcess = rand() % CommunicationSize();

		printf("\tSCH EVENT CURR=%i TO %i @ %f\n",CommunicationRank(), nextProcess, nextTime); fflush(stdout);
		ScheduleEventIn(nextTime, new TestEA, nextProcess);
	}
	const int GetBufferSize() { return 0; }
	void Serialize(int* dataBuffer, int& index) {}
	void Deserialize(int* dataBuffer, int& index) {}
private:
	Distribution* _dist;
};

void Test1() {

	InitializeSimulation();
	SetSimulationLookahead(5);

	// registering airplane
	RegisterEventActionClass(TestEA::getUniqueId(), TestEA::New);
	

	Time nextTime = 6;
	int nextProcess = rand() % CommunicationSize();

	printf("\tSCH EVENT CURR=%i TO %i @ %f\n", CommunicationRank(), nextProcess, nextTime); fflush(stdout);
	ScheduleEventIn(nextTime, new TestEA, nextProcess);

	RunSimulation(15);
}
#endif // !TEST_HARNESS_H

