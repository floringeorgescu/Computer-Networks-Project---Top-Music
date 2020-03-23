#include <mysql.h> // librarii necesare pentru lucrul cu mysql
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>

#include "structs.h"

#define PORT 3000 // Portul pe care va porni serverul

MYSQL* initMysql();
int songExists(MYSQL *con, char *songName);
char* readData(int clientSocket);
void sendData(int clientSocket, char *message);
int login(char *username, char *password, MYSQL *con);
int registerUser(char *username, char *password, MYSQL *con);
void addSongToTop(int clientSocket, MYSQL *con);
void voteSong(int clientSocket, MYSQL *con, char *username, int canVote);
void showTop(int clientSocket, MYSQL *con);
void showTopOnGenre(int clientSocket, MYSQL *con);
void addComment(int clientSocket, MYSQL *con, char *username);
void restrictVote(int clientSocket, MYSQL *con, int userType);
void deleteSong(int clientSocket, MYSQL *con, int userType);
void getComments(int clientSocket, MYSQL *con);

void* clientThread(void *param); // functia ce va trata cererile

int main()
{
	struct sockaddr_in serverAddr; // aici vom specifica niste caracteristici ale serverului
	struct sockaddr_in clientAddr; // aici vor ajunge datele clientului

	int serverSocket = socket(AF_INET, SOCK_STREAM, 0); // creem un socket al serverului

	int reuse = 1;

	if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        printf("Can't set options! Line: %d\n", __LINE__);
    	return 1;
    }

	printf("Starting...\n");

	if (serverSocket < 0)
	{
		printf("Can't create the server socket! Line: %d\n", __LINE__); // Verificam server socket-ul.
																		// __LINE__ ia valoarea liniei in care se afla.
		return 1; // orice numar inafara de 0 este considerat eroare
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		printf("Error at bind! Line: %d\n", __LINE__);
		return 1;
	}

	if (listen(serverSocket, 5)) // permitem ca maxim 5 conexiuni sa stea la coada pentru a fi procesate
	{
		printf("Failed to listen! Line: %d\n", __LINE__);
		return 1;
	}

	MYSQL *con = initMysql();

	if (con == NULL)
	{
		return 1;
	}

	socklen_t clientAddressSize = sizeof(clientAddr); // socklen_t e ca un fel de size_t dar pentru socket

	printf("Started!\n");

	while(1) // while (true) -> bucla infinita. Vrem ca serverul sa mearga non stop.
	{
		int clientSocket = accept (serverSocket, (struct sockaddr *)&clientAddr, &clientAddressSize); // creem un socket pentru noul client

		if (clientSocket < 0) // verificam daca socketul e valid
		{
			printf("Failed to create a client socket! Line: %d\n", __LINE__);
		}

		printf("New client!\n");

		struct threadData userData;
		userData.clientSocket = clientSocket;
		userData.con = con;

		pthread_t thread;
		pthread_create(&thread, NULL, clientThread, (void*)&userData);
	}
	return 0;
}

void* clientThread(void *param)
{
	struct threadData *userData = (struct threadData*) param;
	int clientSocket = userData->clientSocket;
	MYSQL *con = userData->con;

	char *action = readData(clientSocket);

	char *username = readData(clientSocket);
	char *password = readData(clientSocket);
	int userType = 0; // 0 - user normal. 1 - admin
	int canVote = 0;

	if (strcmp(action, "login") == 0)
	{
		int loginResult = login(username, password, userData->con);

		if (loginResult != 0)
		{
			userType = loginResult % 100 / 10;
			canVote = loginResult % 10;
		}

		write(clientSocket, &loginResult, sizeof(int));
	}
	else if (strcmp(action, "register") == 0)
	{
		int registerResult = registerUser(username, password, con);
		sendData(clientSocket, registerResult == 1 ? "Success" : "Error");

		if (!registerResult)
		{
			return NULL;
		}
	}

	while(1)
	{
		MYSQL *tempCon = con;

		char *command = readData(clientSocket);

		printf("Command: %s\n", command);

		if (strcmp(command, "addSong") == 0)
		{
			addSongToTop(clientSocket, tempCon);
		}
		else if (strcmp(command, "voteSong") == 0)
		{
			voteSong(clientSocket, tempCon, username, canVote);
		}
		else if (strcmp(command, "generalTop") == 0)
		{
			showTop(clientSocket, tempCon);
		}
		else if (strcmp(command, "genreTop") == 0)
		{
			showTopOnGenre(clientSocket, tempCon);	
		}
		else if (strcmp(command, "comment") == 0)
		{
			addComment(clientSocket, tempCon, username);
		}
		else if (strcmp(command, "restrictVote") == 0)
		{
			restrictVote(clientSocket, tempCon, userType);
		}
		else if (strcmp(command, "deleteSong") == 0)
		{
			deleteSong(clientSocket, tempCon, userType);
		}
		else if (strcmp(command, "getComments") == 0)
		{
			getComments(clientSocket, tempCon);
		}
		else
		{
			sendData(clientSocket, "Command not found!");
		}
		free(command);
	}

	free(username);
	free(password);

	return NULL;
}

