#ifndef __MYLOG_H__
#define __MYLOG_H__
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif



#define mylogd(...) do {\
                            printf("[xhl][DEBUG][%s][%d]: ", __func__, __LINE__);\
                            printf(__VA_ARGS__);\
                            printf("\n");\
                           }while(0)
#define myloge(...) do {\
                            printf("[xhl][ERROR][%s][%d]: ",  __func__, __LINE__);\
                            printf(__VA_ARGS__);\
                            printf("\n");\
                           }while(0)


#ifdef __cplusplus
}
#endif
#endif
