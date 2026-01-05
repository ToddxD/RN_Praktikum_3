#ifndef PROTOCOL_H
#define PROTOCOL_H

extern char ownName[32];

void do_chat(const char* target, const char* msg);

void do_login(const char* chat_name, const char* local_host, const int local_port,
              const char* target_host, const int target_port);

void protocol_handle_msg(const int connection);

#endif  // PROTOCOL_H