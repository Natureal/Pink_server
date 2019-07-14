#### Pink: A High Performance HTTP Server

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

(1) 单线程 I/O 复用方式: test/pressure_test.cpp

(2) 多进程并发方法: webbench



#### To do list:

1. 实现自销毁的线程池(智能指针) Finished

2. ~~封装自销毁的连接池(智能指针) Finished~~

  (取消连接池，采用内存池(to be continued)+动态分配连接)

3. 添加定时器

4. 实现内存池

5. 优化并行模式 -> Reactor 模式 Finished

6. 生产者消费者，降低耦合 To be continued

7. 守护进程配置

8. 在线修改配置参数

9. 日志系统

10. 考虑线程池惊群问题
