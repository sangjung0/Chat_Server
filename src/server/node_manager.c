#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "common_constants.h"

#define MIN 10
#define MAX 20

static pthread_mutex_t avail_mutx;

static Node* avail = NULL;
static int capacity = 0;

void node_manager_init()
{
    pthread_mutex_init(&avail_mutx, NULL);
}

void node_manager_close()
{    
    Node* temp;

    while(avail != NULL){
        temp = avail;
        avail = avail->next;
        free(temp);
    }

    pthread_mutex_destroy(&avail_mutx);
}

Node* get_node()
{
    int i;
    Node *temp;

    pthread_mutex_lock(&avail_mutx);
    if (avail == NULL){
        avail = (Node*)malloc(sizeof(Node));
        avail->next = NULL;
        for(i=0; i<MIN;i++){
            temp = (Node*)malloc(sizeof(Node));
            temp->next = avail;
            avail = temp;
        }
        capacity += MIN + 1;
    }

    temp = avail;
    avail = temp->next;
    capacity -= 1;
    temp->next = NULL;
    pthread_mutex_unlock(&avail_mutx);

    return temp;
}

void return_node(Node* node)
{
    int i;

    pthread_mutex_lock(&avail_mutx);
    node->next = avail;
    avail = node;
    capacity += 1;

    if(capacity > MAX){
        for(i = 0; i < capacity - MIN; i++){
            node = avail;
            avail = avail->next;
            free(node);
        }
        capacity = MIN;
    }
    pthread_mutex_unlock(&avail_mutx);
}