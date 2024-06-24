#include <arpa/inet.h>
#include <sys/socket.h>

#include "common_constants.h"

#ifndef S_UTIL_H
#define S_UTIL_H

void setup_address(Net_Obj *obj, const char *ip, uint16_t port);
void setup_default_address(Net_Obj *obj, uint16_t port);

int string_copy(const char* src, char* dest, int length);
Bool string_compare(char* src, const char* dest, int length);
int remove_ln(char* src, int length);
int space_token(char *msg, int index, int len);
void slide_str(char *msg, int index, int len);

Message_Type get_message_type_from_str(char* str);

void error_handling(char * msg);

#endif