char* readData(int clientSocket)
{
	int dataSize = 0;
	read(clientSocket, &dataSize, sizeof(int));

	char *data = malloc(sizeof (char) * (dataSize + 1));
	read(clientSocket, data, sizeof(char) * dataSize);
	data[dataSize] = '\0';
	// char end = '\0';
	// strcat(data, &end);
	printf("Data: |%s|\n", data);

	return data;
}

void sendData(int clientSocket, char *message)
{
	int messageLength = strlen(message);
	write(clientSocket, &messageLength, sizeof(int));
	write(clientSocket, message, sizeof(char) * messageLength);
}

MYSQL* initMysql()
{
	MYSQL *con = mysql_init(NULL);

	if (con == NULL)
	{
		printf("Can't init the MySQL connection! Line: %d\n", __LINE__);
		return NULL;
	}

	if (mysql_real_connect(con, "localhost", "root", "password", "TopMusicDB", 0, NULL, 0) == NULL)
	{
		printf("Can't connect to the MySQL database! Line: %d\n", __LINE__);
		return NULL;
	}
	return con;
}

int songExists(MYSQL *con, char *songName)
{
	char *baseCheckSongQuery = "select * from songs where name = \"";
	int baseCheckSongQueryLength = strlen(baseCheckSongQuery);
	char *checkSongQuery = malloc(sizeof(char) * (baseCheckSongQueryLength + strlen(songName) + 10)); // 10 pentru ; \n etc...

	strcpy(checkSongQuery, baseCheckSongQuery);
	strcat(checkSongQuery, songName);
	strcat(checkSongQuery, "\";");

	printf("Query: %s\n", checkSongQuery);

	if (mysql_query(con, checkSongQuery))
  	{
      	printf("Can't run the query! Line: %d\n", __LINE__);
      	free(checkSongQuery);
      	return 0;
  	}

  	free(checkSongQuery);

  	MYSQL_RES *result = mysql_store_result(con);
  
  	if (result == NULL) 
  	{
     	printf("Can't get the result! Line: %d\n", __LINE__);
     	return 0;
  	}

  	MYSQL_ROW row = mysql_fetch_row(result);

  	if (row == NULL)
  	{
  		printf("Song not found! Line: %d\n", __LINE__);
      	return 0;
  	}
  	return 1;
}

void addSongToTop(int clientSocket, MYSQL *con)
{
	char *successMessage = "success";

	char *name = readData(clientSocket);
	char *description = readData(clientSocket);
	char *link = readData(clientSocket);
	int numberOfGenres = atoi(readData(clientSocket));

	int insertSongQueryLength = strlen("insert into songs (name, description, link) values ();") + strlen(name) + strlen(description) + strlen(link) + 10; // 10 pentru virgule si spatii
	char *insertSongQuery = malloc(sizeof(char) * insertSongQueryLength);

	strcpy(insertSongQuery, "insert into songs (name, description, link) values (\"");
	strcat(insertSongQuery, name);
	strcat(insertSongQuery, "\", \"");
	strcat(insertSongQuery, description);
	strcat(insertSongQuery, "\", \"");
	strcat(insertSongQuery, link);
	strcat(insertSongQuery, "\");");

  	printf("Query: %s\n", insertSongQuery);

	if (mysql_query(con, insertSongQuery))
  	{
      	printf("Can't run the query! Line: %d\n", __LINE__);
      	sendData(clientSocket, "Can't run the query!");
      	free(insertSongQuery);
      	return ;
  	}

	if (numberOfGenres == 0)
	{
		printf("Invalid number of genres! Line: %d\n", __LINE__);
		sendData(clientSocket, "Invalid number of genres!");
		return;
	}

	for (int i = 0; i < numberOfGenres; i++)
	{
		char *genre = readData(clientSocket);
		char *baseQuery = "insert into genres (songName, genre) values (\"\", \"\");";
		int addGenresQueryLength = strlen(baseQuery) + strlen(name) + strlen(genre);

		char* insertGenreQuery = malloc(sizeof(char) * addGenresQueryLength);
		strcpy(insertGenreQuery, "insert into genres (songName, genre) values (\"");
		strcat(insertGenreQuery, name);
		strcat(insertGenreQuery, "\", \"");
		strcat(insertGenreQuery, genre);
		strcat(insertGenreQuery, "\");");

		printf("Query: %s\n", insertGenreQuery);

		if (mysql_query(con, insertGenreQuery)) 
  		{
      		printf("Can't run the query! Line: %d\n", __LINE__);
      		sendData(clientSocket, "Can't run the query!");
      		free(insertSongQuery);
      		return;
  		}
  	}

  	sendData(clientSocket, successMessage);
}

