#pragma once
#include "Communication.h"
#include <stdio.h>
#include "mpi.h"

// Cargo for the plane
struct Cargo { 
	unsigned int quantity = 0; 
	double capacity = 0.0f; 
	double size = 0.0f; 
};

class Airplane : CommunicationPattern
{
public:
	Airplane(double capacity);
	Airplane(int source);

	/// Sends the airplane to the process indicated by rank. 
	void SendFlight(int rank);

	/// increments num flights
	void AddFlight();

	/// Gets the number of flights completed
	bool MaxFlight() { return _numFlights == _maxFlgihts; }

	/// sets the origin of the last flight
	void AddFlightOrigin();

	/// Adds cargo to plane
	void AddCargo(double size);

	/// Removes cargo from plane
	void RemoveCargo(double size);

	/// Returns number of items in cargo
	int GetCargoQuantity();

	/// Returns how much space utilized
	double GetCargoSize();

	/// Returns whether or not item with this size can fit
	bool Fits(double size);

	/// Prints the plane console
	void PrintAirplane();
private:
	// Serialization Methods
	const int GetBufferSize() {
		return ((sizeof(_planeId) +
			sizeof(_processorId) +
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
	
	unsigned int _numFlights;		// number of flights completed
	unsigned int _lastFlight;		// last processor

	const unsigned int _maxFlgihts = 10;	// maximum number of flights

	// Cargo for this plane
	Cargo _cargo;
};

