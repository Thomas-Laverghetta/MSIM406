#pragma once
#include "SimulationExecutive.h"
#include "Distribution.h"

// Cargo for the plane
struct Cargo {
	unsigned int quantity = 0;
	double capacity = 0.0f;
	double size = 0.0f;
};

class Airplane
{
public:
	Airplane(double capacity, Distribution * dist = new Triangular(5.0,6.0,7.0));
	Airplane() {}

	///
	void Arrival();

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
	void Serialize(int* databuffer, int& index);
	void Deserialize(int* databuffer, int& index);

private:
	// Identifiers 
	unsigned int _planeId;
	unsigned int _processorId;

	unsigned int _numFlights;		// number of flights completed
	unsigned int _lastFlight;		// last processor

	const unsigned int _maxFlgihts = 10;	// maximum number of flights

	// Cargo for this plane
	Cargo _cargo;

	// Time Distributions
	Distribution * _dist;

	static unsigned int _nextId;
};

class AirplaneArrival : public EventAction {
public:
	AirplaneArrival() {}
	AirplaneArrival(Airplane * plane) { _plane = plane; }
	void Execute() { 
		_plane->Arrival();
	}

	const int GetBufferSize() {
		return _plane->GetBufferSize();
	}

	void Serialize(int* dataBuffer, int& index) {
		_plane->Deserialize(dataBuffer, index);
	}

	void Deserialize(int* dataBuffer, int& index) { 
		_plane = new Airplane(0);
		_plane->Serialize(dataBuffer, index);
	}

	static int classId;
	int GetClassId() { return classId; }

	static EventAction* New() { return new AirplaneArrival; }
private:
	// save a airplane
	Airplane* _plane;
};
