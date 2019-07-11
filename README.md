# Pink: A High Performance HTTP Server

## [基础知识](https://github.com/Natureal/Pink_server/blob/master/knowledge/README.md)


Motivation: 学习了书本上和一些开源小项目后，决定自己写一个 http server，实现一些自己的想法。并给它取了一个冷艳的名字: Pink。最终的期望它是一个优雅高效独特的 Server. :)

## tools

1. 测压工具 <br>
(1) ./test/pressure_test.cpp <br>
(2) 开源工具 webbench <br>

2. 开发工具 <br>
(1) ctags 源码定位工具 <br>
(2) Sublime Text 3 <br>


## to do list:

1. 将线程池设置成自销毁(智能指针) Finished <br>

2. 封装一个自销毁的连接池(智能指针) Finished <br>

3. 实现定时器 <br>

4. 实现内存池 <br>

5. 优化并行模式 <br>

6. 模板封装，降低耦合 To be continued <br>

7. 守护进程配置 <br>

8. 在线修改配置参数 <br>

9. 日志系统 <br>
