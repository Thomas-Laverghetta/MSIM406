#include "TestHarness.h"
#include <string>

ofstream SimpleEA::outputFile;
double SimpleEA::td_mean = 1.5;
double SimpleEA::tw_max = 5;
int n = 2;

Distribution* SimpleEA::td = 0;
Distribution* SimpleEA::tw = 0;
void TestOptimisticSimulation()
{
	InitializeSimulation();
	RegisterEventActionClass(SimpleEA::_EventClassID, SimpleEA::New);

	int p = CommunicationSize();

	SimpleEA::td = new Exponential(SimpleEA::td_mean);
	SimpleEA::tw = new Uniform(0.0f, SimpleEA::tw_max / (((float)CommunicationRank() + 1)));

	SimpleEA::outputFile = ofstream("process_" + to_string(CommunicationRank()) + ".csv");

	for (int i = 0; i < n; i++)
	{
		InitialScheduleEventIn(0, new SimpleEA, rand() % CommunicationSize());
	}

	RunSimulation(10000);
}
