/*
编译gcc server.c - o server -l pthread
运行./server
*/
#include<netinet/in.h>   
#include<sys/types.h>   
#include<sys/socket.h>   
#include<stdio.h>   
#include<stdlib.h>   
#include<string.h>  
#include<arpa/inet.h> 
#include<time.h>
#include <unistd.h>
#include <pthread.h>
#include<stdint.h>

#define HELLO_WORLD_SERVER_PORT    6676 
#define LENGTH_OF_LISTEN_QUEUE     20  
#define BUFFER_SIZE                1024  
#define FILE_NAME_MAX_SIZE         512  

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


typedef struct
{
    char State_B[5];
    int socket;
}func;
int Transport_Flag = 1;

int* thread(void* arg)
{
    printf("yes\n");
    func* r = (func*)arg;
    char buff[5];
    bzero(r->State_B, sizeof(r->State_B));
    while (1)
    {
        if ((recv(r->socket, r->State_B, 5, 0)) > 0)
        {
            //  printf("nice\n");
            if (r->State_B[2] == 'o')//stop
            {
                printf("client want to stop\n");
                Transport_Flag = 2;
            }
            else if (r->State_B[2] == 'n')//contiue
            {
                printf("client want to contune\n");
                Transport_Flag = 3;
            }
            else if (r->State_B[2] == 'd')//end
            {
                printf("client want to end\n");
                Transport_Flag = 0;
            }
        }

    }
    return NULL;
}
int main(int argc, char** argv)
{
    // set socket's address information   
    // ����һ��socket��ַ�ṹserver_addr,����������internet�ĵ�ַ�Ͷ˿�  
    uint32_t count;
    uint32_t crc = 0xFFFFFFFF;
    unsigned char buff[1024];


    make_crc32_table();

    char qurey[BUFFER_SIZE] = { "Do you want to receive file:test.mp4" };
    char buff1[5] = "stop";
    pid_t pid;
    int n, flag = 0, speed, Flag_remain_data, file_size;

    clock_t start, finish;
    float time = 0;
    char Pipe_In[5], Pipe_out[5];
    struct sockaddr_in6  server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(HELLO_WORLD_SERVER_PORT);

    // create a stream socket   
    // ��������internet����Э��(TCP)socket����server_socket������������ͻ����ṩ����Ľӿ�  
    int server_socket = socket(PF_INET6, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        printf("Create Socket Failed!\n");
        exit(1);
    }

    // ��socket��socket��ַ�ṹ��   
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        printf("Server Bind Port: %d Failed!\n", HELLO_WORLD_SERVER_PORT);
        exit(1);
    }

    // server_socket���ڼ���   
    if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE))
    {
        printf("Server Listen Failed!\n");
        exit(1);
    }
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);
    Flag_remain_data = 0;

    int new_server_socket = accept(server_socket, (struct sockaddr*)&client_addr, &length);
    if (new_server_socket < 0)
    {
        printf("Server Accept Failed!\n");
        exit(1);
    }

    // ����ͻ��˵�socket��ַ�ṹclient_addr�����յ����Կͻ��˵�����󣬵���accept  
    // ���ܴ�����ͬʱ��client�˵ĵ�ַ�Ͷ˿ڵ���Ϣд��client_addr��  
   //printf("%s\n",qurey);
    send(new_server_socket, qurey, BUFFER_SIZE, 0);

    char buffer[BUFFER_SIZE];
    int buf[5];
    bzero(buffer, sizeof(buffer));

    //acorrding client to decide whether to tansport
    if ((n = recv(new_server_socket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        if (buffer[0] == 'n')
        {
            printf("Server stop to transport\n");
            exit(1);
        }
        else if (buffer[0] == 'Y')
        {
            printf("Server start to transport\n");
        }
    }

    //decide the speed of transport
    if ((n = recv(new_server_socket, buf, 5, 0)) > 0)
    {
        speed = buf[0];
        printf("%d\n", buf[0]);
    }

    length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
    if (length < 0)
    {
        printf("Server Recieve Data Failed!\n");
        //break;  
        exit(1);
    }

    pthread_t thid;
    func r = { "OK",new_server_socket };
    if (n = pthread_create(&thid, NULL, (void*)thread, (void*)&r) != 0) {
        printf("thread creation failed\n");
        exit(1);
    }

    char file_name[FILE_NAME_MAX_SIZE + 1];
    bzero(file_name, sizeof(file_name));
    strncpy(file_name, buffer,
        strlen(buffer) > FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE : strlen(buffer));
    FILE* fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        printf("File:\t%s Not Found!\n", file_name);
    }
    else
    {   
	while (!feof(fp))
		{
			memset(buff, 0, sizeof(buff));
			count = fread(buff, 1, sizeof(buff), fp);
			crc = make_crc(crc, buff, count);
		}
	printf("In main: calculate crc is 0x%x\n", crc);
        bzero(buffer, BUFFER_SIZE);
        int file_block_length = 0;

        fseek(fp, 0, SEEK_END);//���ļ�λ��ָ��bai�����ļ���β
        file_size = ftell(fp) / 1024;//�õ���ǰλ�����ļ���ʼλ�õ��ֽ�ƫ������
        printf("size of file is %d Mb\n", file_size / 1000);
        bzero(qurey, BUFFER_SIZE);
        sprintf(qurey, "%d", file_size / 1000);
        send(new_server_socket, qurey, BUFFER_SIZE, 0);
        fseek(fp, 0, SEEK_SET);

        while ((file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0 && (Transport_Flag != 0))//�ļ�����
        {
            start = clock();
            while (Transport_Flag == 2)//stop
            {
                sleep(1);
            }
            if (send(new_server_socket, buffer, file_block_length, 0) < 0)
            {
                printf("Send File:\t%s Failed!\n", file_name);
                break;
            }
            finish = clock();
            time = time + (double)(finish - start) / CLOCKS_PER_SEC;
            flag++;
            if (time < 1.0 && flag == 1000 * speed)//speed control
            {
                flag = (int)(1000000 - time * 1000000);
                Flag_remain_data++;
                printf(" tranport speed is %d Mb/s  ", speed);
                float remaining_data = Flag_remain_data * speed;
                float pre = (float)Flag_remain_data * speed / file_size * 1000.0;
                float left_time = (file_size / 1000 - Flag_remain_data * speed) / speed * 1.0;
                printf("the data of file has transport %d MB,percent is %f,the time need %f s\n", Flag_remain_data * speed,
                    pre, left_time);
                if (flag < 0) continue;
                usleep(flag);
                flag = 0;
                time = 0;
            }
            bzero(buffer, sizeof(buffer));
        }
        fclose(fp);
        printf("File:\t%s Transfer Finished!\n", file_name);
    }
    close(new_server_socket);
    close(server_socket);
    return 0;
}
