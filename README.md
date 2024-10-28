# IPv4_PROJ
基于IPv4的流媒体广播项目
单工的通信：server 没有接受c端的输入或者选择情况；S端只是一直发送数据，C段只需要从这些发送来的数据选择接收。

# 问题


虚拟机上前期流畅，后面开始跳频
在虚拟机上，虚拟机跑在一个核上，难以体现并发。虚拟机外接设备可能与真机有区别，播放会出现断断续续的。
虚拟机会不停校验声卡所使用的时间轴，导致声音断开，真机基本没有啥问题。



真机有一个小问题 真机可能会出现停顿一段时间

【mdialib】中的Mytbf_init中每秒钟传输的字节个数跟当前每秒钟通过channel线程往socket发数据的大小有关系。
真机可能会出现停顿一段时间，每秒钟传输的内容不满足当前播放的速率；c端有管道，父进程从网络中接受的数据发送给子进程是通过管道父子间通讯的工具发送的。

S端改进
可将mytbf_init的传输速率多给一些（MP3_BITRATE/8*3），使管道中一直有待播放的数据存在，不会出现播完一句，只能空等数据

C端改进
可以自己做一个缓冲机制，将接收完的数据将要写管道的时候先不写，先存到缓冲区中，当缓冲区中的数据达到一定字节数量开始向管道中写。从而确保子进程不会出现播放量不足的情况。


# 项目改进方向
C端图形界面
C端使用图形界面实现：使用QT开发一个C端，拉去图形界面

S端数据库
媒体库原来实现是使用当前的文件系统（Linux中存放用户信息组信息都是使用文件系统），可以引入数据库MySQL；将信息分频道或者分类存放，读取内容的时候利用C与MySQL的接口从库中取内容。


多点通信

视频解码器

解码器更改：mplayer

可以将server_conf.h中的目录文件地址更改，地址是.avi的视频文件。


点播系统

Server：更改节目单包含各种视频或者音频资料；

C端：指定某一个音频或者视频播放

S通过socket将指定数据传给C端，C端解码器播放。


C端用户多向S端请求过多，则使用多进程多线程的并发；可以构建集群（对外看到s端ip，但背后有众多的服务器，根据每一台服务器所运行的状态和承载能力，将当前用户要求相应的连接给某一个子进程，通过该子进程工作）。多进程（多线程）并发，进程间通信，负载均衡原理（数据中继，中继引擎的实现））。



# 额外
在视频平台观看视频的时候，会出现广告；有的利用广告这段时间将要播放的视频数据缓冲。








# 自己调试问题
1.server端无法运行守护进程
	解决：daemonize()函数没有返回值

2.真机运行依然有跳字节问题，播放不连贯
