#include <pthread.h>
#include <unistd.h>

#include "common_constants.h"
#include "s_util.h"

extern Node* get_node();
extern void return_node(Node* node);

static pthread_mutex_t clnt_mutx;
static int all_clnt_length = MAX_CLNT_SIZE+3;
static Node* all_clnt[MAX_CLNT_SIZE+3];
static Node* last_clnt = NULL;

static Node* cnt = NULL;
static int idx = 0;

void clnt_db_init()
{
    pthread_mutex_init(&clnt_mutx, NULL);
}

void clnt_db_close()
{
    Node* temp;

    while(last_clnt != NULL){
        temp = last_clnt;
        last_clnt->prev = last_clnt;
        return_node(temp);
    }
    pthread_mutex_destroy(&clnt_mutx);
}

int get_all_client(int index, Message* msg)
{
    int len, name_len, sock;

    pthread_mutex_lock(&clnt_mutx);
    for(;index < all_clnt_length; index++)
    {
        if(all_clnt[index] == NULL) continue;

        name_len = all_clnt[index]->clnt.name_len;
        sock = all_clnt[index]->clnt.net_obj.sock;

        len = msg->transceiver + 2*sizeof(int) + name_len;
        if (len > MAX_MESSAGE_LENGTH -1) break;

        memcpy(&msg->message[msg->transceiver], &sock, sizeof(int));
        msg->transceiver += sizeof(int);

        memcpy(&msg->message[msg->transceiver], &name_len, sizeof(int));
        msg->transceiver += sizeof(int);

        memcpy(&msg->message[msg->transceiver], all_clnt[index]->clnt.name, name_len);
        msg->transceiver += name_len;
    }
    pthread_mutex_unlock(&clnt_mutx);

    if (index == all_clnt_length) return 0;
    return index;
}

void get_client_data(int sock, Message* msg)
{
    memcpy(&msg->message[msg->transceiver], &sock, sizeof(int));
    msg->transceiver += sizeof(int);

    memcpy(&msg->message[msg->transceiver], &all_clnt[sock]->clnt.name_len, sizeof(int));
    msg->transceiver += sizeof(int);

    memcpy(&msg->message[msg->transceiver], all_clnt[sock]->clnt.name, all_clnt[sock]->clnt.name_len);
    msg->transceiver += all_clnt[sock]->clnt.name_len;
}

Client* get_client(int sock)
{
    Client* clnt;
    clnt = NULL;
    pthread_mutex_lock(&clnt_mutx);
    if (all_clnt[sock] != NULL) clnt = &all_clnt[sock]->clnt;
    pthread_mutex_unlock(&clnt_mutx);
    
    clnt->name_len = 0;
    return clnt;
}

Client* get_new_clnt()
{
    if (cnt != NULL){
        return_node(cnt);
    }
    cnt = get_node();
    return &cnt->clnt;
}

void add_clnt()
{
    if (cnt == NULL) return;
    pthread_mutex_lock(&clnt_mutx);
    if(last_clnt == NULL){
        cnt->next = NULL;
        cnt->prev = NULL;
        last_clnt = cnt;
    }else{
        cnt->next = NULL;
        cnt->prev = last_clnt;
        last_clnt->next = cnt;
        last_clnt = cnt;
    }
    all_clnt[cnt->clnt.net_obj.sock] = cnt;
    pthread_mutex_unlock(&clnt_mutx);

    cnt = NULL;
}

void remove_clnt(int sock)
{
    Node* temp;

    pthread_mutex_lock(&clnt_mutx);
    if (all_clnt[sock] == NULL) return;
    temp = all_clnt[sock];
    if(temp->next != NULL) temp->next->prev = temp->prev;
    if(temp->prev != NULL) temp->prev->next = temp->next;
    all_clnt[sock] = NULL;
    pthread_mutex_unlock(&clnt_mutx);
    return_node(temp);
}

void for_each_client_except_sock(void (*callback)(Client*, Message*), int sock, Message* msg)
{
    Node* current;

    pthread_mutex_lock(&clnt_mutx);
    current = last_clnt;
    while (current != NULL) {
        if(current->clnt.name_len > 0 && current->clnt.net_obj.sock != sock) callback(&current->clnt, msg);
        current = current->prev;
    }
    pthread_mutex_unlock(&clnt_mutx);
}

void for_each_client(void (*callback)(Client*, Message*), Message* msg) 
{
    Node* current;

    pthread_mutex_lock(&clnt_mutx);
    current = last_clnt;
    while (current != NULL) {
        if(current->clnt.name_len > 0) callback(&current->clnt, msg);
        current = current->prev;
    }
    pthread_mutex_unlock(&clnt_mutx);
}

void for_client(void (*callback)(Client*, Message*), int sock, Message* msg)
{
    pthread_mutex_lock(&clnt_mutx);
    if (all_clnt[sock] != NULL) callback(&all_clnt[sock]->clnt, msg);
    pthread_mutex_unlock(&clnt_mutx);

}


