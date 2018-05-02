#include "IOCPServer.h"

int main()
{
	cIOCPServer Server;
	Server.InitServer();

	std::cout << "[ Server ]  Server Running..." << std::endl;
	std::cout << "[ Server ]  If you want Shut Down, Please press Any Key..." << std::endl;

	getchar();

	Server.m_cLog.WritePacketStatisticsFile();
	Server.CloseServer();
	return 0;
}