void voteSong(int clientSocket, MYSQL *con, char *username, int canVote)
{
	if (!canVote)
	{
		printf("You don't have the right to vote!");
		sendData(clientSocket, "You don't have the right to vote!");
		return;
	}

	char *successMessage = "success";

	char *songName = readData(clientSocket);
	char *genre = readData(clientSocket);
	char *vote = readData(clientSocket);

	if (!songExists(con, songName))
	{
		printf("Song not found! Line: %d\n", __LINE__);
		sendData(clientSocket, "Song not found!");
		return;
	}

	char *baseInsertQuery = "insert into votes (songName, username, vote, genre) values (\"";
	int baseInsertQueryLength = strlen(baseInsertQuery);
	char *insertQuery = malloc(sizeof(char) * (baseInsertQueryLength + strlen(songName) + strlen(username) + strlen(vote) + strlen(genre) + 3 * strlen("\", \"") + strlen("\");")));
	strcpy(insertQuery, baseInsertQuery);
	strcat(insertQuery, songName);
	strcat(insertQuery, "\", \"");
	strcat(insertQuery, username);
	strcat(insertQuery, "\", \"");
	strcat(insertQuery, vote);
	strcat(insertQuery, "\", \"");
	strcat(insertQuery, genre);
	strcat(insertQuery, "\");");

	printf("Query: %s\n", insertQuery);

	if (mysql_query(con, insertQuery)) 
  	{
      	printf("Can't run the query! Line: %d\n", __LINE__);
      	sendData(clientSocket, "Can't run the query!");
      	free(insertQuery);
      	return;
  	}

  	sendData(clientSocket, successMessage);
}

void showTop(int clientSocket, MYSQL *con)
{
	if (mysql_query(con, "select songName, max(vote) from votes group by songName order by max(vote) desc;")) 
  	{
      	printf("Can't run the query! Line: %d\n", __LINE__);
      	sendData(clientSocket, "Can't run the query!");
      	return;
  	}
  
  	MYSQL_RES *result = mysql_store_result(con);
  
  	if (result == NULL) 
  	{
     	printf("Can't get the result! Line: %d\n", __LINE__);
     	sendData(clientSocket, "Can't get the result!");
     	return;
  	}

  	MYSQL_ROW row;

  	char *spacer = " --- ";

  	int numberOfFields = mysql_num_rows(result);
  	int fields = 0;

	char stringLines[10];
	sprintf(stringLines, "%d", numberOfFields); // transforma int in char *

  	sendData(clientSocket, stringLines);

  	while ((row = mysql_fetch_row(result))) 
  	{
      	char *generalTopLine = calloc(sizeof(char), strlen(row[0]) + strlen(spacer) + strlen(row[1]) + strlen(" votes"));
      	strcpy(generalTopLine, row[0]);
      	strcat(generalTopLine, spacer);
      	strcat(generalTopLine, row[1]);
      	strcat(generalTopLine, " points");

      	printf("%s\n", generalTopLine);

      	sendData(clientSocket, generalTopLine);

      	free(generalTopLine);

      	fields++;
      	if (fields > numberOfFields)
      	{
      		break;
      	}
  	}
  	printf("\n");

  	sendData(clientSocket, "End");
}

