/*
* 编译：gcc client.c -o client
* 运行: ./client  IPV6Address
* 运行过程输入stop,cont,end确定传输状态
* 基本思路：一个进程用来在传输文件的过程中读键盘中数值，一个进程用来收发文件
*/
#include<netinet/in.h>                         // for sockaddr_in  
#include<sys/types.h>                          // for socket  
#include<sys/socket.h>                         // for socket  
#include<arpa/inet.h>
#include<stdio.h>                              // for printf  
#include<stdlib.h>                             // for exit  
#include<string.h>                             // for bzero  
#include <unistd.h> 
#include <sys/time.h>
#include<stdint.h>

#define HELLO_WORLD_SERVER_PORT       6676  
#define BUFFER_SIZE                   1024  
#define FILE_NAME_MAX_SIZE            512  


uint32_t crc32_table[256];

int make_crc32_table()
{
	uint32_t c;
	int i = 0;
	int bit = 0;

	for (i = 0; i < 256; i++)
	{
		c = (uint32_t)i;

		for (bit = 0; bit < 8; bit++)
		{
			if (c & 1)
			{
				c = (c >> 1) ^ (0xEDB88320);
			}
			else
			{
				c = c >> 1;
			}

		}
		crc32_table[i] = c;
	}


}

uint32_t make_crc(uint32_t crc, unsigned char* string, uint32_t size)
{

	while (size--)
		crc = (crc >> 8) ^ (crc32_table[(crc ^ *string++) & 0xff]);

	return crc;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Usage: ./%s ServerIPAddress\n", argv[0]);
		exit(1);
	}
	
	char receive[BUFFER_SIZE], buffer[BUFFER_SIZE];
	int n;
	int speed;
	int buff[5];
	pid_t pid;
	
	
	// 设置一个socket地址结构client_addr, 代表客户机的internet地址和端口  
	struct sockaddr_in6 client_addr;
	bzero(&client_addr, sizeof(client_addr));
	client_addr.sin6_family = AF_INET6; // internet协议族  
	client_addr.sin6_addr = in6addr_any; // INADDR_ANY表示自动获取本机地址  
	client_addr.sin6_port = htons(0); // auto allocated, 让系统自动分配一个空闲端口  

	// 创建用于internet的流协议(TCP)类型socket，用client_socket代表客户端socket  
	int client_socket = socket(AF_INET6, SOCK_STREAM, 0);
	if (client_socket < 0)
	{
		printf("Create Socket Failed!\n");
		exit(1);
	}

	// 把客户端的socket和客户端的socket地址结构绑定   
	if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)))
	{
		printf("Client Bind Port Failed!\n");
		exit(1);
	}

	// 设置一个socket地址结构server_addr,代表服务器的internet地址和端口  
	struct sockaddr_in6  server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;

	// 服务器的IP地址来自程序的参数   
	/*if (inet_aton(argv[1], &server_addr.sin6_addr) == 0)
	{
		printf("Server IP Address Error!\n");
		exit(1);
	}*/
	if (inet_pton(AF_INET6, argv[1], &server_addr.sin6_addr) <= 0) //将字符串的ipv6地址转换成二进制格式存到servaddr.sin6_addr所指示的地方
		perror("inet_pton error");

	server_addr.sin6_port = htons(HELLO_WORLD_SERVER_PORT);
	socklen_t server_addr_length = sizeof(server_addr);

	// 向服务器发起连接请求，连接成功后client_socket代表客户端和服务器端的一个socket连接  
	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0)
	{
		printf("Can Not Connect To %s!\n", argv[1]);
		exit(1);
	}

	if ((n = read(client_socket, receive, BUFFER_SIZE)) > 0)
	{
		receive[n] = 0;
		if (fputs(receive, stdout) == EOF)
			perror("fputs error");
		printf("\ninput Y/n to decide:  ");
		bzero(receive, sizeof(receive));
		//scanf("%s",receive);
		fgets(receive, sizeof(receive), stdin);
		if (receive[0] == 'Y')
		{
			send(client_socket, receive, BUFFER_SIZE, 0);
			printf("client accepts to receive.\n");

			printf("client input the speed of transport,unit is Mb/s :\t");
			bzero(receive, sizeof(receive));
			scanf("%d", &buff[0]);
			speed = buff[0];
			send(client_socket, buff, 5, 0);
		}
		else
		{
			printf("Client reject to receive\n");
			exit(1);
		}
	}
	char file_name[FILE_NAME_MAX_SIZE + 1];
	bzero(file_name, sizeof(file_name));
	printf("Please Input File Name:\t");
	scanf("%s", file_name);

	bzero(buffer, sizeof(buffer));
	strncpy(buffer, file_name, strlen(file_name) > BUFFER_SIZE ? BUFFER_SIZE : strlen(file_name));
	// 向服务器发送buffer中的数据，此时buffer中存放的是客户端需要接收的文件的名字  
	send(client_socket, buffer, BUFFER_SIZE, 0);
	int File_Size;
	if ((n = read(client_socket, receive, BUFFER_SIZE)) > 0)
	{
		File_Size = (receive[0] - '0') * 10 + receive[1] - '0';//简单记录一下文件大小，小于100M
	}
	FILE* fp = fopen(file_name, "w");
	if (fp == NULL)
	{
		printf("File:\t%s Can Not Open To Write!\n", file_name);
		exit(1);
	}
	/*创建一个新的子进程*/
	pid = fork();
	if (pid == -1)
	{
		printf("fork error");
		exit(EXIT_FAILURE);
	}
	if (pid == 0)
	{
		// 从服务器端接收数据到buffer中   
		bzero(buffer, sizeof(buffer));
		int length = 0;
		int Transport_Cnt_flag = 0, flag = 0;
		struct timeval tv_begin, tv_end;
		int i;
		FILE* sp = NULL;
		char ch;
		uint32_t count;
		uint32_t crc = 0xFFFFFFFF;
		unsigned char buf1[1024];
		FILE* dp = NULL;
		make_crc32_table();
		while (length = recv(client_socket, buffer, BUFFER_SIZE, 0))
		{
			Transport_Cnt_flag++;
			gettimeofday(&tv_begin, NULL);
			if (length < 0)
			{
				printf("Recieve Data From Server %s Failed!\n", argv[1]);
				break;
			}

			int write_length = fwrite(buffer, sizeof(char), length, fp);
			if (write_length < length)
			{
				printf("File:\t%s Write Failed!\n", file_name);
				break;
			}
			bzero(buffer, BUFFER_SIZE);
			if (Transport_Cnt_flag - flag > 10000)
			{
				printf(" tranport speed is %d Mb/s  ", speed);
				printf("The data of file has tansport %f MB,percent is %f,the time need %d s\n", Transport_Cnt_flag / 1000.0, Transport_Cnt_flag / 1000.0 / File_Size,
					(File_Size - Transport_Cnt_flag / 1000) / speed);
				flag = Transport_Cnt_flag;
			}
		}
		printf("Recieve File:\t %s From Server[%s] Finished!\n", file_name, argv[1]);
		// 传输完毕，关闭socket 
	
	fclose(fp);
	
	
	sp = fopen(file_name, "rb");
		while (!feof(sp))
		{
			memset(buf1, 0, sizeof(buf1));
			count = fread(buf1, 1, sizeof(buf1), sp);
			crc = make_crc(crc, buf1, count);
		}
	
	printf("In main: calculate crc is 0x%x\n", crc);
	fclose(sp);
		exit(EXIT_SUCCESS);/*成功退出子进程*/
	}
	else
	{
		char send_buf[5] = { "ok" };
		bzero(send_buf, sizeof(send_buf));
		/*如果客户端Ctrl+C结束通信进程，fgets获取的就是NULL，或者判断输入为END，否则就进入循环正常发送数据*/
		while ((fgets(send_buf, sizeof(send_buf), stdin) != NULL)) {
			send(client_socket, send_buf, sizeof(send_buf), 0);
			bzero(send_buf, sizeof(send_buf));/*发送完成清空发送缓冲区*/
		}
		exit(EXIT_SUCCESS);
	}

	
	
	
	
	close(client_socket);
	return 0;
}
