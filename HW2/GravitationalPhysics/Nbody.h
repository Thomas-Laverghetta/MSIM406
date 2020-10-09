#pragma once
#include <fstream>

class Nbody
{
public:
	Nbody(int n, double deltaT);
	~Nbody();
	void RunSimulation(int numSteps);
private:
	int _processID, _numProcess;
	double _t;
	int _n;
	double _deltaT;
	double *_bodiesX;
	double* _bodiesY;
	double* _bodiesM;
	double* _bodiesVx;
	double* _bodiesVy;
	double* _bodiesAx;
	double* _bodiesAy;
	void Update();
	void ShareData();
	void PrintBody(int i);
	void PrintBodies(double t);
	std::ofstream fout;
};