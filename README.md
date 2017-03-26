# fake-slave

这个程序是一个 MySQL 数据监控器, 可以检测数据改变, 并通知到下游系统.
基于这些改变, 你可以更新缓存, 同步NoSQL数据库等.

> 程序尚未写完.

## 注意

为了避免写兼容代码, 只支持使用 MySQL 5.0.0+ 以上的版本(也就是 Binlog Versions 大于 4 的版本).

## TODO

1. checksum_binlog, 可以延后实现.
2. rows event 事件里面对数据值使用链表保存, 方便以后生成 JSON
3. 不考虑其他数据写入, 先正确的组合 JSON 输出到 stdout