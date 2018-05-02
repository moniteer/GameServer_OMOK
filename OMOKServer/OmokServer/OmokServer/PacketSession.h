#pragma once


#define FIRST_CLIENT_CODE			133
#define SECOND_CLIENT_CODE			134
#define LOGIN_SUCCESS_CODE			135
#define LOGOUT_ANOTHER_CODE			136
#define LOGIN_FAIL_CODE				137
#define CREATE_USER_SUCCESS_CODE	138
#define CREATE_USER_FAIL_CODE		139
#define GAME_START_FAIL_CODE		143
#define NOT_JOIN_ROOM_STATUS_CODE	144
#define NEED_ONE_MORE_USER_CODE		145
#define GAME_START_CODE				146
#define GAME_TIME_OUT_CODE			147
#define GAME_END_CODE				148
#define MAKE_ROOM_SUCCESS_CODE		151
#define MAKE_ROOM_FAIL_CODE			152
#define ANOTHER_MAKE_ROOM_CODE		153
#define ALREADY_MAKE_ROOM_CODE		154
#define JOIN_ROOM_SUCCESS_CODE		155
#define JOIN_ROOM_FAIL_CODE			156
#define ANOTHER_JOIN_ROOM_CODE		157
#define PROGRAM_SHUTDOWN_CODE		158
#define CHECK_ALIVE_CODE			159
#define NEED_CREATE_ROOM_CODE		160
#define DISCONNECT_ANOTHER_CODE		161



///===============///
///  패킷 공용체	  ///
///===============///

enum PacketType
{
	NONE = 0,

	LOGIN_REQUEST = 1, LOGIN_SUCCESS, LOGIN_FAIL, LOGOUT_ANOTHER,
					   CREATE_USER_REQUEST, CREATE_USER_SUCCESS, CREATE_USER_FAIL,

	MY_STONE_PUT = 10, ANOTHER_STONE_PUT, MY_GAME_TIME_OUT, ANOTHER_GAME_TIME_OUT, 
					   GAME_START_REQUEST, GAME_START_SUCCESS, GAME_START_FAIL, GAME_END,

	MAKE_ROOM_REQUEST = 20, MAKE_ROOM_SUCCESS, MAKE_ROOM_FAIL, ALREADY_MAKE_ROOM, ANOTHER_MAKE_ROOM,
							JOIN_ROOM_REQUEST, JOIN_ROOM_SUCCESS, JOIN_ROOM_FAIL, ANOTHER_JOIN_ROOM, ALREADY_JOIN_ROOM,

	MY_CHAT = 30, ANOTHER_CHAT,

	MAKE_ERROR = 40, MAKE_MEMORY_LEAK, PROGRAM_SHUTDOWN, PROGRAM_QUIT, CHECK_ALIVE_REQUEST, CHECK_ALIVE, STILL_ALIVE, ECHO_SEND, ECHO_RECV
};



///=============///
///  패킷 헤드    ///
///============= ///


class PacketHead
{
private:
	int PacketSize;
	PacketType Type;

public:
	PacketHead() : Type(NONE), PacketSize(0) { }
	PacketHead(PacketType _Type, int _PacketSize) : Type(_Type), PacketSize(_PacketSize) {}
	
	PacketType Get_PacketHeader()
	{
		return Type;
	}
};




///===============///
///  로그인 패킷    ///
///===============///


class Login_Request : PacketHead
{
public:
	char ID_value[15];
	char PW_value[15];

public:
	Login_Request() :ID_value(), PW_value(){}
	Login_Request(std::string _ID, std::string _PW) : PacketHead(LOGIN_REQUEST, sizeof(Login_Request))
	{
		strcpy(ID_value, _ID.c_str());
		strcpy(PW_value, _PW.c_str());
	}
};


class Login_Success : PacketHead
{
public:
	int SuccessCode;

public:
	Login_Success() : SuccessCode(){}
	Login_Success(int _code) : PacketHead(LOGIN_SUCCESS, sizeof(Login_Success))
	{
		SuccessCode = _code;
	}
};


class Login_Fail : PacketHead
{
public:
	int FailCode;

public:
	Login_Fail() : FailCode(){}
	Login_Fail(int _code) : PacketHead(LOGIN_FAIL, sizeof(Login_Fail))
	{
		FailCode = _code;
	}
};


class Logout_Another : PacketHead
{
public:
	int LogOutCode;

public:
	Logout_Another() : LogOutCode(){}
	Logout_Another(int _code) : PacketHead(LOGOUT_ANOTHER, sizeof(Logout_Another))
	{
		LogOutCode = _code;
	}
};



class Create_User_Request : PacketHead
{
public:
	char ID_value[15];
	char PW_value[15];

public:
	Create_User_Request() :ID_value(), PW_value() {}
	Create_User_Request(std::string _ID, std::string _PW) : PacketHead(CREATE_USER_REQUEST, sizeof(Create_User_Request))
	{
		strcpy(ID_value, _ID.c_str());
		strcpy(PW_value, _PW.c_str());
	}
};


