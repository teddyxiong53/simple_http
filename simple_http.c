#include "simple_http.h"
#include "mylog.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#ifdef USE_RTOS
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <errno.h>

/*
    host 可以是点分十进制的ip地址，也可以是baidu.com这样的地址。
*/
static int _connect(struct simple_http *this, char *host, short port)
{
    struct sockaddr_in serveraddr = {0};
    struct in_addr s;
    int ret;
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, host, (void *)&serveraddr.sin_addr);
    serveraddr.sin_port = htons(port);

    this->sock = socket(AF_INET, SOCK_STREAM, 0);
    if(this->sock < 0) {
        myloge("create socket fail");
        goto end;
    }
    ret = connect(this->sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if(ret < 0) {
        myloge("connect fail");
        goto end;
    }
    strcpy(this->host, host);
    this->port = port;
    return 0;
end:
    return -1;
}
static int webclient_strncasecmp(const char *a, const char *b, size_t n)
{
    uint8_t c1, c2;
    if (n <= 0)
        return 0;
    do {
        c1 = tolower(*a++);
        c2 = tolower(*b++);
    } while (--n && c1 && c1 == c2);
    return c1 - c2;
}

static const char *webclient_strstri(const char* str, const char* subStr)
{
    int len = strlen(subStr);

    if(len == 0)
    {
        return NULL;
    }

    while(*str)
    {
        if(webclient_strncasecmp(str, subStr, len) == 0)
        {
            return str;
        }
        ++str;
    }
    return NULL;
}
static const char *header_fields_get(struct simple_http *this, const char *fields)
{
    char *resp_buf = NULL;
    size_t resp_buf_len = 0;



    resp_buf = this->recv_buffer;
    while (resp_buf_len < RECV_BUF_LEN)
    {
        if (webclient_strstri(resp_buf, fields))
        {
            char *mime_ptr = NULL;

            /* jump space */
            mime_ptr = strstr(resp_buf, ":");
            if (mime_ptr != NULL)
            {
                mime_ptr += 1;

                while (*mime_ptr && (*mime_ptr == ' ' || *mime_ptr == '\t'))
                    mime_ptr++;

                return mime_ptr;
            }
        }

        if (*resp_buf == '\0')
            break;

        resp_buf += strlen(resp_buf) + 1;
        resp_buf_len += strlen(resp_buf) + 1;
    }

    return NULL;
}
static void *_send_thread_proc(void *arg)
{
    struct simple_http *this = (struct simple_http *)arg;
    int ret;
    while(1) {
        //等待消息。
        mylogd("");
        pthread_mutex_lock(&this->send_mutex);
        while(!this->can_send) {
            pthread_cond_wait(&this->send_cond, &this->send_mutex);
        }
        pthread_mutex_unlock(&this->send_mutex);
        //说明收到消息了
        //进行发送
        //在send之前，有可能socket是被关闭的状态
        //所以有必要进行一次连接
        while(this->sock < 0) {
            ret = _connect(this, this->host, this->port);
            if(ret < 0) {
                myloge("connect fail");
                usleep(1000*3000);
            }
        }
        mylogd("");
        int send_len = 0;
        while(1) {
            errno = 0;
            ret = send(this->sock, this->send_buffer+send_len, this->send_buffer_len-send_len, 0);
            if(ret < 0) {
                if(errno == EINTR) {
                    continue;
                } else {
                    mylogd("socket fatal error:%d", errno);
                    break;
                }
            }
            send_len += ret;
            if(send_len >= this->send_buffer_len) {
                mylogd("send all content");
                break;
            }
        }
        this->can_send = 0;
    }
}

static void *_recv_thread_proc(void *arg)
{
    struct simple_http *this = (struct simple_http *)arg;
    int ret;
    while(1) {
        if(this->sock < 0) {
            usleep(1000*20);
            continue;
        }
        //需要先拿到header里的content-length。
        //回复里一定有content-length
        //没有就认为是异常，不处理。

        errno = 0;
        ret = recv(this->sock, this->recv_buffer, RECV_BUF_LEN, 0);
        mylogd("recv len:%d, errno:%d", ret, errno);
        if(ret == 0) {
            //说明服务端被关闭了
            mylogd("server socket is closed");
            //需要关闭socket
            close(this->sock);
            this->sock = -1;

        } else if(ret < 0) {

            if(errno == EINTR) {

                continue;
            } else {
                close(this->sock);
                this->sock = -1;
            }
        } else {
            //这个是成功收到了数据。
            mylogd("recv:%s", this->recv_buffer);

        }
    }
}

static int _submit(struct simple_http *this, char *buf, int len)
{
    memcpy(this->send_buffer, buf, len);
    this->send_buffer_len = len;
    this->can_send = 1;
    mylogd("");
    pthread_cond_signal(&this->send_cond);
}
struct simple_http *simple_http_create()
{
    int ret;
    struct simple_http *this = malloc(sizeof(*this));
    if(!this) {
        myloge("malloc fail");
        goto end;
    }
    memset(this, 0, sizeof(*this));
    this->sock = -1;
    this->can_send = 0;
    this->send_buffer_len = 0;
    this->connect = _connect;
    this->submit = _submit;
    pthread_mutex_init(&this->send_mutex, NULL);
    pthread_cond_init(&this->send_cond, NULL);
    ret = pthread_create(&this->send_thread, NULL, _send_thread_proc, this);
    if(ret) {
        myloge("pthread_create fail");
        goto end;
    }
    ret = pthread_create(&this->recv_thread, NULL, _recv_thread_proc, this);
    if(ret) {
        myloge("pthread_create fail");
        goto end;
    }

    return this;
end:
    if(this) {
        free(this);
    }
    return NULL;
}

