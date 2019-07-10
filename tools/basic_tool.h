#ifndef BASIC_TOOL_H
#define BASIC_TOOL_H

#include <fstream>
#include <iostream>
#include <string>
#include <string.h>
#include <signal.h>
#include <assert.h>

using std::string;
using std::ifstream;
using std::cout;
using std::endl;

const short PATH_LEN = 128;
const short IMPORT_CONF_SUCCESS = 0;
const short IMPORT_CONF_ERROR = -1;

typedef struct configuration{
	char doc_root[PATH_LEN];
	int port;
	int max_thread_number;
}conf_t;

int read_conf(const char *filename, conf_t &conf);
void add_signal(const int sig, void (handler)(int), bool restart = true);


#endif
