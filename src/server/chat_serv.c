#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#include "common_constants.h"
#include "server_constants.h"
#include "s_util.h"
#include "message.h"

#define LISTEN_Q_SIZE 5

/* node_manager */
extern void node_manager_init();
extern void node_manager_close();

/* room_manager */
extern void room_manager_init();
extern void room_manager_close();

/* clnt_db */
extern void clnt_db_init();
extern void clnt_db_close();
extern int get_all_client(int index, Message* msg);
extern void get_client_data(int sock, Message* msg);
extern Client* get_client(int sock);
extern Client* get_new_clnt();
extern void add_clnt();
extern void remove_clnt(int sock);
extern void for_each_client_except_sock(void (*callback)(Client*, Message*), int sock, Message* msg);
extern void for_each_client(void (*callback)(Client*, Message*), Message* msg);
extern void for_client(void (*callback)(Client*, Message*), int sock, Message* msg);

/* room_db */
extern void room_db_init();
extern void room_db_close();
extern int add_room(Client* admin, char* room_name);
extern Bool remove_room(Client* clnt, int room_id);
extern int get_all_room(int index, Message* msg);
extern Member* get_all_client_in_room(int room_id, Message* msg, Member* member);
extern Bool is_admin(Client* clnt, int room_id);
extern Bool add_client_at_room(Client* clnt, int room_id);
extern Bool remove_client_at_room(Client* clnt, int room_id);
extern void for_each_client_in_room(void (*callback)(Client*, int, Message*), int room_id, int sock, Message* msg);

void * handle_clnt(void * arg);
void signal_handler(int sig);

static Net_Obj server_obj;
static Bool running = True;

int main(int argc, char *argv[])
{
	Client* new_clnt;
	int len;
	pthread_t t_id;
	
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

    signal(SIGINT, signal_handler);

	/* 저장소 초기화 */
	node_manager_init();
	room_manager_init();
	clnt_db_init();
	room_db_init();

	setup_default_address(&server_obj, atoi(argv[1]));
	if(bind(server_obj.sock, &(server_obj.adr), sizeof(server_obj.adr))==-1)
		error_handling("bind() error");
	if(listen(server_obj.sock, LISTEN_Q_SIZE)==-1)
		error_handling("listen() error");

	while(running)
	{
		new_clnt = get_new_clnt();
		len = sizeof(sizeof(new_clnt->net_obj.adr));
		new_clnt->net_obj.sock = accept(
			server_obj.sock,
			&new_clnt->net_obj.adr,
			&len
		);

		if (new_clnt->net_obj.sock == -1) {
            if (!running) break;
            continue;
        }
		add_clnt();

		pthread_create(&t_id, NULL, handle_clnt, (void*)new_clnt);
		pthread_detach(t_id);
		//printf("Connected client IP: %s \n", inet_ntoa(((struct sockaddr_in*)&new_clnt->net_obj.adr)->sin_addr));
	}
	close(server_obj.sock);

    clnt_db_close();
    room_db_close();
    room_manager_close();
    node_manager_close();
	return 0;
}
	
