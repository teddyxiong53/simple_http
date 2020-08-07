
#include "simple_http.h"
#include "mylog.h"
#include <unistd.h>

char *gen_httpbin_get()
{
    char * buf = malloc(1024);
    memset(buf, 0, 1024);
    char *p = buf;
    strcpy(buf, "GET / HTTP/1.1\r\n");
    // strcat(buf, "Connection: keep-alive\r\n");
    // strcat(buf, "Content-Type: text/html\r\n");
    strcat(buf, "Cache-Control: no-cache\r\n");
    strcat(buf, "User-Agent: xhl\r\n");
    strcat(buf, "Host: httpbin.org\r\n");
    strcat(buf, "Accept: */*\r\n");
    strcat(buf, "Accept-Encoding: gzip, deflate\r\n");
    strcat(buf, "\r\n");
    //body
    return buf;

}

int main(int argc, char const *argv[])
{
    struct simple_http *http = simple_http_create();
    if(!http) {
        mylogd("simple_http_create fail");
        return -1;
    }
    // http->connect(http, "smartdevice.ai.tuling123.com", 80);
    // http->connect(http, "127.0.0.1", 8080);

    http->connect(http, "54.236.246.173", 80);//httpbin.org
    char *content = gen_httpbin_get();
    http->submit(http, content, strlen(content));

    // http->submit(http, content, strlen(content));
    while(1) {
        usleep(1000);
    }
    return 0;
}
