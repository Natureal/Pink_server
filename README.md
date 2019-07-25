### Pink: A High Performance HTTP Server

**Navigator**:
**[架构与模块](https://github.com/Natureal/Pink_server/blob/master/knowledge/architecture.md)** || **[压力测试](https://github.com/Natureal/Pink_server/blob/master/knowledge/evaluation.md)** || **[问题记录](https://github.com/Natureal/Pink_server/blob/master/knowledge/problems.md)** || **[基础知识](https://github.com/Natureal/Pink_server/blob/master/knowledge/basic.md)**


**Motivation**: 翻阅了书本以及学习了一些开源项目后，决定自己写一个 server，串联知识，探索一些小想法。

---

### Tools

- **1. 开发环境**

(1) Ubuntu 18.04 (Linux Core 4.15)

(2) Intel i5-8250U (1.6(max: 3.4)GHz * 8)

- **2. 开发工具**

(1) Vim + Sublime Text 3 (C++11)

(2) CMake + ctags (源码定位器)

(3) doxygen (强大的调用关系图工具) [Reference](https://blog.csdn.net/ZeroLiko/article/details/78162408)


- **3. 测压工具**

(1) 单线程 I/O 复用方式: ./test/pressure_test.cpp

(2) 多进程并发方法: **[webbench](http://home.tiscali.cz/~cz210552/webbench.html)**

(3) 多线程 I/O 复用方式 (最高压力):  **[wrk](https://github.com/wg/wrk)**

---

### Configuration

- 端口号: 7777

- 页面根目录: ./web

- 连接池初始连接数: 2000（最大: 10K）

- 线程池线程数量: 4

- 非活动连接超时时间: 10s

- 定期处理超时事件: 5s

- Epoll 模式: et / lt

---

### To do list:

1. 实现自销毁的线程池(智能指针) **Finished v1.0**

2. 添加定时器，实现自销毁的时间堆 **Finished v1.0**

3. 优化定时器触发机制，学习内核 hrtimer

4. 实现自销毁的连接池 **Finished v1.0**

5. 实现内存池（学习 STL memory pool/tcmalloc/jemalloc）**doing**

6. 优化并行模式 -> 优化的 Reactor 模式，省略一次性完成的写完成事件注册 **Finished v1.0**

7. 生产者消费者，降低耦合 **Finished v1.0**

8. 守护进程配置

9. 在线修改配置参数

10. 实现负载均衡（学习NginX）

11. 考虑[线程池惊群问题](https://github.com/Natureal/Pink_server/blob/master/knowledge/%E6%83%8A%E7%BE%A4%E9%97%AE%E9%A2%98.md) **doing**

12. 探索 Proactor 模式: 完全异步 + 非阻塞模式 (AIO)

13. 考虑 pipeline 技术

14. 日志系统
