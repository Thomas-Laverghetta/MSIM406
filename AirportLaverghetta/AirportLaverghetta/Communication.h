#ifndef COMMUNICATION_H
#define COMMUNICATION_H

void CommunicationInitialize();
void CommunicationFinalize();
int CommunicationRank();
int CommunicationSize();
bool CheckForComm(int &tag, int &source);

class CommunicationPattern
{
public:
	void Send(int dest, int tag = 1);
	void Broadcast(int tag = 0);
	void Receive(int source, int tag = 1);
protected:
	CommunicationPattern() {}
	template <class T>
	void AddToBuffer(int* dataBuffer, int* dataRef, int& index, T obj)
	{
		for (int i = 0; i < sizeof(T) / sizeof(int); i++) {
			dataBuffer[index++] = dataRef[i];
		}
	}

	template <class T>
	void TakeFromBuffer(int* dataBuffer, int* dataRef, int& index, T obj)
	{
		for (int i = 0; i < sizeof(T) / sizeof(int); i++) {
			dataRef[i] = dataBuffer[index++];
		}
	}

	virtual const int GetBufferSize() = 0;
	virtual void Serialize( int *dataBuffer) = 0;
	virtual void Deserialize( int *dataBuffer) = 0;
};

#endif
