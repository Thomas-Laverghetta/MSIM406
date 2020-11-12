#include "SimulationExecutive.h"
#include <cstdio>

int main() {
	InitializeSimulation();

	SetSimulationLookahead(5);

	RunSimulation(20);
	return 0;
}