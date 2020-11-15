#include "TestHarness.hpp"
#include <iostream>
#include <random>

using namespace std;
using namespace SimExec;

unsigned int Airplane::_nextId = 0;

Airplane::Airplane(double capacity, Distribution* dist)
{
	_dist = dist;
	_cargo.capacity = capacity;

	// setting IDs
	_processorId = CommunicationRank();
	_planeId = _nextId++;
	_lastFlight = _processorId;

	_numFlights = 0;
}

void Airplane::Arrival()
{
	AddFlight();
	
	PrintAirplane();
	
	// determing if plane has finished and will be broadcasted
	if (!MaxFlight()) {
		AddFlightOrigin();
		SendFlight(rand() % CommunicationSize());
	}
}

void Airplane::SendFlight(int rank)
{
	ScheduleEventIn(_dist->GetRV(), new AirplaneArrival(this), rank);
}

// Number of flights completed
void Airplane::AddFlight()
{
	_numFlights++;
}

void Airplane::AddFlightOrigin()
{
	_lastFlight = CommunicationRank();
}

void Airplane::AddCargo(double size)
{
	if (Fits(size)) {
		_cargo.quantity++;
		_cargo.size += size;
	}
}

void Airplane::RemoveCargo(double size)
{
	_cargo.quantity--;
	_cargo.size -= size;
}

int Airplane::GetCargoQuantity()
{
	return _cargo.quantity;
}

double Airplane::GetCargoSize()
{
	return _cargo.size;
}

bool Airplane::Fits(double size)
{
	return (size + _cargo.size <= _cargo.capacity);
}

void Airplane::PrintAirplane()
{
	printf("%i,%i,%i,%i,%i,%i,%f,%f\n",
		CommunicationRank(), _lastFlight, _processorId, _planeId, _numFlights, _cargo.quantity, _cargo.capacity, _cargo.size);
	fflush(stdout);
}


void Airplane::Serialize(int* dataBuffer, int& index)
{
	EventAction::AddToBuffer(dataBuffer, (int*)&_processorId, index, _processorId);
	EventAction::AddToBuffer(dataBuffer, (int*)&_planeId, index, _planeId);
	EventAction::AddToBuffer(dataBuffer, (int*)&_numFlights, index, _numFlights);
	EventAction::AddToBuffer(dataBuffer, (int*)&_lastFlight, index, _lastFlight);
	EventAction::AddToBuffer(dataBuffer, (int*)&_cargo.quantity, index, _cargo.quantity);
	EventAction::AddToBuffer(dataBuffer, (int*)&_cargo.capacity, index, _cargo.capacity);
	EventAction::AddToBuffer(dataBuffer, (int*)&_cargo.size, index, _cargo.size);
}


void Airplane::Deserialize(int* dataBuffer, int& index)
{
	EventAction::TakeFromBuffer(dataBuffer, (int*)&_processorId, index, _processorId);
	EventAction::TakeFromBuffer(dataBuffer, (int*)&_planeId, index, _planeId);
	EventAction::TakeFromBuffer(dataBuffer, (int*)&_numFlights, index, _numFlights);
	EventAction::TakeFromBuffer(dataBuffer, (int*)&_lastFlight, index, _lastFlight);
	EventAction::TakeFromBuffer(dataBuffer, (int*)&_cargo.quantity, index, _cargo.quantity);
	EventAction::TakeFromBuffer(dataBuffer, (int*)&_cargo.capacity, index, _cargo.capacity);
	EventAction::TakeFromBuffer(dataBuffer, (int*)&_cargo.size, index, _cargo.size);
}

