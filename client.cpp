#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h> 
#include <cerrno>
#include <stdlib.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h> 
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <ctime> 

using namespace std;

pthread_mutex_t mutex;

int socket_id = 0; 																			//объявить идентификатор  сокета;
																					
int flag = 0; 																				//{объявить флаг завершения потока установления 																																		соединения;
 																							//{объявить флаг завершения потока передачи запросов;
																							//{объявить флаг завершения потока приема ответов;
																							
pthread_t id_c;																				//объявить идентификатор потока установления 																																			соединения;
pthread_t id_s;																				//объявить идентификатор потока передачи запросов;
pthread_t id_r;																				//объявить идентификатор потока приема ответов;
																							
struct sockaddr_in sock_address;															//структура 
 				
static void* sending(void* arg) 															//функция передачи запросов()
{
	int var;
	while(flag == 0)																		//пока флаг завершения потока передачи запросов не установлен
	{
		sleep(1);
		var = 1 + rand() % (9 - 1);															//создать запрос;
		int err = send(socket_id, &var, sizeof(int), 0);									//передать запрос в сокет;
		cout << "Sending this: " << var;
		if (err == -1)
		{
			perror("Sending problem");
		}
	}
	pthread_exit(NULL);
}

static void* receiving(void* arg)															//функция приема ответов()
{
	usleep(20000);
	while(flag == 0)																		//пока флаг завершения потока приема ответов не установлен
	{
		void *buff = new int;
		sleep(1);
		*(long*)buff = 0;
		int err = recv(socket_id, buff, sizeof(buff), 0);									//принять ответ из сокета;
		if (err == -1)
		{
			perror("Recieving problem");
			//continue;
			usleep(200);
		}
		else cout << "	Recieving: " << *(int*)buff << endl;									//вывести ответ на экран;
		delete((int*)buff);
	}
	pthread_exit(NULL);
}

static void* connection(void* arg)															//функция установления соединения()
{
	while(flag == 0) 																		//пока (флаг завершения потока установления соединения 																																		не установлен) 
	{
		int err = connect(socket_id, (struct sockaddr*)&sock_address, sizeof(sock_address));//установить соединение с сервером;
		if (err != -1)																		//если соединение установлено 
		{
			pthread_create(&id_s, NULL, sending, NULL);										//создать поток передачи запросов;
			pthread_create(&id_r, NULL, receiving, NULL);									//создать поток приема ответов;
			pthread_exit(NULL);
		}
		else 
		{
			if (errno != 115)
			{
				perror ("Connection error");
				//exit(2);
			}
		}
		sleep (2);
	}
}

int main()
{	
	srand(time(0));
	cout << "Starting main programm... Press any key to finish execution" << endl << flush;
	pthread_mutex_init(&mutex, NULL);
	socket_id = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);								//создать сокет для работы с сервером;
	sock_address.sin_family = AF_INET;														//тип протокола - интернет IPV4
	sock_address.sin_port = htons(31337);
	inet_aton("127.0.0.1", &sock_address.sin_addr);
	pthread_create(&id_c, NULL, connection, NULL);											//создать поток установления соединения;
	getchar();																				//ждать нажатия клавиши;
	flag = 1;																				//{установить флаг завершения потока передачи запросов;
																							//{установить флаг завершения потока приема ответов;
																							//{установить флаг завершения потока установления 																																			соединения;


	pthread_join(id_c, NULL);																//ждать завершения потока установления соединения;
	pthread_join(id_s, NULL);																//ждать завершения потока передачи запросов;
	pthread_join(id_r, NULL);																//ждать завершения потока приема ответов;

	shutdown(socket_id, 2);																	//закрыть соединение с сервером;SD_BOTH == 2 Shutdown 																													both send and receive operations.
	
	close(socket_id);																		//{закрыть сокет;
}
