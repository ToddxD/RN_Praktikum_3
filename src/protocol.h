#ifndef PROTOCOL_H
#define PROTOCOL_H

extern char ownName[32];

void do_chat(const char* target, const char* msg);

void do_login(char* chat_name, int  local_port, char* target_host, int target_port);

void protocol_handle_msg(const int connection);

#endif // PROTOCOL_H