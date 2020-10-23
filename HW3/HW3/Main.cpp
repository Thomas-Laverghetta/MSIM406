#include "Communication.h"
#include "Airplane.h"
#include <stdlib.h>     /* srand, rand */
#include <iostream>
using namespace std;

int GetNextDest()
{
	int rank = CommunicationRank();
	int size = CommunicationSize();
	int offset = rand() % (size - 1);
	return((rank + 1 + offset) % size);
}

int main( int argc, char *argv[])
{
	CommunicationInitialize();

	// plane pointer
	Airplane* plane;

	// number of initial planes
	int initNumPlanes = 3;

	// number of planes that have finished and left simulation
	unsigned int numPlanesFinished = 0;

	// Setting RNG seed
	srand(CommunicationRank() * 3);

	// initial planes
	for (int i = 0; i < initNumPlanes; i++) {
		plane = new Airplane(1.0f);
		plane->PrintAirplane();
		plane->SendFlight(GetNextDest());
		delete plane;
		plane = nullptr;
	}

	// while not all planes have finished
	while (numPlanesFinished < CommunicationSize() * initNumPlanes) {
		int tag;
		int source;
		while (!(CheckForComm(tag, source)));

		// Deserialize Plane
		plane = new Airplane(source);

		// tag = 1 == plane coming in
		if (tag) {
			// send plane
			plane->PrintAirplane();
			plane->AddFlight();
			plane->AddFlightOrigin();
			plane->SendFlight(GetNextDest());
		}

		// tag = 0 == plane is finished
		else {
			// plane finished
			printf("\tPLANE FINISHED RECV | ");
			plane->PrintAirplane();
			numPlanesFinished++;
		}

		// Remove plane from this airport (either finished or sent to another processor)
		delete plane;
		plane = nullptr;
	}

	CommunicationFinalize();

	std::cout << "rank " << CommunicationRank() << " done" << std::endl << std::flush;
}
