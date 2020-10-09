#include <iostream>
#include "Nbody.h"
#include "TestHarness.h"
#include <chrono>

void GravitySimulation() {
	const double dt = 1.0;
	const int numBodies = 100;
	Nbody nbody(numBodies, dt);
	nbody.RunSimulation(100);
}

int main() {
	// Butterfly Test-harness
	// Butterfly_Barrier_Delay_Tester();

	// running the simulation
	GravitySimulation();
	
	return 0;
}