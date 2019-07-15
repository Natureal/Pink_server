#include "basic_tool.h"

int read_conf(const char *filename, conf_t &conf){

	ifstream conf_file(filename);
	if(!conf_file.is_open()){
		cout << "Cannot open the config file." << endl;
		return IMPORT_CONF_ERROR;
	}
	string str_line;
	while(!conf_file.eof()){

		getline(conf_file, str_line);
		size_t pos = str_line.find('=');
		string key = str_line.substr(0, pos);
		string val = str_line.substr(pos + 1, str_line.length());

		if(key == "port"){
			conf.port = atoi(val.c_str());
		}
		else if(key == "doc_root"){
			strcpy(conf.doc_root, val.c_str());
		}
		else if(key == "max_thread_number"){
			conf.max_thread_number = atoi(val.c_str());
		}
		else if(key == "timeslot"){
			conf.timeslot = atoi(val.c_str());
		}
		else if(key == "conn_timeout"){
			conf.conn_timeout = atoi(val.c_str());
		}
		else if(key == "pre_conn_number"){
			conf.pre_conn_number = atoi(val.c_str());
		}
		else if(key == "max_conn_number"){
			conf.max_conn_number = atoi(val.c_str());
		}
		else if(key == "epoll"){
			if(val == "et"){
				conf.epoll_et = true;
			}
			else if(val == "lt"){
				conf.epoll_et = false;
			}
		}
	}
	return IMPORT_CONF_SUCCESS;
}

void add_signal(const int sig, void (handler)(int), bool restart){
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = handler;
	if(restart){
		sa.sa_flags |= SA_RESTART;
	}
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);
}

