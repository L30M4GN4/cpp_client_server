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
#include <queue>
#include <netinet/in.h>
#include <arpa/inet.h> 



using namespace std;

pthread_mutex_t rq_mutex;
pthread_mutex_t wq_mutex;

struct args {

	queue<int> mes_recv_queue;																						//{объявить идентификатор очереди запросов на 																																							обработку;
																													//{создать очередь запросов на обработку;
																							
	queue<int> mes_send_queue; 																						//{объявить идентификатор очереди ответов на 																																							передачу;
																													//{создать очередь ответов на передачу;
	int working_socket_id = 0;																						//объявить идентификатор сокета для работы с 																																							клиентом;	
																			
};

int listening_socket_id = 0; 																						//объявить идентификатор «слушающего» сокета;

int flag = 0; 																										//{объявить флаг завершения потока ожидания 																																						соединения;
 																													//{объявить флаг завершения потока передачи 																																							ответов;
																													//{объявить флаг завершения потока приема 																																								запросов;
															 														//{объявить флаг завершения потока обработки 																																								запросов;
															 								
pthread_t id_r;																										//объявить идентификатор потока приема запросов;
pthread_t id_p;																										//объявить идентификатор потока обработки 																																								запросов;
pthread_t id_s;																										//объявить идентификатор потока передачи ответов;
pthread_t id_c;																										//объявить идентификатор потока ожидания 																																							соединений;

struct sockaddr_in sock_address;																					//структура 
struct sockaddr_in client_address;

args* arguments = new args;
 						
static void* receiving(void* arg)																					//функция потока()
{
	args* arguments = (args*)arg;
	while(flag == 0)																								//пока (флаг завершения потока не установлен)
	{
		void *buff = new int;	
		*(int*)buff = 0;
		int err = 0;
		if (buff)  err = recv(arguments->working_socket_id, buff, sizeof(int), 0);											//принять запрос из сокета;
		if (err == -1)
		{
			perror("Recieving problem");
		}
		if (err != 0)
		{
			cout << "Accepted this: " << *(int*)buff;
//			/*	For using NetCat uncomment code below 	*/
//			char* tmp = new char;
//			if (err == 0) continue;
//			for (int i = 0; i < err; i++)
//			{
//				tmp[i] =  *(char*)(buff + i);
//			}
//			int recv_data_toInt = atoi(tmp);
//			cout << "Accepted string: " << recv_data_toInt << endl;
//			arguments->mes_recv_queue.push(recv_data_toInt);															
//			delete (tmp);
//			/*		And comment next line				*/ 
			pthread_mutex_lock(&rq_mutex);
			arguments->mes_recv_queue.push(*(int*)buff);																//положить запрос в очередь на обработку;
			pthread_mutex_unlock(&rq_mutex);
		}
		usleep(2000);
		if ((int*)buff) delete ((int*)buff);
	}
}

static void* processing(void* arg)																					//функция потока()
{
	args* arguments = (args*)arg;
	int var = 0;
	int result = 0;
	while(flag == 0)																								//пока (флаг завершения потока не установлен)
	{
		pthread_mutex_lock(&rq_mutex);
		if (!arguments->mes_recv_queue.empty())																		//если очередь не пуста:
		{
			var = arguments->mes_recv_queue.front(); 																//считать первый элемент из очереди
			arguments->mes_recv_queue.pop();
			pthread_mutex_unlock(&rq_mutex);																		//удалить прочитанный элемент из очереди
			result = sysconf(var);																					//обработать запрос и сформировать ответ;
			//cout << "	result is: " << result 	<< endl;															//вывести на экран результат обработки
			//getaddrinfo - возвращает одну или несколько структур
			pthread_mutex_lock(&wq_mutex);
			arguments->mes_send_queue.push(result);																	//положить ответ в очередь на передачу;
			pthread_mutex_unlock(&wq_mutex);
		}
		else 
		{
			pthread_mutex_unlock(&rq_mutex);
			usleep(2000);
		}
	}
	pthread_exit(NULL);
}

