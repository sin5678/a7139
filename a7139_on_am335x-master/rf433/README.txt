说明文档

1，程序描述
    rf433模块为KJ98-F(C)项目的433无线传感器收发软件模块，提供了RSWP433无线协议和433无线报文转UDP报文的功能。
    rf433模块分为两个程序，rfrepeater负责RSWP433协议处理，UDP报文转发的工作；rfcli负责与rfrepeater进行通信，支持进行433和socket配置和获取程序状态参数的功能；
    
2，文件说明
    buffer.c            -   缓存管理库源程序
    buffer.h            -   缓存管理库头文件
    common.c            -   通用函数库源程序
    common.h            -   通用函数库头文件
    list.h              -   链表头文件
    Makefile            -   Makefile
    README.txt          -   模块说明
    rf433lib.c          -   rf433中间层库源程序
    rf433lib.h          -   rf433中间层库头文件
    rf433.sh*           -   rf433系统脚本
    rfcli.c             -   用户使用接口源程序
    rfcli.h             -   用户使用接口头文件
    rfrepeater.c        -   协议处理转发源程序
    rfrepeater.h        -   协议处理转发头文件

3，编译说明
    在模块根目录下，执行make，可执行文件将生成在模块目录下，执行make install，将rfrepeater和rfcli进行strip后，安装至rootfs目录中。
