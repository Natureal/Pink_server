#include "basic_tool.h"

int read_conf(const char *filename, conf_t *conf){
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

