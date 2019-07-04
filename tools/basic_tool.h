#ifndef BASIC_TOOL_H
#define BASIC_TOOL_H

#include <fstream>
#include <iostream>

const short PATH_LEN = 128;
const short IMPORT_CONF_SUCCESS = 0;
const short IMPORT_CONF_ERROR = -1;

typedef struct configuration{
	char root[PATH_LEN];
	int port;
	int max_thread_num;
}conf_t;

int import_conf(const char *filename, conf_t *conf);
void add_signal(const int sig, void (handler)(int), bool restart = true);


#endif
