#include "SimulationExecutive.h"
#include "TestHarness.hpp"
#include <cstdio>

using namespace SimExec;

int main() {
	InitializeSimulation();

	SetSimulationLookahead(5);

	RunSimulation(20);
	return 0;
}