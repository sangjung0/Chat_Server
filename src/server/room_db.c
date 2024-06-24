#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "common_constants.h"
#include "s_util.h"

extern Room* get_room();
extern void return_room(Room* room);
extern Member* get_member();
extern void return_member(Member* room);

static Bool add_client_at_lobby(Client* clnt);
static Bool remove_client_at_lobby(Client* clnt);
static Bool add_client_at_room_Implt(Client* clnt, int room_id);
static Bool remove_client_at_room_Implt(Client* clnt, int room_id);

static pthread_mutex_t room_mutx;
static pthread_mutex_t lobby_mutx;
static Room* all_room[MAX_ROOM_SIZE+1];
static int last_room_id = 0;

static int room_name_length = MAX_ROOM_NAME+1; 

void room_db_init()
{
    Room* room;

    room = get_room();
    room->next = NULL;
    room->member = NULL;
    room->admin = NULL;
    room->room_id = last_room_id++;
    string_copy("Lobby", room->room_name, room_name_length);
    room->name_len = 5;
    all_room[room->room_id] = room;
    pthread_mutex_init(&room_mutx, NULL);
    pthread_mutex_init(&lobby_mutx, NULL);
}

void room_db_close()
{
    Member* member, *temp;
    int i;
    
    for(i = 0; i<MAX_ROOM_SIZE; i++)
    {
        if (all_room[i] == NULL) continue;
        member = all_room[i]->member;
        while(member!=NULL)
        {
            temp = member;
            member = member->next;
            return_member(temp);
        }
        return_room(all_room[i]);
    }

    pthread_mutex_destroy(&room_mutx);
    pthread_mutex_destroy(&lobby_mutx);
}

int add_room(Client* admin, char* room_name)
{
    Room* room;
    Member* member;
    int rid;

    room = get_room();
    member = get_member();
    member->next = NULL;
    member->prev = NULL;
    member->clnt = admin;

    room->member = member;
    room->admin = admin;
    room->name_len = string_copy(room_name, room->room_name, room_name_length);

    pthread_mutex_lock(&room_mutx);
    for(rid = last_room_id++ ; all_room[rid] && rid < MAX_ROOM_SIZE; rid++);
    if (rid < MAX_ROOM_SIZE)
    {
        room->room_id = rid;
        all_room[rid] = room;
    }
    else rid = -1;
    pthread_mutex_unlock(&room_mutx);

    return rid;
}

Bool remove_room(Client* clnt, int room_id)
{
    Room* room;
    Member* member, *temp;

    pthread_mutex_lock(&room_mutx);
    last_room_id = room_id;
    room = all_room[room_id];
    if (room->admin == clnt) all_room[room_id] = NULL;
    else room = NULL;
    pthread_mutex_unlock(&room_mutx);
    if (room != NULL)
    {
        member = room->member;
        while(member != NULL)
        {
            temp = member;
            member = member->next;
            return_member(temp);
        }
        return_room(room);
        return False;
    }
    return True;
}

int get_all_room(int index, Message* msg)
{
    int len, name_len;
    
    pthread_mutex_lock(&room_mutx);
    for(;index < last_room_id; index++)
    {
        name_len = all_room[index]->name_len;

        len = msg->transceiver + 2*sizeof(int) + name_len;
        if (len > MAX_MESSAGE_LENGTH -1) break;

        memcpy(&msg->message[msg->transceiver], &index, sizeof(int));
        msg->transceiver += sizeof(int);

        memcpy(&msg->message[msg->transceiver], &name_len, sizeof(int));
        msg->transceiver += sizeof(int);

        memcpy(&msg->message[msg->transceiver], all_room[index]->room_name, name_len);
        msg->transceiver += name_len;
    }
    pthread_mutex_unlock(&room_mutx);

    if (index == last_room_id) return 0;
    return index;
}

