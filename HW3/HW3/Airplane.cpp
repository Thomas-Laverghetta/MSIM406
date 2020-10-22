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

	inBuffer = new int[GetBufferSize()];
}

Airplane::Airplane(int * source)
{
	Deserialize(source);
}

void Airplane::SendFlight(int rank)
{
	Serialize(inBuffer);

	// send MPI HERE
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
void AddDataToBuffer(int * databuffer, int * )

void Airplane::Serialize(int* databuffer)
{
	int index = 0;
	int* dataRef;
	dataRef = (int*)&x;
	for (int i = 0; i < sizeof(x) / sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i];
	}
	dataRef = (int*)&(testStruct1.y);
	for (int i = 0; i < sizeof(testStruct1.y) / sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i];
	}
	dataRef = (int*)&(testStruct1.j);
	for (int i = 0; i < sizeof(testStruct1.j) / sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i];
	}
	dataRef = (int*)&(testStruct2->y);
	for (int i = 0; i < sizeof(testStruct2->y) / sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i];
	}
	dataRef = (int*)&(testStruct2->j);
	for (int i = 0; i < sizeof(testStruct2->j) / sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i];
	}
	dataRef = (int*)&k;
	for (int i = 0; i < sizeof(k) / sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i];
	}
}
