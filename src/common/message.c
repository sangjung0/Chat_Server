#include <unistd.h>

#include "common_constants.h"
#include "s_util.h"

#include "message.h"


void rebuild_msg(Message* msg, int sender)
{
    msg->transceiver= sender;
}

void rebuild_system_msg(Message* msg, Message_Type type, int sender)
{
    msg->m_type = type;
    msg->transceiver = sender;
}

void build_common_msg(Message* msg)
{
    msg->m_type = Common_Msg;
}

void build_invite_room_msg(Message* msg, int dest)
{
    msg->m_type = Invite_Room_Msg;
    msg->transceiver = dest;
}

void build_private_msg(Message* msg, int dest)
{
    msg->m_type = Private_Msg;
    msg->transceiver = dest;
}

void build_broadcast_msg(Message* msg)
{
    msg->m_type = Broadcast_Msg;
}

void build_make_room_msg(Message* msg)
{
    msg->m_type = Make_Room_Msg;
}

void build_join_room_msg(Message* msg, int room_id)
{
    msg->m_type = Join_Room_Msg;
    msg->transceiver = room_id;
}

void build_remove_room_msg(Message* msg)
{
    msg->m_type = Remove_Room_Msg;
}

void build_expulsion_room_msg(Message* msg, int dest)
{
    msg->m_type = Expulsion_Room_Msg;
    msg->transceiver = dest;
}

void build_exit_room_msg(Message* msg)
{
    msg->m_type = Exit_Room_Msg;
}

void build_all_clnt_data_msg(Message* msg)
{
    msg->m_type = All_Clnt_Data_Msg;
}

void build_all_clnt_in_room_data_msg(Message* msg)
{
    msg->m_type = All_Clnt_In_Room_Data_Msg;
}

void build_all_room_data_msg(Message* msg)
{
    msg->m_type = All_Room_Data_Msg;
}

void build_quit_msg(Message* msg)
{
    msg->m_type = Quit_msg;
}

void send_to(Client* clnt, Message* msg)
{
    write(clnt->net_obj.sock, msg, sizeof(Message));
}