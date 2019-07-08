#include "pink_http_machine.h"

// 定义HTTP相应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n"; 
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get the file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requested file.\n";
const char *doc_root = "/var/www/html";


void pink_http_machine::init(char *read_buf, char *write_buf){
	check_state = CHECK_STATE_REQUESTLINE;
	
	method = GET;
	
	m_read_buf = read_buf;
	m_write_buf = write_buf;

	url = nullptr;
	version = nullptr;
	host = nullptr;
	
	content_length = 0;
	start_line = 0;

}

pink_http_machine::char* get_line(){}{
	return read_buf + start_line;
}


// 主状态机
// 调用 parse_line(), parse_request_line(), parse_headers(), parse_content()
pink_http_machine::HTTP_CODE pink_http_machine::process_read(){
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NO_REQUEST;
	char *text = 0;

	while(((check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK))
			|| ((line_status = parse_line()) == LINE_OK)){
		text = get_line();
		start_line = checked_idx;
		printf("got 1 http line: %s\n", text);

		switch(check_state){
			case CHECK_STATE_REQUESTLINE:{
				ret = parse_request_line(text);
				if(ret == BAD_REQUEST){
					return BAD_REQUEST;
				}
				break;
			}
			case CHECK_STATE_HEADER:{
				ret = parse_headers(text);
				if(ret == BAD_REQUEST){
					return BAD_REQUEST;
				}
				else if(ret == GET_REQUEST){
					return do_request();
				}
				break;
			}
			case CHECK_STATE_CONTENT:{
				ret = parse_content(text);
				if(ret == GET_REQUEST){
					return do_request();
				}
				line_status = LINE_OPEN;
				break;
			}
			default:{
				return INTERNAL_ERROR;
			}
		}
	}

	return NO_REQUEST;
}

// 从状态机
pink_http_machine::LINE_STATUS pink_http_machine::parse_line(){
	char temp;
	for(; checked_idx < read_idx; ++checked_idx){
		temp = read_buf[checked_idx];
		if(temp == '\r'){
			if((checked_idx + 1) == read_idx){
				return LINE_OPEN;
			}
			else if(read_buf[checked_idx + 1] == '\n'){
				read_buf[checked_idx++] = '\0';
				read_buf[checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
		else if(temp == '\n'){
			if((checked_idx > 1) && (read_buf[checked_idx - 1] == '\r')){
				read_buf[checked_idx - 1] = '\0';
				read_buf[checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	return LINE_OPEN;
}

// 解析HTTP请求行，获取请求方法，目标URL，以及HTTP版本号
pink_http_machine::HTTP_CODE pink_http_machine::parse_request_line(char *text){
	// strpbrk(): string pointer break
	// 找到任意字符出现的第一个位置
	url = strpbrk(text, " \t");
	if(!url){
		return BAD_REQUEST;
	}
	*url++ = '\0';

	if(strcasecmp(text, "GET") == 0){
		method = GET;
	}
	else{
		// 目前只处理GET
		return BAD_REQUEST;
	}

	url += strspn(url, " \t");
	version = strpbrk(url, " \t");
	if(!version){
		return BAD_REQUEST;
	}
	*version++ = '\0';
	version += strspn(version, " \t");
	if(strcasecmp(version, "HTTP/1.1") != 0){
		return BAD_REQUEST;
	}
	if(strncasecmp(url, "http://", 7) == 0){
		url += 7;
		url = strchr(url, '/');
	}

	if(!url || url[0] != '/'){
		return BAD_REQUEST;
	}

	check_state = CHECK_STATE_HEADER;
	
	// 尚未完成
	return NO_REQUEST;
}


// 解析HTTP请求的一个头部信息
pink_http_machine::HTTP_CODE pink_http_machine::parse_headers(char *text){
	// 遇到空行，解析完毕
	if(text[0] == '\0'){
		// 如果该HTTP请求还有消息体，则还需要读取 m_content_length 字节的消息体
		// 转到 CHECK_STATE_CONTENT 状态
		if(content_length != 0){
			check_state = CHECK_STATE_CONTENT;
			// 未完成
			return NO_REQUEST;
		}
		// 完整
		return GET_REQUEST;
	}
	// 处理connection头部字段
	else if(strncasecmp(text, "Connection:", 11) == 0){
		text += 11;
		text += strspn(text, " \t");
		if(strcasecmp(text, "keep-alive") == 0){
			linger = true;
		}
	}
	// 处理Content-Length头部字段
	else if(strncasecmp(text, "Content-Length", 15) == 0){
		text += 15;
		text += strspn(text, " \t");
		content_length = atol(text);
	}
	// 处理Host头部字段
	else if(strncasecmp(text, "Host:", 5) == 0){
		text += 5;
		text += strspn(text, " \t");
		host = text;
	}
	else{
		printf("oops! unknow header %s\n", text);
	}
	
	// 未完成
	return NO_REQUEST;
}


// 这里并没有真正解析HTTP请求的消息体，只是判断其是否被完整地读入了
pink_http_machine::HTTP_CODE pink_http_machine::parse_content(char *text){
	if(read_idx >= (content_length + checked_idx)){
		text[content_length] = '\0';
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

// 当得到一个完整，正确的HTTP请求时，我们就开始分析目标文件。
// 如果目标存在，对所有用户可读，且不是目录，就用mmap将其映射到m_file_address处。
pink_http_machine::HTTP_CODE pink_http_machine::do_request(){
	strcpy(real_file, doc_root);
	int len = strlen(doc_root);
	strncpy(real_file + len, url, FILENAME_LEN - len - 1);
	if(stat(real_file, &file_stat) < 0){
		return NO_RESOURCE;
	}
	
	// S_IROTH：其他用户具有可读取权限
	if(!(file_stat.st_mode & S_IROTH)){
		return FORBIDDEN_REQUEST;
	}

	if(S_ISDIR(file_stat.st_mode)){
		return BAD_REQUEST;
	}

	int fd = open(real_file, O_RDONLY);
	file_address = (char*)mmap(0, file_stat.st_size, PROT_READ,
									MAP_PRIVATE, fd, 0);
	close(fd);
	return FILE_REQUEST;
}

