#define h_addr h_addr_list[0]
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#define PORT 3000
#define HOST "localhost"

void sendData(int clientSocket, char *message);
char* readData(int clientSocket);

int main()
{
	printf("Starting...\n");
	struct sockaddr_in serverAddress;
	struct hostent *host;

	if (!(host = gethostbyname(HOST)))
	{
		printf("Can't get host address! Line: %d\n", __LINE__);
		return 1;
	}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr = *((struct in_addr*)host->h_addr);

	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (clientSocket < 0)
	{
		printf("Failed to create client socket! Line: %d\n", __LINE__);
	}

	if (connect(clientSocket, (struct sockaddr *) &serverAddress, sizeof(struct sockaddr)) == -1)
	{
		printf("Can't connect to the server! Line: %d\n", __LINE__);
		return 1;
	}

	int authResponse = -1;

	char action[50];
	char username[50];
	char password[50];

	printf("Enter action: ");
	scanf ("%s", action);

	printf("Enter username: ");
	scanf ("%s", username);

	printf("Enter password: ");
	scanf ("%s", password);

	sendData(clientSocket, action);
	sendData(clientSocket, username);
	sendData(clientSocket, password);

	read(clientSocket, &authResponse, sizeof(int));
	
	if (authResponse != 0)
	{
		printf("Connected!\n");
	}
	else if (authResponse == 0)
	{
		printf("Authentication failed!\n");
		return 1;
	}
	else
	{
		printf("Error at login!\n");
		return 1;
	}

	char command[300];

	while(1)
	{
		printf("Enter command: ");
		scanf ("%s", command);

		sendData(clientSocket, command);

		if (strcmp(command, "addSong") == 0)
		{
			size_t nameSize = 50;
			char *name = (char*)malloc(sizeof(char) * nameSize);

			size_t descriptionSize = 50;
			char *description = (char*)malloc(sizeof(char) * descriptionSize);

			char numberOfGenres[5];

			size_t linkSize = 50;
			char *link = (char*)malloc(sizeof(char) * linkSize);

			printf("Enter name: ");
			while (getline (&name, &nameSize, stdin) < 2) {}

			printf("Enter description: ");
			while (getline (&description, &descriptionSize, stdin) < 2) {}

			printf("Enter link: ");
			while (getline (&link, &linkSize, stdin) < 2) {}

			printf("Enter numberOfGenres: ");
			scanf ("%s", numberOfGenres);

			int genres = atoi(numberOfGenres);

			for (int i = strlen(name); i > 0; i--)
			{
				if (name[i] == '\n') // scoatem new line de la capat
				{
					name[i] = '\0';
					break;
				}
			}

			for (int i = strlen(description); i > 0; i--)
			{
				if (description[i] == '\n')
				{
					description[i] = '\0';
					break;
				}
			}

			for (int i = strlen(link); i > 0; i--)
			{
				if (link[i] == '\n') // scoatem new line de la capat
				{
					link[i] = '\0';
					break;
				}
			}

			while (genres == 0)
			{
				printf("Invalid number of genres!\n");
				printf("Enter numberOfGenres: ");
				scanf ("%s", numberOfGenres);
				genres = atoi(numberOfGenres);
			}

			sendData(clientSocket, name);
			sendData(clientSocket, description);
			sendData(clientSocket, link);
			sendData(clientSocket, numberOfGenres);

			for (int i = 0; i < genres; i++)
			{
				char genreName[100];
				printf("Enter genre name: ");
				scanf ("%s", genreName);
				sendData(clientSocket, genreName);
			}

			free(name);
			free(description);
			free(link);
		}
		else if (strcmp(command, "voteSong") == 0)
		{
			char songName[50];
			char genre[50];
			char vote[3];

			printf("Enter the song name: ");
			scanf ("%s", songName);

			printf("Enter the song genre: ");
			scanf ("%s", genre);

			printf("Enter the vote (1-10): ");
			scanf ("%s", vote);

			while (strcmp(vote, "0") == 0)
			{
				printf("Give a valid vote (1-10): ");
				scanf ("%s", vote);
			}

			sendData(clientSocket, songName);
			sendData(clientSocket, genre);
			sendData(clientSocket, vote);
		}
		else if(strcmp(command, "generalTop") == 0)
		{
			char *stringLines = readData(clientSocket);
			int lines = atoi(stringLines);
			printf("Lines: %s\n", stringLines);

			for (int i = 0; i < lines; i++)
			{
				char *generalTopLine = readData(clientSocket);
				printf("%s\n", generalTopLine);
			}
		}
		else if(strcmp(command, "genreTop") == 0)
		{
			char genre[50];

			printf("Enter the genre: ");
			scanf ("%s", genre);

			sendData(clientSocket, genre);

			int lines = atoi(readData(clientSocket));
			for (int i = 0; i < lines; i++)
			{
				char *genreTopLine = readData(clientSocket);
				printf("%s\n", genreTopLine);
			}
		}
		else if(strcmp(command, "comment") == 0)
		{
			size_t songSize = 50;
			char *songName = (char*)malloc(sizeof(char) * songSize);
			size_t commentSize = 300;
			char *comment = (char*) malloc(sizeof(char) * commentSize);

			printf("Enter the song name: ");
			while (getline (&songName, &songSize, stdin) < 2) {}

			printf("Enter the comment: ");
			while (getline (&comment, &commentSize, stdin) < 2) {} // Am folosit aici getline deoarece poate citii si spatii, scanf se opreste cand da de un spatiu.

			for (int i = strlen(songName); i > 0; i--)
			{
				if (songName[i] == '\n') // scoatem new line de la capat
				{
					songName[i] = '\0';
					break;
				}
			}

			for (int i = strlen(comment); i > 0; i--)
			{
				if (comment[i] == '\n') // scoatem new line de la capat
				{
					comment[i] = '\0';
					break;
				}
			}

			sendData(clientSocket, songName);
			sendData(clientSocket, comment);

			free(songName);
			free(comment);
		}
		else if (strcmp(command, "restrictVote") == 0)
		{
			size_t usernameSize = 50;
			char *username = (char*)malloc(sizeof(char) * usernameSize);

			printf("Enter username: ");
			while (getline (&username, &usernameSize, stdin) < 2) {} 


			for (int i = strlen(username); i > 0; i--)
			{
				if (username[i] == '\n')
				{
					username[i] = '\0';
					break;
				}
			}	

			sendData(clientSocket, username);
		}
		else if (strcmp(command, "deleteSong") == 0)
		{
			size_t songNameSize = 50;
			char *songName = (char*)malloc(sizeof(char) * songNameSize);

			printf("Enter song name: ");
			while (getline (&songName, &songNameSize, stdin) < 2) {} 

			for (int i = strlen(songName); i > 0; i--)
			{
				if (songName[i] == '\n')
				{
					songName[i] = '\0';
					break;
				}
			}	

			sendData(clientSocket, songName);
		}
		else if (strcmp(command, "getComments") == 0)
		{
			printf("Comments:\n");

			int lines = atoi(readData(clientSocket));
			for (int i = 0; i < lines; i++)
			{
				char *commentLine = readData(clientSocket);
				printf("%s\n", commentLine);
				free(commentLine);
			}
		}
		else if (strcmp(command, "register") == 0)
		{
			char username[50];
			char password[50];

			printf("Enter username: ");
			scanf ("%s", username);

			printf("Enter password: ");
			scanf ("%s", password);

			sendData(clientSocket, username);
			sendData(clientSocket, password);
		}

		printf("%s\n", readData(clientSocket));
	}

	return 0;
}

void sendData(int clientSocket, char *message)
{
	int messageLength = strlen(message);
	write(clientSocket, &messageLength, sizeof(int));
	write(clientSocket, message, sizeof(char) * messageLength);
}

char* readData(int clientSocket)
{
	int dataSize = 0;
	read(clientSocket, &dataSize, sizeof(int));

	char *data = malloc(sizeof (char) * dataSize);
	read(clientSocket, data, sizeof(char) * dataSize);

	return data;
}
