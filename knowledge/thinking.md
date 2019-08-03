## 一些思考

1. ET 比 LT 快多少？

2. EPOLLONESHOT 的实现机制？

参考：https://www.cnblogs.com/charlesblc/p/5538363.html

“注意在一个线程处理完一个 socket 的数据，也就是触发 EAGAIN errno 时候，就应该重置 EPOLL_ONESHOT 的 flag，这时候，新到的事件，就可以重新进入触发流程了。

EPOLL_ONESHOT 的原理其实是，每次触发事件之后，就将事件注册从fd上清除了，也就不会再被追踪到；下次需要用 epoll_ctl 的 EPOLL_CTL_MOD 来手动加上才行。”

因此在项目中，pink_http_conn.cpp 的 process 函数中，如果 read 完缓冲并解析完发现请求不完整（NOT_COMPLETED），这时需要重新注册事件，pink_epoll_modfd。

