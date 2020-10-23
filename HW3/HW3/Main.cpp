#include "Communication.h"
#include "Airplane.h"

#include <iostream>
using namespace std;

int main( int argc, char *argv[])
{
	CommunicationInitialize();

	unsigned int finished = 0;
	while (finished) {
		int tag;
		int source;
		while (!(CheckForComm(tag, source)));

	}

	CommunicationFinalize();

	cout << "rank " << CommunicationRank() << " done" << endl << flush;
}
