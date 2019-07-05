c11 = g++ -std=c++11

cur_dir = $(shell pwd)
obj_dir = $(cur_dir)/obj

all: pink

pink: $(obj_dir)/main.o $(obj_dir)/pink_epoll.o $(obj_dir)/pink_http_conn.o $(obj_dir)/socket_tool.o $(obj_dir)/basic_tool.o
	$(c11) -pthread -o pink $^

$(obj_dir)/main.o: main.cpp
	$(c11) -c $< -o $@

$(obj_dir)/pink_epoll.o: pink_epoll.cpp
	$(c11) -c $< -o $@

$(obj_dir)/pink_http_conn.o: pink_http_conn.cpp
	$(c11) -c $< -o $@

$(obj_dir)/socket_tool.o: tools/socket_tool.cpp
	$(c11) -c $< -o $@

$(obj_dir)/basic_tool.o: tools/basic_tool.cpp
	$(c11) -c $< -o $@


clean:
	rm -rf $(obj_dir)/*.o pink

