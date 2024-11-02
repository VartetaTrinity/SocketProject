#include <iostream>
#include <WinSock2.h>

#define PORT 19999

class SocketComm
{
private:
	struct sockaddr_in srv;
	int nListenerSocket;
	int nClientSocket[5];
	fd_set fr;
	int nMaxFd;

	void AcceptNewConnection();
	void RecvSendClientMsg(int nCliSocket);

public:
	SocketComm();
	int StartSocketServer();
	~SocketComm();
};