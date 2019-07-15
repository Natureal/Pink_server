#### Pink: A High Performance HTTP Server

**Preview:**
基于优化的Reactor模式

**Navigator**: **[基础知识](https://github.com/Natureal/Pink_server/blob/master/knowledge/README.md)** | **[架构与模块](https://github.com/Natureal/Pink_server/blob/master/knowledge/architecture.md)** | **[压力测试](https://github.com/Natureal/Pink_server/blob/master/knowledge/evaluation.md)**


**Motivation**: 翻阅了书本以及学习了一些开源项目后，决定自己写一个 server，巩固知识，实现一些小想法。

#### Tools

- **1. 开发环境**

(1) Ubuntu 18.04 (Linux Core 4.15)

(2) Intel i5-8250U (1.6(max: 3.4)GHz * 8)

- **2. 开发工具**

(1) Vim + Sublime Text 3 (C++11)

(2) CMake + ctags (源码定位器)


- **3. 测压工具**

(1) 单线程 I/O 复用方式: ./test/pressure_test.cpp

(2) 多进程并发方法: **[webbench](http://home.tiscali.cz/~cz210552/webbench.html)**

(3) 多线程 I/O 复用方式 (最高压力):  **[wrk](https://github.com/wg/wrk)**

#### To do list:

1. 实现自销毁的线程池(智能指针) **Finished**

2. 添加定时器 **Finished**

3. 实现内存池

4. 优化并行模式 -> Reactor + 半同步半异步 **Finished**

5. 生产者消费者，降低耦合 **Finished**

6. 守护进程配置

7. 在线修改配置参数

8. 日志系统

9. 考虑线程池惊群问题

10. 探索完全异步 + 非阻塞模式 (AIO)
