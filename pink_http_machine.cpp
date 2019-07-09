#include "pink_http_machine.h"

using std::string;
using std::unordered_map;


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


extern conf_t conf;
constexpr int pink_http_machine::FILENAME_LEN; // 静态成员的类外声明

std::unordered_map<string, string> pink_http_machine::mime_type = 
								pink_http_machine::mime_map_init();

// 静态函数，初始化静态成员 mime_type
std::unordered_map<string, string> pink_http_machine::mime_map_init(){

	std::unordered_map<string, string> temp;

	temp.insert(make_pair(string("text/html"), string(".html")));
	temp.insert(make_pair(string("text/plain"), string("")));
	temp.insert(make_pair(string("text/xml"), string(".xml")));
	temp.insert(make_pair(string("text/css"), string(".css")));


	temp.insert(make_pair(string("application/xml"), string(".xml")));
	temp.insert(make_pair(string("application/xhtml+xml"), string("xhtml")));

	temp.insert(make_pair(string("image/png"), string(".png")));
	temp.insert(make_pair(string("image/gif"), string(".gif")));

	temp.insert(make_pair(string("application/pdf"), string(".pdf")));
	temp.insert(make_pair(string("application/rtf"), string(".rtf")));
	
	temp.insert(make_pair(string("application/vnd.ms-powerpoint"), string(".ppt")));
	temp.insert(make_pair(string("application/vnd.ms-excel"), string(".xls")));
	temp.insert(make_pair(string("application/msword"), string(".doc")));
	

	temp.insert(make_pair(string("audio/basic"), string(".au")));
	temp.insert(make_pair(string("video/mpeg"), string(".mpeg")));
	temp.insert(make_pair(string("video/x-msvideo"), string(".avi")));

	temp.insert(make_pair(string("application/x-gzip"), string(".gz")));
	temp.insert(make_pair(string("application/x-tar"), string(".tar")));

	temp.insert(make_pair(string("audio/x-pn-realaudio"), string(".ram")));
	temp.insert(make_pair(string("audio/x-midi"), string(".midi")));

	return temp;
}

void pink_http_machine::init(char *read_buf, char *write_buf, int *read_idx, 
			int *write_idx, const int read_buf_size, const int write_buf_size){

	check_state = CHECK_STATE_REQUESTLINE;
	checked_idx = 0;
	start_of_line = 0;
	m_read_buf = read_buf;
	m_write_buf = write_buf;
	m_read_idx = read_idx;
	m_write_idx = write_idx;
	m_read_buf_size = read_buf_size;
	m_write_buf_size = write_buf_size;

	method = GET;
	url = nullptr;
	version = nullptr;

	linger = false; // 不保持HTTP连接
	accept = nullptr;
	accept_charset = nullptr;
	accept_encoding = nullptr;
	accept_language = nullptr;
	cookie = nullptr;
	content_length = 0;
	host = nullptr;
	date = nullptr;
	client_ip = nullptr;
	client_email = nullptr;
	referer = nullptr;
	user_agent = nullptr;

	memset(request_file, '\0', FILENAME_LEN);
	file_address = nullptr;

}

