#include "basic_tool.h"

int import_conf(const char *filename, conf_t *conf){
	ifstream conf_file(filename->c_str());
	if(!conf_file.is_open()){
		cout << "Cannot open the config file." << endl;
		return IMPORT_CONF_ERROR
	}
	string str_line;
	while(!conf_file.eof()){
		getline(conf_file, str_line);
		size_t pos = str_line.find('=');
		string key = str_line.substr(0, pos);
		
	}
	return IMPORT_CONF_SUCCESS;
}

void add_signal(const int sig, void (handler)(int), bool restart = true){
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = handler;
	if(restart){
		sa.sa_flags |= SA_RESTART;
	}
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);
}

