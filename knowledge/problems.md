# 碰到的问题

### 0. epoll 中的 listenfd 不能注册 EPOLLONESHOT 事件！

### 1. 指针的引用
在 pink_http_machine.cpp 中，传递给工具函数 check_and_move() 的 text 指针必须是引用形式，否则无法在函数内对其本身的值进行更改

### 2. 静态常量类成员的类外声明
在 pink_http_machine.h 中，静态成员 FILENAME_LEN 由于声明为常量型，所以可以在类内直接初始化，但是还应该在类外声明以下（此时不用带 static）。

### 3. 类内 enum 的声明
在 pink_http_machine.h 中，类内写的 enum 只是一种声明，并非定义，所以它们不占内存。

### 4. 使用 using std::xxx 简化代码
在 main.cpp 以及其他代码中，可以考虑多使用 using 来简化代码

### 5. 使用 swap 来释放 vector 所占内存
在 main.cpp 中，用 swap 来释放 user 指向的 vector，并且在交换后记得令 user=nullptr，防止出现野指针。

### 6. switch 的判断变量
只能用表达式，整形/布尔型/字符型/枚举型。

### 7. size_t
其类型为 unsigned int / unsigned long

### 8. goto 语句
goto 有一个非常致命的缺点：不能跳过变量初始化。否则有:crosses initialization of ... 错误。

### 9. perror
作用:将上一个函数发生的错误原因,输出到标准设备。

### 10. extern 变量的初始化
在变量初始化的文件中不要用 extern 了，否则会报错（或者警告）。

### 11. webbench 的请求报文格式
由于 HTTP/1.1 普遍采用 Host 字段，所以原来的 URL 只是即相对资源路径。<br>
Host 是 HTTP/1.1 特有的，具体和 1.0 的区别可以参考：https://www.cnblogs.com/sue7/p/9414311.html

### 12. webbench 中的 write 和 read 为阻塞
在第一次测试的时候 webbench 一直卡住，查了源码发现是其用了 read 阻塞读。

### 13. 关闭连接时忘记 close(fd)

### 14. EPOLLRDHUP 不可靠！
在给监听 socket 开启 EPOLLRDHUP 后，客户端一连接上并发数据，这个错误就跳出来。
EPOLLRDHUP indeed comes if you continue to poll after receiving a zero-byte read.

参考：https://stackoverflow.com/questions/27175281/epollrdhup-not-reliable

### 15. recv(, , 0) 返回 0，在尚未读入数据的情况下
解决方法：给 recv() 的最后一个参数设置成 MSG_WAITALL

### 16. epoll 中监听套接字的触发模式很重要
ET/LT 的区别

### 17. shared_ptr 操作数组的困难
参考1：https://www.cnblogs.com/xiaoshiwang/p/9726511.html

数组的智能指针的限制:
1，unique_ptr的数组智能指针，没有*和->操作，但支持下标操作[]
2，shared_ptr的数组智能指针，有*和->操作，但不支持下标操作[]，只能通过get()去访问数组的元素。
3，shared_ptr的数组智能指针，必须要自定义deleter

### 18. 静态函数指针的初始化
在 pink_http_conn 中设置的静态回调函数(用于放回连接到连接池)指针需要在 cpp 中进行初始化。

### 19. 连接池链表需要线程安全
通过互斥锁实现连接体链表的线程安全

### 20. 如果将时间堆分离出一个头文件和cpp会出现 undefined referrence to 链接问题（待解决）

### 21. unique_ptr 的初始化方式与 shared_ptr 不同
可以先赋值为 nullptr，再用 unique_ptr.reset(new object) 的形式

### 22. 工作线程逻辑的优化
在解析完 HTTP 请求报文后尝试写出响应报文，只有当该次非阻塞 write 返回 EAGAIN 才注册 EPOLLOUT 事件，以提高效率。

### 23. 关闭连接体的操作应该采用统一的接口
全部统一到 pink_http_conn.close_conn() 函数中。

### 24. 线程池中线程结束资源释放问题
工作线程全部设置为 detached 模式（通过 pthread_detach），从而避免使用 pthread_join，防止资源泄露。

使用 valgrind 检查内存泄露时，会提醒使用 pthread_create 可能会泄露内存（possibly lost)，这就提醒我们用 detached 模式/及时 join。

参考更多：https://stackoverflow.com/questions/5610677/valgrind-memory-leak-errors-when-using-pthread-create