Member* get_all_client_in_room(int room_id, Message* msg, Member* member)
{
    int len, sock;
    Room* room;

    if (member == NULL) 
    {
        pthread_mutex_lock(&room_mutx);
        room = all_room[room_id];
        if (room != NULL) member = room->member;
    }
    if (member != NULL) {
        while (member != NULL) {
            sock = member->clnt->net_obj.sock;

            len = msg->transceiver + sizeof(int);
            if (len > MAX_MESSAGE_LENGTH - 1) break;
    
            memcpy(&msg->message[msg->transceiver], &sock, sizeof(int));
            msg->transceiver += sizeof(int);

            member = member->next;
        }
    }
    if (member == NULL) {
        pthread_mutex_unlock(&room_mutx);
    }
    return member;
}

Bool is_admin(Client* clnt, int room_id)
{
    Bool chk = False;
    pthread_mutex_lock(&room_mutx);
    if (all_room[room_id]->admin == clnt) chk = True;
    pthread_mutex_unlock(&room_mutx);
    return chk;
}

Bool add_client_at_room(Client* clnt, int room_id)
{
    if(room_id == 0) return add_client_at_lobby(clnt);
    if(room_id > 0) return add_client_at_room_Implt(clnt, room_id);
    return True;
}

Bool remove_client_at_room(Client* clnt, int room_id)
{
    if(room_id == 0) return remove_client_at_lobby(clnt);
    if(room_id > 0) return  remove_client_at_room_Implt(clnt, room_id);
    return True;
}

void for_each_client_in_room(void (*callback)(Client*, Message*), int room_id, int sock, Message* msg) 
{
    Room* room;
    Member* member;

    pthread_mutex_lock(&room_mutx);
    room = all_room[room_id];
    if (room != NULL)
    {
        member = room->member;
        while(member != NULL)
        {
            if(member->clnt->net_obj.sock != sock) callback(member->clnt, msg);
            member = member->next;
        }
    }
    pthread_mutex_unlock(&room_mutx);
}

static Bool add_client_at_lobby(Client* clnt)
{
    Member* member;
    Room* lobby;

    lobby = all_room[0];
    member = get_member();
    member->clnt = clnt;
    member->prev = NULL;

    pthread_mutex_lock(&lobby_mutx);
    member->next = lobby->member;
    if (lobby->member != NULL)  lobby->member->prev = member;
    lobby->member = member;
    pthread_mutex_unlock(&lobby_mutx);

    return False;
}

static Bool remove_client_at_lobby(Client* clnt)
{
    Member* member;
    Room* lobby;

    lobby = all_room[0];
    pthread_mutex_lock(&lobby_mutx);
    for(member = lobby->member; member != NULL && member->clnt != clnt; member = member->next);
    if (member != NULL)
    {
        if(member->prev != NULL) member->prev->next = member->next;
        if(member->next != NULL) member->next->prev = member->prev;
        if (lobby->member == member) lobby->member = member->next;
    }
    pthread_mutex_unlock(&lobby_mutx);
    if (member != NULL) {
        return_member(member);
        return False;
    }
    return True;
}

static Bool add_client_at_room_Implt(Client* clnt, int room_id)
{ 
    Room* room;
    Member* member;

    member = get_member();
    member->clnt = clnt;

    pthread_mutex_lock(&room_mutx);
    room = all_room[room_id];
    if (room != NULL)
    {
        member->next = room->member;
        member->prev = room->member->prev;
        room->member->prev = member;
        room->member = member;
    }
    pthread_mutex_unlock(&room_mutx);
    if (room == NULL) {
        return_member(member);
        return True;
    }
    return False;
}

static Bool remove_client_at_room_Implt(Client* clnt, int room_id)
{
    Room* room;
    Member* member;

    pthread_mutex_lock(&room_mutx);
    room = all_room[room_id];
    member = NULL;
    if (room != NULL)
    {
        for(member = room->member; member != NULL && member->clnt != clnt; member = member->next);
        if (member != NULL)
        {
            if(member->prev != NULL) member->prev->next = member->next;
            if(member->next != NULL) member->next->prev = member->prev;
            if(room->member == member) room->member = member->next;
        }
    }
    pthread_mutex_unlock(&room_mutx);
    if (member != NULL) 
    {
        return_member(member);
        return False;
    }
    return True;
}