void showTopOnGenre(int clientSocket, MYSQL *con)
{
	char *genre = readData(clientSocket);
	int queryLength = strlen("select songName, max(vote) from votes where genre = \"") + strlen(genre) + strlen("  group by songName order by max(vote) desc;");
	char *query = malloc(sizeof(char) * queryLength);

	strcpy(query, "select songName, max(vote) from votes where genre = \"");
	strcat(query, genre);
	strcat(query, "\" group by songName order by max(vote) desc;");

	if (mysql_query(con, query)) 
  	{
      	printf("Can't run the query! Line: %d\n", __LINE__);
      	sendData(clientSocket, "Can't run the query!");
      	free(query);
      	return;
  	}

  	printf("Query: %s\n", query);

  	MYSQL_RES *result = mysql_store_result(con);
  
  	if (result == NULL) 
  	{
     	printf("Can't get the result! Line: %d\n", __LINE__);
     	sendData(clientSocket, "Can't get the result!");
     	return;
  	}

  	MYSQL_ROW row;

  	char *spacer = " --- ";

  	int numberOfFields = mysql_num_rows(result);
  	int fields = 0;

	char stringLines[10];
	sprintf(stringLines, "%d", numberOfFields); // transforma int in char *

  	sendData(clientSocket, stringLines);

  	printf("int lines: %d, string lines: %s\n", numberOfFields, stringLines);

  	while ((row = mysql_fetch_row(result))) 
  	{
      	char *genreTopLine = calloc(sizeof(char), strlen(row[0]) + strlen(spacer) + strlen(row[1]) + strlen(" votes"));
      	strcpy(genreTopLine, row[0]);
      	strcat(genreTopLine, spacer);
      	strcat(genreTopLine, row[1]);
      	strcat(genreTopLine, " points");

      	printf("%s\n", genreTopLine);

      	sendData(clientSocket, genreTopLine);

      	free(genreTopLine);

      	fields++;
      	if (fields > numberOfFields)
      	{
      		break;
      	}
  	}
  	printf("\n");

  	sendData(clientSocket, "End");
}

void addComment(int clientSocket, MYSQL *con, char *username)
{
	char *songName = readData(clientSocket);
	char *comment = readData(clientSocket);

	if (!songExists(con, songName))
	{
		printf("Song not found! Line: %d\n", __LINE__);
		sendData(clientSocket, "Song not found!");
		return;
	}

	char *baseQuery = "insert into comments (username, songName, comment) values (\"";
	int queryLength = strlen(baseQuery) + strlen(username) + strlen(songName) + strlen(comment) + 10; // 10 pentru "", spatii etc...
	char *query = malloc(sizeof(char) * queryLength);

	strcpy(query, baseQuery);
	strcat(query, username);
	strcat(query, "\", \"");
	strcat(query, songName);
	strcat(query, "\", \"");
	strcat(query, comment);
	strcat(query, "\");");

	if (mysql_query(con, query)) 
  	{
      	printf("Can't run the query! Line: %d\n", __LINE__);
      	sendData(clientSocket, "Can't run the query!");
      	free(query);
      	return;
  	}

  	free(query);
  	sendData(clientSocket, "Comment added!");
}

void restrictVote(int clientSocket, MYSQL *con, int userType)
{
	char *username = readData(clientSocket);

	if (userType == 0)
	{
		printf("You don't have enought rights! Line: %d\n", __LINE__);
		sendData(clientSocket, "You don't have enought rights!");
		return;
	}
	else if (userType == 1)
	{
		char *baseQuery = "update users set canVote = 0 where username = \"";
		char queryLength = strlen(baseQuery) + strlen(username) + 5;
		char *query = malloc(sizeof(char) * queryLength);

		strcpy(query, baseQuery);
		strcat(query, username);
		strcat(query, "\";");

		printf("Query: %s\n", query);
		if (mysql_query(con, query)) 
  		{
      		printf("Can remove user right to vote! Line: %d\n", __LINE__);
      		sendData(clientSocket, "Can remove user right to vote!");
      		return;
  		}
	}

	sendData(clientSocket, "Done");
}

