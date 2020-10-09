#define _USE_MATH_DEFINES
#include <math.h>
#include <random>
#include <iostream>
#include <mpi.h>
#include <string>
#include "Barrier.h"
#include "Nbody.h"

//#define DEBUG
#define RING
//#define MPI_BARRIER
//#define BUTTERFLY

using namespace std;

std::default_random_engine generator;
std::normal_distribution<double> Distance(0.0, 2.0);
std::uniform_real_distribution<double> Angle(0.0, 2 * M_PI);

//#define xOff 0
//#define yOff 1
//#define vxOff 2
//#define vyOff 3
//#define mOff 4
#define G 6.673E-11 //Nm^2kg^-2

double Dist(double xi, double yi, double xj, double yj)
{
	double xdiff = xi - xj;
	double ydiff = yi - yj;
	return(std::sqrt(xdiff * xdiff + ydiff * ydiff));
}

Nbody::Nbody(int n, double deltaT)
{
	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &_processID);
	MPI_Comm_size(MPI_COMM_WORLD, &_numProcess);

#if defined(RING)
	fout = std::ofstream("RING_gravity_" + std::to_string(_numProcess) + "_processors.csv");
#elif defined(BUTTERFLY)
	fout = std::ofstream("BUTTERFLY_gravity_" + std::to_string(_numProcess) + "_processors.csv");
#elif defined(MPI_BARRIER)
	fout = std::ofstream("MPI_barrier_gravity_" + std::to_string(_numProcess) + "_processors.csv");
#endif

	_t = 0;
	_n = n;
	_deltaT = deltaT;
	_bodiesX = new double[n];
	_bodiesY = new double[n];
	_bodiesM = new double[n];
	_bodiesVx = new double[n];
	_bodiesVy = new double[n];
	_bodiesAx = new double[n];
	_bodiesAy = new double[n];
	for (int i = 0; i < n; i++) {
		_bodiesVx[i] = 0.0;
		_bodiesVy[i] = 0.0;
		_bodiesAx[i] = 0.0;
		_bodiesAy[i] = 0.0;
	}
	if (_processID == 0) {
		for (int i = 0; i < n; i++) {
			double x, y, dist, angle;
			dist = Distance(generator);
			angle = Angle(generator);
			x = dist * cos(angle);
			y = dist * sin(angle);
			_bodiesX[i] = x;
			_bodiesY[i] = y;
			_bodiesM[i] = 1000.0;
		}
	}
#ifdef DEBUG
	fout << _processID << " broadcast" << endl;
#endif
	MPI_Bcast(_bodiesX, n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
#ifdef DEBUG
	fout << _processID << " broadcast" << endl;
#endif
	MPI_Bcast(_bodiesY, n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
#ifdef DEBUG
	fout << _processID << " broadcast" << endl;
#endif
	MPI_Bcast(_bodiesM, n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
#ifdef DEBUG
	fout << _processID << " broadcast" << endl;
#endif
}

Nbody::~Nbody()
{
	MPI_Finalize();
}

void Nbody::RunSimulation(int numSteps)
{
	for (int step = 0; step < numSteps; step++) {
		Update();
#if defined(RING)
		Ring_Barrier();
#elif defined(BUTTERFLY)
		Butterfly_Barrier();
#elif defined(MPI_BARRIER)
		MPI_Barrier(MPI_COMM_WORLD);
#endif
		ShareData();
		if (_processID == 0) {
			PrintBodies(step*_deltaT);
		}
	}
}

void Nbody::ShareData()
{
#ifdef DEBUG
	fout << _processID << " sharing data" << endl;
#endif
	int n = _n / _numProcess;
	for (int p = 0; p < _numProcess; p++) {
		MPI_Bcast(&(_bodiesX[p * n]), n, MPI_DOUBLE, p, MPI_COMM_WORLD);
		MPI_Bcast(&(_bodiesY[p * n]), n, MPI_DOUBLE, p, MPI_COMM_WORLD);
		MPI_Bcast(&(_bodiesM[p * n]), n, MPI_DOUBLE, p, MPI_COMM_WORLD);
	}
#ifdef DEBUG
	fout << _processID << " done sharing data" << endl;
#endif
}

void Nbody::Update()
{
	_t += _deltaT;
	int n = _n / _numProcess;
	for (int i = _processID * n; i < _processID * n + n; i++) {
		_bodiesAx[i] = _bodiesAy[i] = 0.0;
		double xi = _bodiesX[i];
		double yi = _bodiesY[i];
		for (int j = 0; j < _n; j++) {
			if (j != i) {
				double xj = _bodiesX[j];
				double yj = _bodiesY[j];
				double m = _bodiesM[j];
				double d = Dist(xi, yi, xj, yj);
				_bodiesAx[i] += m * (xi - xj) / d;
				_bodiesAy[i] += m * (yi - yj) / d;
			}
		}
		_bodiesAx[i] *= -G;
		_bodiesAy[i] *= -G;
	}
	for (int i = _processID * n; i < _processID * n + n; i++) {
		double vx = _bodiesX[i] + _deltaT * _bodiesAx[i];
		double vy = _bodiesY[i] + _deltaT * _bodiesAy[i];
		_bodiesVx[i] = vx;
		_bodiesVy[i] = vy;
		_bodiesX[i] += _deltaT * vx;
		_bodiesY[i] += _deltaT * vy;
	}
}

void Nbody::PrintBody(int i)
{
	if (i >= 0) {
		fout << _t << ", " << _bodiesX[i] << ", " << _bodiesY[i] << std::endl;
	}
	else {
		fout << _t << ", ";
		for (int i = 0; i < _n; i++) {
			fout << _bodiesX[i] << ", " << _bodiesY[i] << ", ";
		}
		fout << std::endl;
	}
}

void Nbody::PrintBodies(double t)
{
	fout << t << ", ";
	for (int i = 0; i < _n; i++) {
		fout << _bodiesX[i] << ", " << _bodiesY[i] << ", ";
	}
	fout << endl;
}
