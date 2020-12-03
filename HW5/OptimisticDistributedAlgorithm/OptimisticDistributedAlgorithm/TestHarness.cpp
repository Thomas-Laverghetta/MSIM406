#include "TestHarness.h"

void TestOptimisticSimulation()
{
	InitializeSimulation();
	RegisterEventActionClass(SimpleEA::_EventClassID, SimpleEA::New);

	int p = CommunicationSize();
	int n = 1;
	double td_mean = 5;
	double tw_max = 10;

	for (int i = 0; i < n; i++)
	{
		printf("Sending out initial events ...");
		InitialScheduleEventIn(Uniform(-10,10).GetRV(), new SimpleEA(td_mean, p, tw_max), rand() % CommunicationSize());
	}

	RunSimulation(50);
}
