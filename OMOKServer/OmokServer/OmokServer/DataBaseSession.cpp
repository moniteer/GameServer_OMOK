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
	mysql_free_result(Result); // Query문 실행이 끝난 Result Set은 반드시 해제해 줘야함.

	mysql_close(Connection); // MySQL 서버와 연결 종료

	std::cout << "[ Server ]  MySQL Disconnect Complete" << std::endl;
}

bool cDataBaseSession::StartMySQL()
{
	Connection = mysql_init(NULL);  // 초기화 함수( 반드시 시작전에 초기화 )

	if (!mysql_real_connect(Connection, Server, User, Password, Database, 3306, NULL, 0))  // MySQL 연결 함수
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
	if (mysql_query(Connection, "SELECT * FROM LoginData")) //  SELECT * FROM 쿼리문으로 테이블을 가져옴
	{
		std::cout << "[ Server ]  mysql_query 'SELECT * FORM' is Failed..." << std::endl;
		return false;
	}

	Result = mysql_use_result(Connection); //  쿼리문 실행 결과 저장, 실패시 NULL.
										   //  mysql_use_result 사용시 mysql_fetch_row 함수로 결과값이 NULL이 리턴될 때 까지 실행해야함
										   //  그렇지 않을 경우 호출 되지 않은 로우가 다음 쿼리 중의 결과 셋의 일부분으로 리턴되고 C-API ERROR 발생함.
										   //  MySQL 서버로 부터 직접 데이터를 바로 읽기 때문에 메모리 사용이 적고 빠르다.
										   //  mysql_store_result 함수는 임시 테이블이나 로컬 버퍼에 데이터를 저장해서 사용하므로 오버헤드가 발생하고 느리다.

	bool Check = false;
	std::string s1;
	std::string s2;

	while ((Row = mysql_fetch_row(Result)) != NULL) //  mysql_fetch_row 함수는 Result Set에서 하나의 row 배열을 가져옴.
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
	//  INSERT INTO VALUES 쿼리문과 입력받은 ID, PW를 합쳐 string으로 처리 후 c_str()로 C 문자열 변환
	std::string InputData = "INSERT INTO LoginData(UserID, UserPW) VALUES ('" + UserID + "', '" + UserPW + "')";

	if (mysql_query(Connection, InputData.c_str())) 
	{
		std::cout << "[ Server ]  mysql_query 'INSERT INTO VALUES' is Failed..." << std::endl;
		return false;
	}

	if (mysql_query(Connection, "SELECT * FROM LoginData")) //  SELECT * FROM 쿼리문으로 테이블을 가져옴
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