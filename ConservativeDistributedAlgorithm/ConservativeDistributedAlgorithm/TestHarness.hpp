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
	TestEA(int id, int rank) {
		_dist = new Triangular(5, 7, 8);
		_ID = id;
		_origin = rank;
	}

	void Execute() {
		printf("EVENT %3i:%-3i EXEC CURR=%i | TIME=%f\n", _ID, _origin, CommunicationRank(), GetSimulationTime()); fflush(stdout);

		Time nextTime = _dist->GetRV();
		int nextProcess = rand() % CommunicationSize();

		/*printf("SCH_EVENT %i CURR=%i->PROC=%i | SCH_TIME=%f\n",_ID, CommunicationRank(), nextProcess, nextTime + GetSimulationTime()); fflush(stdout);*/
		ScheduleEventIn(nextTime, new TestEA(_ID, _origin), nextProcess);
	}
	const int GetBufferSize() { return (sizeof(_ID) + sizeof(_origin))/sizeof(int); }
	void Serialize(int* dataBuffer, int& index) {
		EventAction::AddToBuffer(dataBuffer, (int*)&_ID, index, _ID);
		EventAction::AddToBuffer(dataBuffer, (int*)&_origin, index, _origin);
	}
	void Deserialize(int* dataBuffer, int& index) {
		EventAction::TakeFromBuffer(dataBuffer, (int*)&_ID, index, _ID);
		EventAction::TakeFromBuffer(dataBuffer, (int*)&_origin, index, _origin);
	}
private:
	Distribution* _dist;
	int _ID;
	int _origin;
};

void Test1() {

	InitializeSimulation();
	SetSimulationLookahead(5);

	// registering event with sim exec
	RegisterEventActionClass(TestEA::getUniqueId(), TestEA::New);
	

	for (int i = 0; i < 2; i++) {
		Time nextTime = Triangular(5,7,9).GetRV();
		int nextProcess = (CommunicationRank() + 1) % CommunicationSize();
		int _ID = rand()%100;
		printf("SCH_EVENT %i CURR=%i->PROC=%i | SCH_TIME=%f\n", _ID, CommunicationRank(), nextProcess, nextTime + GetSimulationTime()); fflush(stdout);
		ScheduleEventIn(nextTime, new TestEA(_ID, CommunicationRank()), nextProcess);
	}
	RunSimulation(15);
}
#endif // !TEST_HARNESS_H

