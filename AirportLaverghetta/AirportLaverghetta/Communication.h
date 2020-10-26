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
	virtual const int GetBufferSize() = 0;
	virtual void Serialize( int *dataBuffer) = 0;
	virtual void Deserialize( int *dataBuffer) = 0;
};

#endif