class Create_User_Success : PacketHead
{
public:
	int SuccessCode;

public:
	Create_User_Success() : SuccessCode() {}
	Create_User_Success(int _code) : PacketHead(CREATE_USER_SUCCESS, sizeof(Create_User_Success))
	{
		SuccessCode = _code;
	}
};


class Create_User_Fail : PacketHead
{
public:
	int FailCode;

public:
	Create_User_Fail() : FailCode() {}
	Create_User_Fail(int _code) : PacketHead(CREATE_USER_FAIL, sizeof(Create_User_Fail))
	{
		FailCode = _code;
	}
};








///=============///
///  로비 패킷   ///
///=============///


class Make_Room_Request : PacketHead
{
public:
	int RequestCode;

public:
	Make_Room_Request() : RequestCode(){}
	Make_Room_Request(int _code) : PacketHead(MAKE_ROOM_REQUEST, sizeof(Make_Room_Request))
	{
		RequestCode = _code;
	}
};


class Make_Room_Success : PacketHead
{
public:
	int SuccessCode;

public:
	Make_Room_Success() : SuccessCode(){}
	Make_Room_Success(int _code) : PacketHead(MAKE_ROOM_SUCCESS, sizeof(Make_Room_Success))
	{
		SuccessCode = _code;
	}
};


class Make_Room_Fail : PacketHead
{
public:
	int FailCode;

public:
	Make_Room_Fail() : FailCode(){}
	Make_Room_Fail(int _code) : PacketHead(MAKE_ROOM_FAIL, sizeof(Make_Room_Fail))
	{
		FailCode = _code;
	}
};


class Already_Make_Room : PacketHead
{
public:
	int ClientID;

public:
	Already_Make_Room() : ClientID(){}
	Already_Make_Room(int _code) : PacketHead(ALREADY_MAKE_ROOM, sizeof(Already_Make_Room))
	{
		ClientID = _code;
	}
};


class Another_Make_Room : PacketHead
{
public:
	int RoomNumber;

public:
	Another_Make_Room() : RoomNumber(){}
	Another_Make_Room(int _code) : PacketHead(ANOTHER_MAKE_ROOM, sizeof(Another_Make_Room))
	{
		RoomNumber = _code;
	}
};


class Join_Room_Request : PacketHead
{
public:
	int RequestCode;

public:
	Join_Room_Request() : RequestCode(){}
	Join_Room_Request(int _code) : PacketHead(JOIN_ROOM_REQUEST, sizeof(Join_Room_Request))
	{
		RequestCode = _code;
	}
};


class Join_Room_Success : PacketHead
{
public:
	int JoinCode;

public:
	Join_Room_Success() : JoinCode(){}
	Join_Room_Success(int _code) : PacketHead(JOIN_ROOM_SUCCESS, sizeof(Join_Room_Success))
	{
		JoinCode = _code;
	}
};



class Join_Room_Fail : PacketHead
{
public:
	int FailCode;

public:
	Join_Room_Fail() : FailCode(){}
	Join_Room_Fail(int _code) : PacketHead(JOIN_ROOM_FAIL, sizeof(Join_Room_Fail))
	{
		FailCode = _code;
	}
};


class Another_Join_Room : PacketHead
{
public:
	char UserID[15];

public:
	Another_Join_Room() : UserID(){}
	Another_Join_Room(std::string _userid) : PacketHead(ANOTHER_JOIN_ROOM, sizeof(Another_Join_Room))
	{
		strcpy(UserID, _userid.c_str());
	}
};


class Already_Join_Room : PacketHead
{
public:
	char UserID[15];

public:
	Already_Join_Room() : UserID(){}
	Already_Join_Room(std::string _userid) : PacketHead(ALREADY_JOIN_ROOM, sizeof(Already_Join_Room))
	{
		strcpy(UserID, _userid.c_str());
	}
};





///=============///
///  게임 패킷   ///
///=============///


class Game_My_Stone_Put : PacketHead
{
public:
	int Index_X;
	int Index_Y;

public:
	Game_My_Stone_Put() : Index_X(), Index_Y(){}
	Game_My_Stone_Put(int _x, int _y) : PacketHead(MY_STONE_PUT, sizeof(Game_My_Stone_Put))
	{
		Index_X = _x;
		Index_Y = _y;
	}
};


class Game_Another_Stone_Put : PacketHead
{
public:
	int Index_X;
	int Index_Y;

public:
	Game_Another_Stone_Put() : Index_X(), Index_Y(){}
	Game_Another_Stone_Put(int _x, int _y) : PacketHead(ANOTHER_STONE_PUT, sizeof(Game_Another_Stone_Put))
	{
		Index_X = _x;
		Index_Y = _y;
	}
};


class My_Game_Time_Out : PacketHead
{
public:
	bool Timeout;

public:
	My_Game_Time_Out() : Timeout(){}
	My_Game_Time_Out(bool _timeout) : PacketHead(MY_GAME_TIME_OUT, sizeof(My_Game_Time_Out))
	{
		Timeout = _timeout;
	}
};


