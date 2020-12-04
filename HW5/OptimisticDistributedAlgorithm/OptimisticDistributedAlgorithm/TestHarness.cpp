#include "TestHarness.h"

void TestOptimisticSimulation()
{
	InitializeSimulation();
	RegisterEventActionClass(SimpleEA::_EventClassID, SimpleEA::New);

	int p = CommunicationSize();
	int n = 5;
	double td_mean = 2;
	double tw_max = 5;

	for (int i = 0; i < n; i++)
	{
		printf("Sending out initial events ...\n"); 
		InitialScheduleEventIn(0, new SimpleEA(td_mean, p, tw_max), rand() % CommunicationSize());
	}

	RunSimulation(10);
}