void * handle_clnt(void * arg)
{
	Client* clnt;
	Message msg;
	Member* mem;
	int sock, dest, room_id;

	clnt = (Client*)arg;
	sock = clnt->net_obj.sock;
	clnt->room_id = -1;

	/* 클라이언트 이름 설정 */
	while(dest <= 1)
	{
		read(sock, &msg, sizeof(Message));
		dest = string_copy(msg.message, clnt->name, MAX_NAME_LENGTH+1);
		if(dest <= 1)
		{
			rebuild_system_msg(&msg, Faillure_Msg, sock);
			for_client(send_to, sock, &msg);
		}
		else
		{
			rebuild_system_msg(&msg, Success_Msg, sock);
			for_client(send_to, sock, &msg);
		}
	}
	clnt->name_len = dest;

	/* 클라이언트 정보 전파 */
	rebuild_system_msg(&msg, All_Clnt_Data_Msg, 0);
	get_client_data(sock, &msg);
	for_each_client_except_sock(send_to, sock, &msg);

	
	while(read(sock, &msg, sizeof(Message))){
		switch (msg.m_type)
		{
		case Common_Msg:
			/* 현재 방 메시지 */
			rebuild_msg(&msg, sock);
			for_each_client_in_room(send_to, clnt->room_id, sock, &msg);
			break;
		case Invite_Room_Msg:
			/* 방 초대 메시지 */
		case Private_Msg:
			/* 개인 메시지 */
			dest = msg.transceiver; 
			rebuild_msg(&msg, sock);
			for_client(send_to, dest, &msg);
			break;
		case Broadcast_Msg:
			/* 전체 메시지 */
			rebuild_msg(&msg, sock);
			for_each_client_except_sock(send_to, sock, &msg);
			break;
		case Make_Room_Msg:
			/* 방 만들기 메시지 */
			room_id = add_room(clnt, msg.message);

			if (room_id < 0)
			{
				rebuild_system_msg(&msg, Faillure_Msg, room_id);
				for_client(send_to, sock, &msg);
				break;
			}
			remove_client_at_room(clnt, clnt->room_id);
			clnt->room_id = room_id;

			rebuild_msg(&msg, room_id);
			for_each_client_except_sock(send_to, sock, &msg);
			rebuild_system_msg(&msg, Success_Msg, room_id);
			for_client(send_to, sock, &msg);
			break;
		case Remove_Room_Msg:
			/* 방 제거 메시지 */
			if(remove_room(clnt, clnt->room_id))
			{
				rebuild_system_msg(&msg, Faillure_Msg, clnt->room_id);
				for_client(send_to, sock, &msg);
				break;
			}
			rebuild_msg(&msg, clnt->room_id);
			for_each_client_except_sock(send_to, sock, &msg);
			rebuild_system_msg(&msg, Success_Msg, clnt->room_id);
			for_client(send_to, sock, &msg);
			break;
		case Join_Room_Msg:
			/* 방 입장 메시지 */
			room_id = msg.transceiver;
			if (room_id == clnt->room_id || add_client_at_room(clnt, room_id))
			{
				rebuild_system_msg(&msg, Faillure_Msg, room_id);
				for_client(send_to, sock, &msg);
				break;
			}
			remove_client_at_room(clnt, clnt->room_id);
			clnt->room_id = room_id;
			rebuild_msg(&msg, sock);
			for_each_client_in_room(send_to, room_id, sock, &msg);
			rebuild_system_msg(&msg, Success_Msg, room_id);
			for_client(send_to, sock, &msg);
			break;
		case Expulsion_Room_Msg:
			/* 방 퇴출 메시지 */
			dest = msg.transceiver;
			room_id = clnt->room_id;
			if (dest != sock && is_admin(clnt, room_id) && !remove_client_at_room(get_client(dest), room_id))
			{
				get_client(dest)->room_id = -1;
				for_each_client_in_room(send_to, room_id, sock, &msg);
				for_client(send_to, dest, &msg);
				rebuild_system_msg(&msg, Success_Msg, dest);
				for_client(send_to, sock, &msg);
				break;
			}
			rebuild_system_msg(&msg, Faillure_Msg, dest);
			for_client(send_to, sock, &msg);
			break;
		case Exit_Room_Msg:
			/* 방 나가기 */
			room_id = clnt->room_id;
			if (room_id == 0)
			{
				rebuild_system_msg(&msg, Faillure_Msg, dest);
				for_client(send_to, sock, &msg);		
			}
			remove_client_at_room(clnt, room_id);
			rebuild_msg(&msg, sock);
			for_each_client_in_room(send_to, room_id, sock, &msg);
			rebuild_system_msg(&msg, Success_Msg, room_id);
			for_client(send_to, sock, &msg);
			break;
		case All_Clnt_Data_Msg:
			/* 모든 유저 정보 요청 */
			dest = 0;
			rebuild_msg(&msg, 0);
			while((dest = get_all_client(dest, &msg)))
			{
				for_client(send_to, sock, &msg);
				rebuild_msg(&msg, 0);
			}
			for_client(send_to, sock, &msg);
			break;
		case All_Clnt_In_Room_Data_Msg:
			/* 현재 방의 유저 정보 요청 */
			mem = NULL;
			rebuild_msg(&msg, 0);
			while((mem = get_all_client_in_room(clnt->room_id, &msg, mem)))
			{
				for_client(send_to, sock, &msg);
				rebuild_msg(&msg, 0);
			}
			for_client(send_to, sock, &msg);
			break;
		case All_Room_Data_Msg:
			/* 현재 방들 정보 요청 */
			dest = 0;
			rebuild_msg(&msg, 0);
			while((dest = get_all_room(dest, &msg)))
			{
				for_client(send_to, sock, &msg);
				rebuild_msg(&msg, 0);
			}
			for_client(send_to, sock, &msg);
			break;
		case Quit_msg:
			/* 종료 */
			room_id = clnt->room_id;
			remove_client_at_room(clnt, room_id);
			rebuild_system_msg(&msg, Exit_Room_Msg, sock);
			for_each_client_in_room(send_to, room_id, sock, &msg);
			remove_clnt(sock);
			close(sock);
			rebuild_system_msg(&msg, Quit_msg, sock);
			for_each_client(send_to, &msg);
			return NULL;
		default:
			break;
		}
	}

	close(sock);
	remove_clnt(sock);
	return NULL;
}

void signal_handler(int sig)
{
    if (sig == SIGINT)
    {
        printf("Caught signal %d, exiting...\n", sig);
        running = 0;
        close(server_obj.sock);
    }
}