#pragma once
#include <pthread.h>

// #define USE_RTOS

#define SEND_BUF_LEN 2048
#ifdef USE_RTOS
#define RECV_BUF_LEN 2048
#else
#define RECV_BUF_LEN 20480
#endif
#define HOST_LEN  48

struct simple_http {
    int sock;
    char host[HOST_LEN];//保存，用来重连的时候用。
    short port;
    int (*connect)(struct simple_http *this, char *host, short port);
    int (*submit)(struct simple_http *this, char *buf, int len);//提交数据给send线程发送
    pthread_t send_thread;
    pthread_cond_t send_cond;
    pthread_mutex_t send_mutex;
    int can_send;
    char send_buffer[SEND_BUF_LEN];
    int send_buffer_len;
    pthread_t recv_thread;
    char recv_buffer[RECV_BUF_LEN];
    int content_length;//这个是为了处理答复的内容，需要先取到这个部分。主要是为了应对一次recv不完的情况，其实当前我的场景，回复都很短。加上以防万一。
};

struct simple_http *simple_http_create();

