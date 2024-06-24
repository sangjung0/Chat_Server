#include <sys/socket.h>

#ifndef COMMON_CONSTANTS_H
#define COMMON_CONSTANTS_H

#define MAX_MESSAGE_LENGTH 2048
#define MAX_NAME_LENGTH 100
#define MAX_CLNT_SIZE 100
#define MAX_ROOM_SIZE 10
#define MAX_ROOM_NAME 100

typedef enum{
    False,
    True
} Bool;

typedef enum{
    Quit_msg,
    Common_Msg,
    Private_Msg,
    Broadcast_Msg,
    Make_Room_Msg,
    Remove_Room_Msg,
    Invite_Room_Msg,
    Expulsion_Room_Msg,
    Join_Room_Msg,
    Exit_Room_Msg,
    Faillure_Msg,
    Success_Msg,
    All_Clnt_Data_Msg,
    All_Room_Data_Msg,
    All_Clnt_In_Room_Data_Msg,
    Unknown_Msg
} Message_Type;

typedef struct {
    int sock;
    struct sockaddr adr;
} Net_Obj;

typedef struct {
    int room_id;
    int name_len;
    Net_Obj net_obj;
    char name[MAX_NAME_LENGTH+1];
} Client;

typedef struct{
    Message_Type m_type;
    int transceiver;
    char message[MAX_MESSAGE_LENGTH+1];
} Message;

typedef struct Node{
    struct Node* next;
    struct Node* prev;
    Client clnt;
} Node;

typedef struct Member{
    struct Member* next;
    struct Member* prev;
    Client* clnt;
} Member;

typedef struct Room{
    struct Room* next;
    Member* member;
    Client* admin;
    int room_id;
    int name_len;
    char room_name[MAX_ROOM_NAME+1];
} Room;

#endif

