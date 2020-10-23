#pragma once
#include "Communication.h"
#include <stdio.h>
#include "mpi.h"


class Airplane : CommunicationPattern
{
public:
	Airplane(double capacity);
	Airplane(int source);

	/// Sends the airplane to the process indicated by rank. 
	void SendFlight(int rank);

	/// increments num flights
	void AddFlight();

	/// sets the origin of the last flight
	void AddFlightOrigin();

	/// 
	void AddCargo(double size);


	void RemoveCargo(double size);


	int GetCargoQuantity();


	double GetCargoSize();


	bool Fits(double size);


	void PrintAirplane();
private:
	// deserialization
	const int GetBufferSize() {
		return ((sizeof(_planeId) +
			sizeof(_lastFlight) +
			sizeof(_numFlights) +
			sizeof(_cargo.capacity) +
			sizeof(_cargo.quantity) +
			sizeof(_cargo.size)) / 
			sizeof(int));
	}
	void Serialize(int* databuffer);
	void Deserialize(int* databuffer);

	// Identifiers 
	unsigned int _planeId;
	unsigned int _processorId;
	
	unsigned int _numFlights;				// number of flights completed
	unsigned int _lastFlight;		// last processor

	// Cargo Specifications
	struct Cargo { unsigned int quantity = 0; double capacity = 0.0f; double size = 0.0f; };
	Cargo _cargo;
};

