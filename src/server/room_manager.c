#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "common_constants.h"

#define MIN 10
#define MAX 20

static pthread_mutex_t room_avail_mutx;
static pthread_mutex_t member_avail_mutx;

static Room* room_avail = NULL;
static int room_capacity = 0;
static Member* member_avail = NULL;
static int member_capacity = 0;

void room_manager_init()
{
    pthread_mutex_init(&room_avail_mutx, NULL);
    pthread_mutex_init(&member_avail_mutx, NULL);
}

void room_manager_close()
{    
    Room* room;
    Member* member;

    while(room_avail != NULL){
        room = room_avail;
        room_avail = room_avail->next;
        free(room);
    }

    while(member_avail != NULL){
        member = member_avail;
        member_avail = member_avail->next;
        free(member);
    }

    pthread_mutex_destroy(&room_avail_mutx);
    pthread_mutex_destroy(&member_avail_mutx);
}

Room* get_room()
{
    int i;
    Room *temp;

    pthread_mutex_lock(&room_avail_mutx);
    if (room_avail == NULL){
        room_avail = (Room*)malloc(sizeof(Room));
        room_avail->next = NULL;
        for(i=0; i<MIN;i++){
            temp = (Room*)malloc(sizeof(Room));
            temp->next = room_avail;
            room_avail = temp;
        }
        room_capacity += MIN + 1;
    }

    temp = room_avail;
    room_avail = temp->next;
    room_capacity -= 1;
    temp->next = NULL;
    pthread_mutex_unlock(&room_avail_mutx);

    return temp;
}

void return_room(Room* room)
{
    int i;

    pthread_mutex_lock(&room_avail_mutx);
    room->next = room_avail;
    room_avail = room;
    room_capacity += 1;

    if(room_capacity > MAX){
        for(i = 0; i < room_capacity - MIN; i++){
            room = room_avail;
            room_avail = room_avail->next;
            free(room);
        }
        room_capacity = MIN;
    }
    pthread_mutex_unlock(&room_avail_mutx);
}

Member* get_member()
{
    int i;
    Member *temp;

    pthread_mutex_lock(&member_avail_mutx);
    if (member_avail == NULL){
        member_avail = (Member*)malloc(sizeof(Member));
        member_avail->next = NULL;
        for(i=0; i<MIN;i++){
            temp = (Member*)malloc(sizeof(Member));
            temp->next = member_avail;
            member_avail = temp;
        }
        member_capacity += MIN + 1;
    }

    temp = member_avail;
    member_avail = temp->next;
    member_capacity -= 1;
    temp->next = NULL;
    pthread_mutex_unlock(&member_avail_mutx);

    return temp;
}

void return_member(Member* member)
{
    int i;

    pthread_mutex_lock(&member_avail_mutx);
    member->next = member_avail;
    member_avail = member;
    member_capacity += 1;

    if(member_capacity > MAX){
        for(i = 0; i < member_capacity - MIN; i++){
            member = member_avail;
            member_avail = member_avail->next;
            free(member);
        }
        member_capacity = MIN;
    }
    pthread_mutex_unlock(&member_avail_mutx);
}
