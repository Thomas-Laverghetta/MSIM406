#include "Communication.h"
#include "ObjectA.h"

#include <iostream>
using namespace std;

int main( int argc, char *argv[])
{
	ObjectA objA;

	CommunicationInitialize();

	if (CommunicationRank() == 0) {
		int tag;
		int source;
		while (!(CheckForComm( tag, source)));
		objA.Receive( source); 
		cout << "rank 0 received objA\n" << flush;
		objA.PrintContents(); }
	else {
		objA.InitAttributes();
		objA.PrintContents();
		cout << "rank 1 is sending objA\n" << flush;
		objA.Send( 0);
	}

	CommunicationFinalize();

	cout << "rank " << CommunicationRank() << " done" << endl << flush;
}
