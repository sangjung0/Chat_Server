#include <stdio.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#include "common_constants.h"

#include "s_util.h"

static void setup_address_base(Net_Obj *obj, in_addr_t ip, uint16_t port);

void setup_address(Net_Obj *obj, const char *ip, uint16_t port)
{
    setup_address_base(obj, inet_addr(ip), htons(port));
}

void setup_default_address(Net_Obj *obj, uint16_t port)
{
    setup_address_base(obj, htonl(INADDR_ANY), htons(port));
}

static void setup_address_base(Net_Obj *obj, in_addr_t ip, uint16_t port){
    struct sockaddr_in * temp;

    obj->sock = socket(PF_INET, SOCK_STREAM, 0);

    temp = (struct sockaddr_in*) &obj->adr;

    memset(temp, 0, sizeof(*temp));
    temp->sin_family = AF_INET;
    temp->sin_addr.s_addr= ip;
    temp->sin_port = port;
}

int string_copy(const char* src, char* dest, int length)
{
    int i;
    i=0;
    while(i < length)
    {
        dest[i] = src[i];
        if(src[i++] == 0) return i;
    }
    return -1;
}

Bool string_compare(char* src, const char* dest, int length)
{
    int i;
    i = 0;
    while(i < length)
    {
        if(src[i] != dest[i] ) return False;
        i++;
    }

    return True;
}

int remove_ln(char* src, int length)
{
    int i;
    i = 0;
    while(i<length)
    {
        if(src[i] == '\n' || src[i] == 0) 
        {
            src[i] = 0;
            return i+1;
        }
        i++;
    }
    return i;
}

void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

int space_token(char *msg, int index, int len)
{
    for(;index < len; index++)
    {
        if(msg[index] == 0) break;
        else if(msg[index] == ' ') return index;
    }
    return -1;
}

void slide_str(char *msg, int index, int len)
{
    int idx;
    idx = 0;
    while(index < len)
    {
        msg[idx] = msg[index++];
        if(msg[idx++] == 0) return;
    }
    while(idx < len)
    {
        msg[idx++] = 0;
    }
}

Message_Type get_message_type_from_str(char* str)
{
    if (str[0] != '/' && str[0] != '@') return Common_Msg;
    if (string_compare(str, "/quit", 5)) return Quit_msg;
    if (string_compare(str, "@all", 4)) return Broadcast_Msg;
    if (str[0] == '@') return Private_Msg;
    if (string_compare(str, "/mkrm ", 6)) return Make_Room_Msg;
    if (string_compare(str, "/rmrm", 5)) return Remove_Room_Msg;
    if (string_compare(str, "/ivrm ", 6)) return Invite_Room_Msg;
    if (string_compare(str, "/exrm ", 6)) return Expulsion_Room_Msg;
    if (string_compare(str, "/jnrm ", 6)) return Join_Room_Msg;
    if (string_compare(str, "/etrm", 5)) return Exit_Room_Msg;
    if (string_compare(str, "/list", 5)) return All_Clnt_Data_Msg;
    if (string_compare(str, "/lsrm", 5)) return All_Room_Data_Msg;
    if (string_compare(str, "/lcrm", 5)) return All_Clnt_In_Room_Data_Msg;
    return Common_Msg;
}
