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
	TestEA(int id) {
		_dist = new Triangular(5, 7, 8);
		_ID = id;
	}

	void Execute() {
		printf("\tEVENT %i EXEC CURR=%i | TIME=%f\n", _ID, CommunicationRank(), GetSimulationTime()); fflush(stdout);

		Time nextTime = _dist->GetRV();
		int nextProcess = rand() % CommunicationSize();

		printf("SCH_EVENT %i CURR=%i->PROC=%i | SCH_TIME=%f\n",_ID, CommunicationRank(), nextProcess, nextTime + GetSimulationTime()); fflush(stdout);
		ScheduleEventIn(nextTime, new TestEA(_ID), nextProcess);
	}
	const int GetBufferSize() { return sizeof(_ID)/sizeof(int); }
	void Serialize(int* dataBuffer, int& index) {
		EventAction::AddToBuffer(dataBuffer, (int*)&_ID, index, _ID);
	}
	void Deserialize(int* dataBuffer, int& index) {
		EventAction::TakeFromBuffer(dataBuffer, (int*)&_ID, index, _ID);
	}
private:
	Distribution* _dist;
	int _ID;
};

void Test1() {

	InitializeSimulation();
	SetSimulationLookahead(5);

	// registering event with sim exec
	RegisterEventActionClass(TestEA::getUniqueId(), TestEA::New);
	

	for (int i = 0; i < 2; i++) {
		Time nextTime = Triangular(5,7,9).GetRV();
		int nextProcess = rand() % CommunicationSize();
		int _ID = rand();
		printf("SCH_EVENT %i CURR=%i->PROC=%i | SCH_TIME=%f\n", _ID, CommunicationRank(), nextProcess, nextTime + GetSimulationTime()); fflush(stdout);
		ScheduleEventIn(nextTime, new TestEA(_ID), nextProcess);
	}
	RunSimulation(15);
}
#endif // !TEST_HARNESS_H

