#include "common_constants.h"

#ifndef MESSAGE_H
#define MESSAGE_H

void rebuild_msg(Message* msg, int sender);
void rebuild_system_msg(Message* msg, Message_Type type, int sender);
void build_common_msg(Message* msg);
void build_invite_room_msg(Message* msg, int dest);
void build_private_msg(Message* msg, int dest);
void build_broadcast_msg(Message* msg);
void build_make_room_msg(Message* msg);
void build_join_room_msg(Message* msg, int room_id);
void build_quit_msg(Message* msg);
void build_expulsion_room_msg(Message* msg, int dest);
void build_exit_room_msg(Message* msg);
void build_all_clnt_data_msg(Message* msg);
void build_all_clnt_in_room_data_msg(Message* msg);
void build_all_room_data_msg(Message* msg);
void send_to(Client* clnt, Message* msg);

#endif