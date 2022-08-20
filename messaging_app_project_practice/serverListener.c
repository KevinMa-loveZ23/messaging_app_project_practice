#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>

#include"dataProcess.c"

void *getConn(void *arg)
{
    int confd = *(int *)arg;
    char buf[65535];//, sendBuf[65600]
    memset(buf, 0, sizeof(buf));
    char *sendBuf;
    struct dataProcessRet *ret;
    // memset(sendBuf, 0, sizeof(sendBuf));
    int exitFlag;
    while ((exitFlag = recv(confd, buf, sizeof(buf), 0)) != -1 && exitFlag)
    {
        // strncpy(sendBuf, readMsgAndReact(buf), sizeof(sendBuf));
        // sendBuf = readMsgAndReact(buf, confd);
        ret = readMsgAndReact(buf, confd);
        if (ret->sendToOther)
        {
            exitFlag = send(ret->confd, ret->sendStr, sizeof(ret->sendStr), 0);
        }
        if (exitFlag == -1 || exitFlag == 0)
        {
            free(ret);
            break;
        }
        // exitFlag = send(confd, sendBuf, sizeof(sendBuf), 0);
        exitFlag = send(confd, ret->retStr, sizeof(ret->retStr), 0);
        if (exitFlag == -1 || exitFlag == 0)
        {
            free(ret);
            break;
        }
        memset(buf, 0, sizeof(buf));
        free(ret);
    }
    pthread_exit(NULL);
}

int serverSetListening(unsigned int cycleTimes)
{
    pthread_t tid;
    struct sockaddr_in addr0;
    memset(&addr0, 0, sizeof(addr0));
    addr0.sin_family = AF_INET;
    addr0.sin_port = htons(1145);
    addr0.sin_addr.s_addr = htonl(INADDR_ANY);
    int lisfd;
    if ((lisfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return -1;
    }
    int vel = 1;
    setsockopt(lisfd, SOL_SOCKET, SO_REUSEADDR, (void *)&vel, sizeof(vel));
    if (bind(lisfd, (struct sockaddr *) &addr0, sizeof(addr0)) == -1)
    {
        perror("bind");
        return -1;
    }
    if (listen(lisfd, 10) == -1)
    {
        perror("listen");
        return -1;
    }
    char stopFlag = (cycleTimes == 0);
    while (stopFlag || cycleTimes--)
    {
        int confd;
        if ((confd = accept(lisfd, NULL, NULL)) == -1)
        {
            perror("accept");
        }
        else
        {
            pthread_create(&tid, NULL, &getConn, &confd);
        }
    }
    return 1;
}
int serverSetListening()
{
    return serverSetListening(0);
}