static void* sending(void* arg)																						//функция потока()
{	
	args* arguments = (args*)arg;
	int msg = 0;
	while(flag == 0)																								//пока (флаг завершения потока не установлен)
	{	
		pthread_mutex_lock(&wq_mutex);
		if (!arguments->mes_send_queue.empty())																		//если очередь не пуста:
		{
			msg = arguments->mes_send_queue.front();																//прочитать запрос из очереди на передачу;
			cout << "	MSG IS: " << msg << endl;
			arguments->mes_send_queue.pop();																		//удалить первый элемент очереди
			pthread_mutex_unlock(&wq_mutex);
			int err = 0;
			if (&msg) err = send(arguments->working_socket_id, &msg, sizeof(int), 0); 									//передать ответ в сокет;
			if (err == -1)
			{
				perror("Sending problem");
			}
			msg = 0;
		}
		else 
		{
			pthread_mutex_unlock(&wq_mutex);
			usleep(2000);
		}
	}
	pthread_exit(NULL);
}

static void* waiting_for_connection(void* arg)																		//функция потока()
{
	args* arguments = (args*)arg;
	while(flag == 0)																								//пока (флаг завершения потока не установлен)
	{
		socklen_t c_len = sizeof(client_address);
		arguments->working_socket_id = accept(listening_socket_id, (struct sockaddr*)&client_address, &c_len);		//прием соединения от клиента;
		/*______EXPERIMENTAL !!!_____*/
//		const void* msg = new char;
//		(const char*)msg = "Write some number please!";
//		send(listening_socket_id, msg, sizeof(long), MSG_DONTWAIT);
		/*__________________________*/

		if (arguments->working_socket_id != -1)
		{
			cout << "Addres is: " << inet_ntoa(sock_address.sin_addr) << endl << flush;
			pthread_create(&id_r, NULL, receiving, arguments);														//создать поток приема запросов;
			pthread_create(&id_p, NULL, processing, arguments);														//создать поток обработки запросов;
			pthread_create(&id_s, NULL, sending, arguments);														//создать поток передачи ответов;
			pthread_exit(NULL);
		}
		else 
		{
			if (errno != 11)
			{ 
				perror ("Connection error");
			}
		}
		usleep(2000);
	}
	
}

int main()
{
	pthread_mutex_init(&rq_mutex, NULL);
	pthread_mutex_init(&wq_mutex, NULL);
	cout << "Starting listening... Press any key to finish execution" << endl << flush;
	sock_address.sin_family = AF_INET;																				//тип протокола - интернет IPV4
	sock_address.sin_addr.s_addr = INADDR_ANY;			
	sock_address.sin_port = htons(31337);
	listening_socket_id = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);											//создать «слушающий» сокет;
	int optval = 1;
	setsockopt(listening_socket_id, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));																			
	if (bind(listening_socket_id, (struct sockaddr*)&sock_address, sizeof(sock_address)) == -1)						//привязать «слушающий» сокет к адресу;
		{
			perror("Bind error");
			exit(1);
		}
	if (listen(listening_socket_id, 3) == -1) perror("Listen error");												//перевести сокет в состояние прослушивания 																																						(max-3 peers);
	pthread_create(&id_c, NULL, waiting_for_connection, arguments);													//создать поток ожидания соединений;
	getchar();																										//ждать нажатия клавиши;
	
	flag = 1;																										//установить флаг завершения потока приема 																																							запросов;
																													//установить флаг завершения потока обработки 																																							запросов;
																													//установить флаг завершения потока передачи 																																							ответов;
																													//установить флаг завершения потока ожидания 																																			соединений;
																					
	pthread_join(id_r, NULL);																						//ждать завершения потока приема запросов;
	pthread_join(id_p, NULL);																						//ждать завершения потока обработки запросов;
	pthread_join(id_s, NULL);																						//ждать завершения потока передачи ответов;
	pthread_join(id_c, NULL);																						//ждать завершения потока ожидания соединений;
	
	cout << "Threads are closed, execution finished" << endl;
	shutdown(arguments->working_socket_id, 2);																		//закрыть соединение с клиентом;SD_BOTH == 2 																															Shutdown both send and receive operations.
	
	close(listening_socket_id);																						//{закрыть сокет для работы с клиентом;
	close(arguments->working_socket_id);																			//{закрыть «слушающий» сокет;
	delete(arguments);	
}
