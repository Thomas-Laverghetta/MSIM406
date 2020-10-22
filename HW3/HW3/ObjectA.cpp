#include "ObjectA.h"

#include <iostream>
using namespace std;

ObjectA::ObjectA() : CommunicationPattern()
{
	x = 0.0;
	//c = 'a';
	testStruct1.y = 0.0;
	testStruct1.j = 0;
	testStruct2 = new TestStruct;
	testStruct2->y = 0.0;
	testStruct2->j = 0;
	k = 0;
}

void ObjectA::InitAttributes()
{
	x = 4.8;
	//c = 'z';
	testStruct1.y = 21.9;
	testStruct1.j = 83;
	testStruct2->y = 101.33;
	testStruct2->j = 99;
	k = 42;
}

const int ObjectA::GetBufferSize()
{
	return( 
		(sizeof( x) + 
		sizeof( testStruct1.y) + 
		sizeof( testStruct1.j) + 
		sizeof( testStruct2->y) + 
		sizeof( testStruct2->j) + 
		sizeof( k))
		/sizeof(int));
}

void ObjectA::Serialize( int *dataBuffer)
{
	int index = 0;
	int *dataRef;
	dataRef = (int*) &x;
	for (int i = 0; i < sizeof(x)/sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i]; }
	dataRef = (int*) &(testStruct1.y);
	for (int i = 0; i < sizeof(testStruct1.y)/sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i]; }
	dataRef = (int*) &(testStruct1.j);
	for (int i = 0; i < sizeof(testStruct1.j)/sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i]; }
	dataRef = (int*) &(testStruct2->y);
	for (int i = 0; i < sizeof(testStruct2->y)/sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i]; }
	dataRef = (int*) &(testStruct2->j);
	for (int i = 0; i < sizeof(testStruct2->j)/sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i]; }
	dataRef = (int*) &k;
	for (int i = 0; i < sizeof(k)/sizeof(int); i++) {
		dataBuffer[index++] = dataRef[i]; }

	cout << "Serialize: rank " << CommunicationRank() << ": buffersize = " << GetBufferSize() << endl << flush;
	for (int i = 0; i < GetBufferSize(); i++) {
		cout << dataBuffer[i] << " "; }
	cout << endl << flush;
}

void ObjectA::Deserialize( int *dataBuffer)
{
	cout << "Deserialize: rank " << CommunicationRank() << ": buffersize = " << GetBufferSize() << endl << flush;
	for (int i = 0; i < GetBufferSize(); i++) {
		cout << dataBuffer[i] << " "; }
	cout << endl << flush;

	int index = 0;
	int *dataRef;
	dataRef = (int*) &x;
	for (int i = 0; i < sizeof(x)/sizeof(int); i++) {
		dataRef[i] = dataBuffer[index++]; }
	dataRef = (int*) &(testStruct1.y);
	for (int i = 0; i < sizeof(testStruct1.y)/sizeof(int); i++) {
		dataRef[i] = dataBuffer[index++]; }
	dataRef = (int*) &(testStruct1.j);
	for (int i = 0; i < sizeof(testStruct1.j)/sizeof(int); i++) {
		dataRef[i] = dataBuffer[index++]; }
	dataRef = (int*) &(testStruct2->y);
	for (int i = 0; i < sizeof(testStruct2->y)/sizeof(int); i++) {
		dataRef[i] = dataBuffer[index++]; }
	dataRef = (int*) &(testStruct2->j);
	for (int i = 0; i < sizeof(testStruct2->j)/sizeof(int); i++) {
		dataRef[i] = dataBuffer[index++]; }
	dataRef = (int*) &k;
	for (int i = 0; i < sizeof(k)/sizeof(int); i++) {
		dataRef[i] = dataBuffer[index++]; }
	delete dataBuffer;
}

void ObjectA::PrintContents()
{
	cout << "\nContents of ObjA on " << CommunicationRank() << ":\n" << flush;
	cout << "	rank = " << CommunicationRank() << endl << flush;
	cout << "	x = " << x << endl << flush;
	cout << "	testStruct1.y = " << testStruct1.y << endl << flush;
	cout << "	testStruct1.j = " << testStruct1.j << endl << flush;
	cout << "	testStruct2->y = " << testStruct2->y << endl << flush;
	cout << "	testStruct2->j = " << testStruct2->j << endl << flush;
	cout << "	k = " << k << endl << endl << flush;
}