class Another_Game_Time_Out : PacketHead
{
public:
	bool Timeout;

public:
	Another_Game_Time_Out() : Timeout(){}
	Another_Game_Time_Out(bool _timeout) : PacketHead(ANOTHER_GAME_TIME_OUT, sizeof(Another_Game_Time_Out))
	{
		Timeout = _timeout;
	}
};


class Game_Start_Request: PacketHead
{
public:
	int RequestCode;

public:
	Game_Start_Request() : RequestCode(){}
	Game_Start_Request(int _code) : PacketHead(GAME_START_REQUEST, sizeof(Game_Start_Request))
	{
		RequestCode = _code;
	}
};


class Game_Start_Success : PacketHead
{
public:
	int StartCode;

public:
	Game_Start_Success() : StartCode(){}
	Game_Start_Success(int _code) : PacketHead(GAME_START_SUCCESS, sizeof(Game_Start_Success))
	{
		StartCode = _code;
	}
};


class Game_Start_Fail : PacketHead
{
public:
	int FailCode;

public:
	Game_Start_Fail() : FailCode(){}
	Game_Start_Fail(int _code) : PacketHead(GAME_START_FAIL, sizeof(Game_Start_Fail))
	{
		FailCode = _code;
	}
};


class Game_End : PacketHead
{
public:
	int EndCode;

public:
	Game_End() : EndCode(){}
	Game_End(int _code) : PacketHead(GAME_END, sizeof(Game_End))
	{
		EndCode = _code;
	}
};





///=============///
///  채팅 패킷   ///
///=============///


class My_Chat : PacketHead
{
public:
	char Chatting[50];

public:
	My_Chat() : Chatting(){}
	My_Chat(std::string _chatting) : PacketHead(MY_CHAT, sizeof(My_Chat))
	{
		strcpy(Chatting, _chatting.c_str());
	}
};


class Another_Chat : PacketHead
{
public:
	char Chatting[50];

public:
	Another_Chat() : Chatting(){ }
	Another_Chat(std::string _chatting) : PacketHead(ANOTHER_CHAT, sizeof(Another_Chat))
	{
		strcpy(Chatting, _chatting.c_str());
	}
};




///=============///
///  에러 패킷   ///
///=============///


class MakeDump_Error : PacketHead
{
public:
	int ErrorCode;

public:
	MakeDump_Error() : ErrorCode() {}
	MakeDump_Error(int _code) : PacketHead(MAKE_ERROR, sizeof(MakeDump_Error))
	{
		ErrorCode = _code;
	}
};


class Make_Memory_Leak : PacketHead
{
public:
	int ErrorCode;

public:
	Make_Memory_Leak() : ErrorCode() {}
	Make_Memory_Leak(int _code) : PacketHead(MAKE_MEMORY_LEAK, sizeof(Make_Memory_Leak))
	{
		ErrorCode = _code;
	}
};


class Program_Shutdown : PacketHead
{
public:
	int ErrorCode;

public:
	Program_Shutdown() : ErrorCode() {}
	Program_Shutdown(int _code) : PacketHead(PROGRAM_SHUTDOWN, sizeof(Program_Shutdown))
	{
		ErrorCode = _code;
	}
};


class Program_Quit : PacketHead
{
public:
	int ErrorCode;

public:
	Program_Quit() : ErrorCode() {}
	Program_Quit(int _code) : PacketHead(PROGRAM_QUIT, sizeof(Program_Quit))
	{
		ErrorCode = _code;
	}
};


class Check_Alive_Request : PacketHead
{
public:
	int CheckCode;

public:
	Check_Alive_Request() : CheckCode() {}
	Check_Alive_Request(int _code) : PacketHead(CHECK_ALIVE_REQUEST, sizeof(Check_Alive_Request))
	{
		CheckCode = _code;
	}
};


class Check_Alive : PacketHead
{
public:
	int CheckCode;

public:
	Check_Alive() : CheckCode() {}
	Check_Alive(int _code) : PacketHead(CHECK_ALIVE, sizeof(Check_Alive))
	{
		CheckCode = _code;
	}
};



class Still_Alive : PacketHead
{
public:
	int CheckCode;

public:
	Still_Alive() : CheckCode() {}
	Still_Alive(int _code) : PacketHead(STILL_ALIVE, sizeof(Still_Alive))
	{
		CheckCode = _code;
	}
};





///=============///
///  에코 패킷   ///
///=============///



class Echo_Send : PacketHead
{
public:
	int CheckCode;

public:
	Echo_Send() : CheckCode() {}
	Echo_Send(int _code) : PacketHead(ECHO_SEND, sizeof(Echo_Send))
	{
		CheckCode = _code;
	}
};


class Echo_Recv : PacketHead
{
public:
	int CheckCode;

public:
	Echo_Recv() : CheckCode() {}
	Echo_Recv(int _code) : PacketHead(ECHO_RECV, sizeof(Echo_Recv))
	{
		CheckCode = _code;
	}
};