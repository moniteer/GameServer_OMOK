#pragma once

#include <iostream>
#include <WinSock2.h>
#include <string>
#include <mysql.h>

#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "ws2_32.lib")

class cDataBaseSession
{
public:
	MYSQL* Connection;
	MYSQL_RES* Result;
	MYSQL_ROW Row;

	char* Server;
	char* User;
	char* Password;
	char* Database;

public:
	cDataBaseSession();
	~cDataBaseSession();

	bool StartMySQL();
	bool CheckUser(std::string UserID, std::string UserPW);
	bool CreateUser(std::string UserID, std::string UserPW);
};