void deleteSong(int clientSocket, MYSQL *con, int userType)
{
	char *songName = readData(clientSocket);

	if (userType == 0)
	{
		printf("You don't have enought rights! Line: %d\n", __LINE__);
		sendData(clientSocket, "You don't have enought rights!");
		return;
	}
	else if (userType == 1)
	{

		if (!songExists(con, songName))
		{
			printf("Song not found! Line: %d\n", __LINE__);
			sendData(clientSocket, "Song not found!");
			return;
		}

		char *baseQuery = "delete from songs where name = \"";
		int queryLength = strlen(baseQuery) + strlen(songName) + 10; // 10 pentru "", spatii etc...
		char *query = malloc(sizeof(char) * queryLength);

		strcpy(query, baseQuery);
		strcat(query, songName);
		strcat(query, "\";");

		printf("Query: %s\n",query);

		if (mysql_query(con, query)) 
	  	{
	      	printf("Can't run the query! Line: %d\n", __LINE__);
	      	sendData(clientSocket, "Can't run the query!");
	      		free(query);
	      		return;
	  	}
	}

  	sendData(clientSocket, "Song deleted!");
}

void getComments(int clientSocket, MYSQL *con)
{
	if (mysql_query(con, "select * from comments")) 
  	{
      	printf("Can't run the query! Line: %d\n", __LINE__);
      	sendData(clientSocket, "Can't run the query!");
      	return;
  	}
  
  	MYSQL_RES *result = mysql_store_result(con);
  
  	if (result == NULL)
  	{
     	printf("Can't get the result! Line: %d\n", __LINE__);
     	sendData(clientSocket, "Can't get the result!");
     	return;
  	}

  	MYSQL_ROW row;

  	char *spacer = " --- ";

  	int numberOfFields = mysql_num_rows(result);
  	int fields = 0;

	char stringLines[10];
	sprintf(stringLines, "%d", numberOfFields); // transforma int in char *
  	sendData(clientSocket, stringLines);

  	while ((row = mysql_fetch_row(result)))
  	{
      	char *commentLine = calloc(sizeof(char), strlen(row[1]) + strlen(spacer) + strlen(row[2]) + strlen(spacer) + strlen(row[3]));
      	strcpy(commentLine, row[1]);
      	strcat(commentLine, spacer);
      	strcat(commentLine, row[2]);
      	strcat(commentLine, spacer);
      	strcat(commentLine, row[3]);

      	sendData(clientSocket, commentLine);

      	free(commentLine);

      	fields++;
      	if (fields > numberOfFields)
      	{
      		break;
      	}
  	}

  	sendData(clientSocket, "End");
}

int login(char *username, char *password, MYSQL *con)
{
	if (mysql_query(con, "SELECT * FROM users")) 
  	{
      	printf("Can't run the query! Line: %d\n", __LINE__);
      	return 0;
  	}
  
  	MYSQL_RES *result = mysql_store_result(con);
  
  	if (result == NULL) 
  	{
     	printf("Can't run the query! Line: %d\n", __LINE__);
     	return 0;
  	}

  	MYSQL_ROW row;

  	while ((row = mysql_fetch_row(result))) 
  	{ 
      	if (strcmp(username, row[1]) == 0 && strcmp(password, row[2]) == 0)
      	{
      		return 1 * 100 + atoi(row[3]) * 10 + atoi(row[4]);
      	}
  	}
  	return 0;
}

int registerUser(char *username, char *password, MYSQL *con) // orice user facut cu registerUser va fi user basic, nu admin
{
	if (mysql_query(con, "SELECT * FROM users")) 
  	{
      	printf("Can't run the query! Line: %d\n", __LINE__);
      	return 0;
  	}
  
  	MYSQL_RES *result = mysql_store_result(con);
  
  	if (result == NULL) 
  	{
     	printf("Can't run the query! Line: %d\n", __LINE__);
     	return 0;
  	}

  	MYSQL_ROW row;

  	while ((row = mysql_fetch_row(result))) 
  	{ 
      	if (strcmp(username, row[1]) == 0)
      	{
      		printf("An user with this name already exists! Line: %d\n", __LINE__);
      		return 0;
      	} 
  	}

  	char *queryBasic = "insert into users (username, password) values (\"";
  	int querySize = strlen(queryBasic) + strlen(username) + strlen(password) + 5;
  	char *query = malloc(sizeof(char) * querySize);

  	strcpy(query, queryBasic);
  	strcat(query, username);
  	strcat(query, "\", \"");
  	strcat(query, password);
  	strcat(query, "\");");

  	printf("Query: %s\n", query);

  	if (mysql_query(con, query)) 
  	{
      	printf("Can't register the user! Line: %d\n", __LINE__);
      	return 0;
  	}

  	free(query);

  	return 1;
}