// 主状态机
// 调用 parse_line(), parse_request_line(), parse_headers(), parse_content()
pink_http_machine::HTTP_CODE pink_http_machine::process_read(){
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NOT_COMPLETED;
	char *text = nullptr;

	while(((check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK))
			|| ((line_status = parse_line()) == LINE_OK)){

		text = m_read_buf + start_of_line;
		start_of_line = checked_idx;

		printf("got a http line: %s\n", text);

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

	return NOT_COMPLETED;
}

// 从状态机，主要工作是读取 m_read_buf 中的一行
pink_http_machine::LINE_STATUS pink_http_machine::parse_line(){
	char temp;
	for(; checked_idx < *m_read_idx; ++checked_idx){
		temp = m_read_buf[checked_idx];
		if(temp == '\r'){
			if((checked_idx + 1) == *m_read_idx){
				return LINE_OPEN; // 还没读完
			}
			else if(m_read_buf[checked_idx + 1] == '\n'){ // 读完了
				m_read_buf[checked_idx++] = '\0';
				m_read_buf[checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
		else if(temp == '\n'){
			if((checked_idx > 1) && (m_read_buf[checked_idx - 1] == '\r')){ // 读完了
				m_read_buf[checked_idx - 1] = '\0';
				m_read_buf[checked_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD; // 这行有问题
		}
	}
	return LINE_OPEN; // 需要继续读
}

// 工具函数，用于解析请求方法
pink_http_machine::METHOD pink_http_machine::parse_request_method(char *text){
	if(text[0] == 'P'){
		if(strcasecmp(text, "POST") == 0) return POST;
		if(strcasecmp(text, "PUT") == 0) return PUT;
		if(strcasecmp(text, "PATCH") == 0) return PATCH;
	}
	if(strcasecmp(text, "GET") == 0) return GET;
	if(strcasecmp(text, "HEAD") == 0) return HEAD;
	if(strcasecmp(text, "DELETE") == 0) return DELETE;
	if(strcasecmp(text, "TRACE") == 0) return TRACE;
	if(strcasecmp(text, "OPTIONS") == 0) return OPTIONS;
	if(strcasecmp(text, "CONNECT") == 0) return CONNECT;
	
	return UNKNOWN_METHOD;
}

// 解析HTTP请求行，获取请求方法，目标URL，以及HTTP版本号
pink_http_machine::HTTP_CODE pink_http_machine::parse_request_line(char *text){
	// 解析 URL
	// strpbrk(): string pointer break
	// 找到任意字符出现的第一个位置
	url = strpbrk(text, " \t");
	if(!url){
		return BAD_REQUEST;
	}
	*url++ = '\0';
	// strspn(): Get span of character set in string
	// 返回只包含字符集字符的前缀串长度
	url += strspn(url, " \t");
	if(strncasecmp(url, "http://", 7) == 0){
		url += 7;
		// strchr(): locate the first occurence of character in string
		url = strchr(url, '/');
	}
	if(!url || url[0] != '/'){
		return BAD_REQUEST;
	}

	// ==================================================================
	// 解析请求方法
	method = parse_request_method(text);
	if(method == UNKNOWN_METHOD){
		// 遇到未知或者不支持的请求方法
		return BAD_REQUEST;
	}

	// ==================================================================
	// 解析 HTTP 版本
	version = strpbrk(url, " \t");
	if(!version){
		return BAD_REQUEST;
	}
	*version++ = '\0';
	version += strspn(version, " \t");
	// 只支持 HTTP/1.1
	if(strcasecmp(version, "HTTP/1.1") != 0){
		return BAD_REQUEST;
	}

	check_state = CHECK_STATE_HEADER;
	
	// 尚未完成全部解析
	return NOT_COMPLETED;
}

inline bool check_and_move(char *text, const char *prefix){
	int len = strlen(prefix);
	if(strncasecmp(text, prefix, len) == 0){
		text += len;
		text += strspn(text, " \t");
		return true;
	}
	return false;
}

// 解析HTTP请求的一个头部信息
pink_http_machine::HTTP_CODE pink_http_machine::parse_headers(char *text){
	// 遇到空行，解析完毕
	if(text[0] == '\0'){
		// 如果该HTTP请求还有消息体，则还需要读取 content_length 字节的消息体
		// 转到 CHECK_STATE_CONTENT 状态
		if(content_length != 0){
			check_state = CHECK_STATE_CONTENT;
			// 未完成
			return NOT_COMPLETED;
		}
		// 完整
		return GET_REQUEST;
	}
	// 处理connection头部字段
	else if(check_and_move(text, "Connection:")){
		if(strcasecmp(text, "keep-alive") == 0){
			linger = true;
		}
		else if(strcasecmp(text, "close") == 0){
			linger = false; // 操作（写回复报文）完后关闭连接
		}
	}
	// 将希望接受的类型告诉服务器
	else if(check_and_move(text, "Accept:")){
		accept = text;
	}
	// 希望的字符集
	else if(check_and_move(text, "Accept-Charset:")){
		accept_charset = text;
	}
	// 希望的编码格式
	else if(check_and_move(text, "Accept-Encoding:")){
		accept_encoding = text;
	}
	// 希望的语言
	else if(check_and_move(text, "Accept-Language:")){
		accept_language = text;
	}
	else if(check_and_move(text, "Cookie")){
		cookie = text;
	}
	// 处理Content-Length头部字段
	else if(check_and_move(text, "Content-Length:")){
		content_length = atol(text);
	}
	// 处理Host头部字段
	else if(check_and_move(text, "Host:")){
		host = text;
	}
	else if(check_and_move(text, "Date:")){
		date = text;
	}
	else if(check_and_move(text, "Client-IP:")){
		client_ip = text;
	}
	else if(check_and_move(text, "From:")){
		client_email = text;
	}
	else if(check_and_move(text, "Referer:")){
		referer = text;
	}
	else if(check_and_move(text, "User-Agent:")){
		user_agent = text;
	}
	else{
		printf("oops! unknow header %s\n", text);
	}
	
	// 未完成
	return NOT_COMPLETED;
}


// 这里并没有真正解析HTTP请求的消息体，只是判断其是否被完整地读入了
pink_http_machine::HTTP_CODE pink_http_machine::parse_content(char *text){
	// 至此, checked_idx 是请求行+所有头部的长度
	if(*m_read_idx >= (checked_idx + content_length)){
		text[content_length] = '\0';
		return GET_REQUEST;
	}
	return NOT_COMPLETED;
}

// 当得到一个完整，正确的HTTP请求时，我们就开始分析目标文件。
// 如果目标存在，对所有用户可读，且不是目录，就用mmap将其映射到m_file_address处。
pink_http_machine::HTTP_CODE pink_http_machine::do_request(){

	strcpy(request_file, conf.doc_root);
	int len = strlen(conf.doc_root);
	int url_len = strlen(url);

	if(url_len > FILENAME_LEN - len - 1){
		return BAD_REQUEST;
	}
	strncpy(request_file + len, url, url_len);
	request_file[len + url_len] = '\0';

	// 成功返回0，失败返回-1
	if(stat(request_file, &file_stat) < 0){
		return NO_RESOURCE;
	}
	
	// S_IROTH：S_ Inode Read Other，其他用户具有可读取权限
	if(!(file_stat.st_mode & S_IROTH)){
		// 无读取权限
		return FORBIDDEN_REQUEST;
	}

	// 是文件夹返回1，不是返回0
	if(S_ISDIR(file_stat.st_mode)){
		return BAD_REQUEST;
	}

	int fd = open(request_file, O_RDONLY);
	// 第一个 NULL 表示让系统自动选定 start 地址
	// PROT_READ： 映射区域可被读取
	// MAP_PRIVATE： 私有的进行map，进行的是写时复制，
	// 本进程对映射区域的改变对硬盘上的文件以及其他映射这个文件的进程来说是不可见的。
	// 最后一个参数：offset = 0
	file_address = (char*)mmap(NULL, file_stat.st_size, PROT_READ,
									MAP_PRIVATE, fd, 0);
	close(fd);

	return FILE_REQUEST;
}

// ========================================================================================
// process_write 相关=======================================================================

// 根据服务器处理HTTP请求的结果，决定返回给客户端的内容
bool pink_http_machine::process_write(HTTP_CODE ret, struct iovec *iv, int &iv_count){

	switch(ret){
		case INTERNAL_ERROR:{
			add_status_line(500, error_500_title);
			add_headers(strlen(error_500_form));
			if(!add_content(error_500_form)){
				return false;
			}
			break;
		}
		// 请求无法解析
		case BAD_REQUEST:{
			add_status_line(400, error_400_title);
			add_headers(strlen(error_400_form));
			if(!add_content(error_400_form)){
				return false;
			}
			break;
		}
		// 未找到资源
		case NO_RESOURCE:{
			add_status_line(404, error_404_title);
			add_headers(strlen(error_404_form));
			if(!add_content(error_404_form)){
				return false;
			}
			break;
		}
		// 没有权限，拒绝提供服务
		case FORBIDDEN_REQUEST:{
			add_status_line(403, error_403_title);
			add_headers(strlen(error_403_form));
			if(!add_content(error_403_form)){
				return false;
			}
			break;
		}
		// 请求成功
		case FILE_REQUEST:{
			add_status_line(200, ok_200_title);
			if(file_stat.st_size != 0){
				add_headers(file_stat.st_size);
				iv[0].iov_base = m_write_buf;
				iv[0].iov_len = *m_write_idx;
				iv[1].iov_base = file_address;
				iv[1].iov_len = file_stat.st_size;
				iv_count = 2;
				return true;
			}
			else{
				const char *ok_string = "<html><body></body></html>";
				add_headers(strlen(ok_string));
				if(!add_content(ok_string)){
					return false;
				}
			}
		}
		default:{
			return false;
		}
	}
	
	iv[0].iov_base = m_write_buf;
	iv[0].iov_len = *m_write_idx;
	iv_count = 1;
	return true;
}

// 工具函数，往写缓冲区中写入待发送的数据
bool pink_http_machine::add_response(const char *format, ...){
	if(*m_write_idx >= m_write_buf_size){
		return false;
	}
	va_list arg_list;
	va_start(arg_list, format);
	// 以format格式把arg_list里的参数输出到字符串中
	// vsnprinf(): Write formatted data from variable argument list to sized buffer
	int len = vsnprintf(m_write_buf + *m_write_idx, m_write_buf_size - 1 - *m_write_idx, format, arg_list);
	// The number of characters that would have been written if n had been sufficiently large, 
	// not counting the terminating null character. If an encoding error occurs, a negative number is returned.
	if(len + 1 > (m_write_buf_size - 1 - *m_write_idx)){
		return false;
	}
	*m_write_idx += len;
	va_end(arg_list);

	return true;
}

bool pink_http_machine::add_status_line(int status, const char *title){
	return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool pink_http_machine::add_headers(int content_len){
	add_content_length(content_len);
	add_linger();
	add_blank_line();
}

bool pink_http_machine::add_content_length(int content_len){
	return add_response("Content-Length: %d\r\n", content_len);
}

bool pink_http_machine::add_linger(){
	return add_response("Connection: %s\r\n", (linger == true) ? "keep-alive" : "close");
}

bool pink_http_machine::add_blank_line(){
	return add_response("%s", "\r\n");
}

bool pink_http_machine::add_content(const char *content){
	return add_response("%s", content);
}

bool pink_http_machine::get_linger(){
	return linger;
}

void pink_http_machine::unmap(){
	if(file_address){
		munmap(file_address, file_stat.st_size);
		file_address = nullptr;
	}
}