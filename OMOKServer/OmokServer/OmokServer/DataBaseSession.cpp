#include "DataBaseSession.h"

cDataBaseSession::cDataBaseSession()
{
	Connection = nullptr;
	Result = nullptr;
	Row = NULL;

	Server = "localhost";
	User = "root";
	Password = "mo26591548";
	Database = "userlist";
}

cDataBaseSession::~cDataBaseSession()
{
	mysql_free_result(Result); // Query�� ������ ���� Result Set�� �ݵ�� ������ �����.

	mysql_close(Connection); // MySQL ������ ���� ����

	std::cout << "[ Server ]  MySQL Disconnect Complete" << std::endl;
}

bool cDataBaseSession::StartMySQL()
{
	Connection = mysql_init(NULL);  // �ʱ�ȭ �Լ�( �ݵ�� �������� �ʱ�ȭ )

	if (!mysql_real_connect(Connection, Server, User, Password, Database, 3306, NULL, 0))  // MySQL ���� �Լ�
	{
		std::cout << "[ Server ]  mysql_real_connect is Failed..." << std::endl;
		return false;
	}

	std::cout << "[ Server ]  MySQL Connect Complete" << std::endl;

	if(mysql_query(Connection, "show tables"))
	{
		std::cout << "[ Server ]  mysql_query 'show tables' is Failed..." << std::endl;
		return false;
	}

	Result = mysql_use_result(Connection);

	std::cout << "[ Server ]  MySQL Table in Mysql Database" << std::endl;

	while ((Row = mysql_fetch_row(Result)) != NULL)
	{
		std::cout << "[ Server ]  Table : " << Row[0] << std::endl;
	}

	return true;
}

bool cDataBaseSession::CheckUser(std::string UserID, std::string UserPW)
{
	if (mysql_query(Connection, "SELECT * FROM LoginData")) //  SELECT * FROM ���������� ���̺��� ������
	{
		std::cout << "[ Server ]  mysql_query 'SELECT * FORM' is Failed..." << std::endl;
		return false;
	}

	Result = mysql_use_result(Connection); //  ������ ���� ��� ����, ���н� NULL.
										   //  mysql_use_result ���� mysql_fetch_row �Լ��� ������� NULL�� ���ϵ� �� ���� �����ؾ���
										   //  �׷��� ���� ��� ȣ�� ���� ���� �ο찡 ���� ���� ���� ��� ���� �Ϻκ����� ���ϵǰ� C-API ERROR �߻���.
										   //  MySQL ������ ���� ���� �����͸� �ٷ� �б� ������ �޸� ����� ���� ������.
										   //  mysql_store_result �Լ��� �ӽ� ���̺��̳� ���� ���ۿ� �����͸� �����ؼ� ����ϹǷ� ������尡 �߻��ϰ� ������.

	bool Check = false;
	std::string s1;
	std::string s2;

	while ((Row = mysql_fetch_row(Result)) != NULL) //  mysql_fetch_row �Լ��� Result Set���� �ϳ��� row �迭�� ������.
	{

		s1 = Row[0];
		s2 = Row[1];

		if (s1 == UserID)
		{
			std::cout << "[ Server ]  MySQL in Table 'LoginData' Check UserID Complete" << std::endl;

			if (s2 == UserPW)
			{
				std::cout << "[ Server ]  MySQL in Table 'LoginData' Check UserPW Complete" << std::endl;
				Check = true;
			}
		}
	}

	if (Check == true)
	{
		return true;
	}
	else
	{
		std::cout << "[ Server ]  MySQL in Table 'LoginData' Check UserData Failed..." << std::endl;
		return false;
	}
}

bool cDataBaseSession::CreateUser(std::string UserID, std::string UserPW)
{
	//  INSERT INTO VALUES �������� �Է¹��� ID, PW�� ���� string���� ó�� �� c_str()�� C ���ڿ� ��ȯ
	std::string InputData = "INSERT INTO LoginData(UserID, UserPW) VALUES ('" + UserID + "', '" + UserPW + "')";

	if (mysql_query(Connection, InputData.c_str())) 
	{
		std::cout << "[ Server ]  mysql_query 'INSERT INTO VALUES' is Failed..." << std::endl;
		return false;
	}

	if (mysql_query(Connection, "SELECT * FROM LoginData")) //  SELECT * FROM ���������� ���̺��� ������
	{
		std::cout << "[ Server ]  mysql_query 'SELECT * FORM' is Failed..." << std::endl;
		return false;
	}

	Result = mysql_use_result(Connection);

	while ((Row = mysql_fetch_row(Result)) != NULL)
	{
		std::cout << Row[0] << Row[1] << std::endl;
	}

	return true;
}