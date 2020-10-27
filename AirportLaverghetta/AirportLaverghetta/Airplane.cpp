#include "Airplane.h"
#include <iostream>
#include <random>

using namespace std;

Airplane::Airplane(double capacity)
{
	_cargo.capacity = capacity;

	// setting IDs
	_processorId = CommunicationRank();
	_planeId = rand() % 1000;
	_lastFlight = _processorId;

	_numFlights = 0;
}

Airplane::Airplane(int source)
{
	// recieve plane
	Receive(source, 1);
}

void Airplane::SendFlight(int rank)
{
	if (_numFlights != _maxFlgihts) {
		// send plane
		Send(rank, 1);
	}
	else {
		// send broadcast. This notifies all that plane is done
		Broadcast(0);
	}
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
	//printf("Curr Proc %i | Previous Proc %i | Origin Proc %i | Plane ID %4i | num flights %3i | Cargo Quantity %3i | Cargo Capacity %3f | Cargo Utilized %f\n",
	//	CommunicationRank(), _lastFlight, _processorId, _planeId, _numFlights, _cargo.quantity, _cargo.capacity, _cargo.size);

	printf("%i,%i,%i,%i,%i,%i,%f,%f\n",
		CommunicationRank(), _lastFlight, _processorId, _planeId, _numFlights, _cargo.quantity, _cargo.capacity, _cargo.size);
	fflush(stdout);
}

template <class T>
void AddToBuffer(int* dataBuffer, int* dataRef, int& index, T obj)
{
	for (int i = 0; i < sizeof(T) / sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i];
	}
}
void Airplane::Serialize(int* dataBuffer)
{
	int index = 0;

	AddToBuffer(dataBuffer, (int*)&_processorId, index, _processorId);
	AddToBuffer(dataBuffer, (int*)&_planeId, index, _planeId);
	AddToBuffer(dataBuffer, (int*)&_numFlights, index, _numFlights);
	AddToBuffer(dataBuffer, (int*)&_lastFlight, index, _lastFlight);
	AddToBuffer(dataBuffer, (int*)&_cargo.quantity, index, _cargo.quantity);
	AddToBuffer(dataBuffer, (int*)&_cargo.capacity, index, _cargo.capacity);
	AddToBuffer(dataBuffer, (int*)&_cargo.size, index, _cargo.size);
}

template <class T>
void TakeFromBuffer(int* dataBuffer, int* dataRef, int& index, T obj)
{
	for (int i = 0; i < sizeof(T) / sizeof(int); i++) {
		dataRef[i] = dataBuffer[index++];
	}
}
void Airplane::Deserialize(int* dataBuffer)
{
	int index = 0;

	TakeFromBuffer(dataBuffer, (int*)&_processorId, index, _processorId);
	TakeFromBuffer(dataBuffer, (int*)&_planeId, index, _planeId);
	TakeFromBuffer(dataBuffer, (int*)&_numFlights, index, _numFlights);
	TakeFromBuffer(dataBuffer, (int*)&_lastFlight, index, _lastFlight);
	TakeFromBuffer(dataBuffer, (int*)&_cargo.quantity, index, _cargo.quantity);
	TakeFromBuffer(dataBuffer, (int*)&_cargo.capacity, index, _cargo.capacity);
	TakeFromBuffer(dataBuffer, (int*)&_cargo.size, index, _cargo.size);
}
