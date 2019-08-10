c11 = g++ -std=c++11

cur_dir = $(shell pwd)
src_dir = $(cur_dir)/src
obj_dir = $(cur_dir)/obj

all: pink

pink: $(obj_dir)/main.o $(obj_dir)/pink_http_conn.o $(obj_dir)/pink_http_machine.o $(obj_dir)/pink_epoll.o  $(obj_dir)/socket_tool.o $(obj_dir)/basic_tool.o $(obj_dir)/pink_epoll_tool.o
	$(c11) -pthread -o pink $^

$(obj_dir)/main.o: $(src_dir)/main.cpp
	$(c11) -o $@ -c $< 

$(obj_dir)/pink_epoll.o: $(src_dir)/pink_epoll.cpp
	$(c11) -c $< -o $@

$(obj_dir)/pink_http_conn.o: $(src_dir)/pink_http_conn.cpp
	$(c11) -c $< -o $@

$(obj_dir)/pink_http_machine.o: $(src_dir)/pink_http_machine.cpp
	$(c11) -c $< -o $@

$(obj_dir)/socket_tool.o: $(src_dir)/tools/socket_tool.cpp
	$(c11) -c $< -o $@

$(obj_dir)/basic_tool.o: $(src_dir)/tools/basic_tool.cpp
	$(c11) -c $< -o $@

$(obj_dir)/pink_epoll_tool.o: $(src_dir)/tools/pink_epoll_tool.cpp
	$(c11) -c $< -o $@

clean:
	rm -rf $(obj_dir)/*.o pink

