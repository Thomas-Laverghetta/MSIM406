#include "Airplane.h"
#include <random>
Airplane::Airplane(double capacity)
{
	_cargo.capacity = capacity;

	// setting IDs
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	_processorId = rank;
	_planeId = rand();

	_numflightsCompleted = 0;

}

Airplane::Airplane(int source)
{
	Receive(source);
}

void Airplane::SendFlight(int rank)
{
	Send(rank);
}

void Airplane::AddFlight()
{
	_numflightsCompleted++;
}

void Airplane::AddFlightOrigin()
{
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	_previousProcessor = rank;
}

void Airplane::AddCargo(double size)
{
	_cargo.quantity++;
	_cargo.size += size;
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
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	printf("Curr Proc %i | Previous Proc %i | Origin rank %i | Plane ID %i | num flights %i | Cargo Quantity %i | Cargo Capacity %f | Cargo Utilized %f\n",
		rank, _previousProcessor, _processorId, _planeId, _numflightsCompleted, _cargo.quantity, _cargo.capacity, _cargo.size);
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

	AddToBuffer(dataBuffer, (int*)&_planeId, index, _planeId);
	AddToBuffer(dataBuffer, (int*)&_processorId, index, _processorId);
	AddToBuffer(dataBuffer, (int*)&_numflightsCompleted, index, _numflightsCompleted);
	AddToBuffer(dataBuffer, (int*)&_previousProcessor, index, _previousProcessor);
	AddToBuffer(dataBuffer, (int*)&_cargo.quantity, index, _cargo.quantity);
	AddToBuffer(dataBuffer, (int*)&_cargo.capacity, index, _cargo.capacity);
	AddToBuffer(dataBuffer, (int*)&_cargo.size, index, _cargo.size);


	//cout << "Serialize: rank " << CommunicationRank() << ": buffersize = " << GetBufferSize() << endl << flush;
	//for (int i = 0; i < GetBufferSize(); i++) {
	//	cout << dataBuffer[i] << " ";
	//}
	//cout << endl << flush;
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

	TakeFromBuffer(dataBuffer, (int*)&_planeId, index, _planeId);
	TakeFromBuffer(dataBuffer, (int*)&_processorId, index, _processorId);
	TakeFromBuffer(dataBuffer, (int*)&_numflightsCompleted, index, _numflightsCompleted);
	TakeFromBuffer(dataBuffer, (int*)&_previousProcessor, index, _previousProcessor);
	TakeFromBuffer(dataBuffer, (int*)&_cargo.quantity, index, _cargo.quantity);
	TakeFromBuffer(dataBuffer, (int*)&_cargo.capacity, index, _cargo.capacity);
	TakeFromBuffer(dataBuffer, (int*)&_cargo.size, index, _cargo.size);

	delete dataBuffer;
}
