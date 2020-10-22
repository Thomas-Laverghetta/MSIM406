#include "Communication.h"

struct TestStruct
{
	int j;
	double y;
};

class ObjectA : public CommunicationPattern
{
public:
	ObjectA();
	void InitAttributes();
	void PrintContents();
private:
	double x;
	TestStruct testStruct1;
	int k;
	TestStruct *testStruct2;
	const int GetBufferSize();
	void Serialize( int *dataBuffer);
	void Deserialize( int *dataBuffer);
};
