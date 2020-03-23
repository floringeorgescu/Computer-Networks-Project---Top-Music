struct user
{
	char *username;
	char *password;
	int type;
	MYSQL *con;
};

struct threadData
{
	int clientSocket;
	MYSQL *con;
};
