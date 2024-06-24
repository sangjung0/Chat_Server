#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>

#include "common_constants.h"
#include "s_util.h"
#include "message.h"

void * recv_msg(void * arg);
int get_sock(char* name, int len);
int get_room_id(char* name, int len);

static Net_Obj net_obj;	
static int max_name_length = MAX_NAME_LENGTH + 1;
static char users[MAX_CLNT_SIZE][MAX_NAME_LENGTH + 1];
static Bool room_users[MAX_CLNT_SIZE];
static char rooms[MAX_ROOM_SIZE][MAX_ROOM_NAME+1];
static char name[MAX_NAME_LENGTH + 1];
static int invite_room = -1;
static int cnt_room = 0;
static Bool wait_res = False;
static Message_Type last_req;
static int clnt_id = -1;
	
int main(int argc, char *argv[])
{
	pthread_t rcv_thread;
    Message msg;
	int sock, name_len, idx, dest;

	sleep(2);

	if(argc!=3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock=net_obj.sock=socket(PF_INET, SOCK_STREAM, 0);
	setup_address(&net_obj, argv[1], atoi(argv[2]));
	if(connect(sock, &net_obj.adr, sizeof(net_obj.adr)) == -1)
		error_handling("connect() error");


	/* 이름 설정 */
	name_len = 0;
	while (name_len <= 1)
	{
		while (name_len <= 1)
		{
			printf("Enter your name: ");
			fgets(msg.message, max_name_length, stdin);
			name_len = remove_ln(msg.message, max_name_length);
		}
		write(sock, &msg, sizeof(msg));
		read(sock, &msg, sizeof(msg));
		if (msg.m_type == Faillure_Msg) name_len = 0;
		else clnt_id = msg.transceiver;
	}
	string_copy(msg.message, name, name_len);
	
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
    pthread_detach(rcv_thread);

	/* 유저 정보 요청 */
	wait_res = True;
	build_all_clnt_data_msg(&msg);
	write(sock, &msg, sizeof(msg));
	while(wait_res) sleep(0.1);

	/* 방 정보 요청 */
	wait_res = True;
	build_all_room_data_msg(&msg);
	write(sock, &msg, sizeof(msg));
	while(wait_res) sleep(0.1);
	
	/* 로비 입장 */
	cnt_room = 0;
	build_join_room_msg(&msg, cnt_room);
	last_req = msg.m_type;
	write(sock, &msg, sizeof(msg));

    while(1) {
        fgets(msg.message, MAX_MESSAGE_LENGTH, stdin);
		if (wait_res)
		{
			while(wait_res) sleep(0.1);
			continue;
		}

		if (invite_room >= 0)
		{
			name_len = remove_ln(msg.message, 2);
			if (msg.message[0] == 'y' && name_len == 2)
			{
				wait_res = True;
				build_join_room_msg(&msg, invite_room);
				invite_room = -1;
				last_req = msg.m_type;
        		write(sock, &msg, sizeof(msg));
			}
			continue;
		}
		
		switch(get_message_type_from_str(msg.message))
		{
			case Quit_msg:
				build_quit_msg(&msg);
				write(sock, &msg, sizeof(msg));
				close(sock);  
				return 0;
			case Invite_Room_Msg:
				idx = space_token(msg.message, 0, MAX_MESSAGE_LENGTH)+1;
				name_len = remove_ln(&msg.message[idx], MAX_MESSAGE_LENGTH);
				dest = get_sock(&msg.message[idx], name_len);
				if (dest == clnt_id || dest == 0) break;
				memcpy(msg.message, &cnt_room, sizeof(int));
				build_invite_room_msg(&msg, dest);
				break;
			case Private_Msg:
				idx = space_token(msg.message, 0, MAX_MESSAGE_LENGTH);
				msg.message[idx] = 0;
				dest = get_sock(&msg.message[1], idx);
				if (dest == clnt_id) break;
				slide_str(msg.message, idx+1, MAX_MESSAGE_LENGTH);
				build_private_msg(&msg, dest);
				break;
			case Broadcast_Msg:
				idx = space_token(msg.message, 0, MAX_MESSAGE_LENGTH);
				slide_str(msg.message, idx+1, MAX_MESSAGE_LENGTH);
				build_broadcast_msg(&msg);
				break;
			case Make_Room_Msg:
				wait_res = True;

				idx = space_token(msg.message, 0, MAX_MESSAGE_LENGTH);
				slide_str(msg.message, idx+1, MAX_MESSAGE_LENGTH);
				remove_ln(msg.message, MAX_MESSAGE_LENGTH);
				build_make_room_msg(&msg);
				break;
			case Remove_Room_Msg:
				wait_res = True;

				build_remove_room_msg(&msg);
				break;
			case Join_Room_Msg:
				wait_res = True;

				idx = space_token(msg.message, 0, MAX_MESSAGE_LENGTH)+1;
				name_len = remove_ln(&msg.message[idx], MAX_MESSAGE_LENGTH);
				dest = get_room_id(&msg.message[idx], name_len);
				build_join_room_msg(&msg, dest);
				break;
			case Expulsion_Room_Msg:
				idx = space_token(msg.message, 0, MAX_MESSAGE_LENGTH)+1;
				name_len = remove_ln(&msg.message[idx], MAX_MESSAGE_LENGTH);
				dest = get_sock(&msg.message[idx], name_len);
				build_expulsion_room_msg(&msg, dest);
				break;
			case Exit_Room_Msg:
				wait_res = True;

				build_exit_room_msg(&msg);
				break;
			case All_Clnt_Data_Msg:
				build_all_clnt_data_msg(&msg);
				break;
			case All_Clnt_In_Room_Data_Msg:
				build_all_clnt_in_room_data_msg(&msg);
				break;
			case All_Room_Data_Msg:
				build_all_room_data_msg(&msg);
				break;
			case Common_Msg:
				remove_ln(msg.message, MAX_MESSAGE_LENGTH);
				build_common_msg(&msg);
				break;
			default:
				continue;
		}
		last_req = msg.m_type;
        write(sock, &msg, sizeof(msg));
    }

	close(sock);  
	return 0;
}

void * recv_msg(void * arg)   // read thread main
{
	int sock=*((int*)arg);
	Message msg;
	int len, idx, name_len;

	while(read(sock, &msg, sizeof(msg)))
	{
		switch(msg.m_type)
		{
		case Common_Msg:
			printf("[%s] %s\n", users[msg.transceiver], msg.message);
			break;
		case Invite_Room_Msg:
			memcpy(&invite_room, msg.message, sizeof(int));
			printf("[%s -> %s Invite] %s (y/n)\n", users[msg.transceiver], name, rooms[invite_room]);
			break;
		case Private_Msg:
			printf("[%s -> %s] %s\n", users[msg.transceiver], name, msg.message);
			break;
		case Broadcast_Msg:
			printf("[%s Broadcast] %s\n", users[msg.transceiver], msg.message);
			break;
		case Make_Room_Msg:
			string_copy(msg.message, rooms[msg.transceiver], MAX_ROOM_NAME + 1);
			break;
		case Remove_Room_Msg:
			rooms[msg.transceiver][0] = 0;
			if (cnt_room == msg.transceiver)
			{
				wait_res = True;
				cnt_room = 0;
				build_join_room_msg(&msg, cnt_room);
				last_req = msg.m_type;
				write(sock, &msg, sizeof(msg));
			}
			break;
		case Join_Room_Msg:
			room_users[msg.transceiver] = True;
			printf("[System] %s is join\n", users[msg.transceiver]);
			break;
		case Expulsion_Room_Msg:
			room_users[msg.transceiver] = False;
			if (msg.transceiver == clnt_id)
			{
				printf("[System] Explusion you\n");
				wait_res = True;
				cnt_room = 0;
				build_join_room_msg(&msg, cnt_room);
				last_req = msg.m_type;
				write(sock, &msg, sizeof(msg));
			}else{
				printf("[System] Explusion %s\n", users[msg.transceiver]);
			}
			break;
		case Exit_Room_Msg:
			room_users[msg.transceiver] = False;
			printf("[System] Exit %s\n", users[msg.transceiver]);
			break;
		case All_Clnt_Data_Msg:
			len = 0;
            while (len < msg.transceiver)
			{
                memcpy(&idx, &msg.message[len], sizeof(int));
                len += sizeof(int);
                memcpy(&name_len, &msg.message[len], sizeof(int));
                len += sizeof(int);
                memcpy(users[idx], &msg.message[len], name_len);
                len += name_len;
				if (last_req == All_Clnt_Data_Msg)
					printf("%s\n", users[idx]);
            }
			wait_res = False;
			break;
		case All_Room_Data_Msg:
			len = 0;
            while (len < msg.transceiver)
			{
                memcpy(&idx, &msg.message[len], sizeof(int));
                len += sizeof(int);
                memcpy(&name_len, &msg.message[len], sizeof(int));
                len += sizeof(int);
                memcpy(rooms[idx], &msg.message[len], name_len);
                len += name_len;

				if (last_req == All_Room_Data_Msg)
					printf("%s\n", rooms[idx]);
            }
			wait_res = False;
			break;
		case All_Clnt_In_Room_Data_Msg:
			for(len = 0; len < MAX_CLNT_SIZE; len++) room_users[len] = False;
			len = 0;
            while (len < msg.transceiver)
			{
                memcpy(&idx, &msg.message[len], sizeof(int));
                len += sizeof(int);

				room_users[idx] = True;
				if (last_req == All_Clnt_In_Room_Data_Msg)
					printf("%s\n", users[idx]);
            }
			wait_res = False;
			break;
		case Success_Msg:
			switch(last_req)
			{
				case Make_Room_Msg:
					cnt_room = msg.transceiver;
					string_copy(msg.message, rooms[cnt_room], MAX_ROOM_NAME + 1);
					printf("[System] %s created successfully.\n", rooms[cnt_room]);
					wait_res = False;
					break;
				case Remove_Room_Msg:
					printf("[System] %s removed successfully.\n", rooms[msg.transceiver]);
					rooms[msg.transceiver][0] = 0;
					build_join_room_msg(&msg, 0);
					last_req = msg.m_type;
					write(sock, &msg, sizeof(msg));
					break;
				case Join_Room_Msg:
					cnt_room = msg.transceiver;
					printf("[System] Joined %s\n", rooms[cnt_room]);
					build_all_clnt_in_room_data_msg(&msg);
					write(sock, &msg, sizeof(msg));
					break;
				case Exit_Room_Msg:
					printf("[System] Exited room %s\n", rooms[cnt_room]);
					cnt_room = 0;
					build_join_room_msg(&msg, cnt_room);
					last_req = msg.m_type;
					write(sock, &msg, sizeof(msg));
					break;
				case Expulsion_Room_Msg:
					room_users[msg.transceiver] = False;
					printf("[System] Explusion %s\n", users[msg.transceiver]);
				default:
					break;
			}
			break;
		case Faillure_Msg:
			if (wait_res)
			{
				wait_res = False;
				printf("[System] Request failed.\n");
			}
			break;
		default:
			break;
		}
	}
	return NULL;
}

int get_sock(char* name, int len)
{
	int i;
	for(i=0; i<MAX_CLNT_SIZE; i++)
	{
		if (users[i][0] == 0) continue;
		if (string_compare(users[i], name, len)) return i;
	}
}

int get_room_id(char* name, int len)
{
	int i;
	for(i=0; i<MAX_ROOM_SIZE; i++)
	{
		if (rooms[i][0] == 0) continue;
		if (string_compare(rooms[i], name, len)) return i;
	}
}