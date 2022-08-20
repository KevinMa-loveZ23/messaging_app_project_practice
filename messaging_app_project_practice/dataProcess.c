#include<stdio.h>
#include<string.h>
#include<sqlite3.h>

const char *dbPath = "generalDB.db";

// #include"serverResponse.c"
// #include"databaseAction.c"

const char *signUp(const char *userName, const char *password);
const char *logIn(const int userID, const char *password, int confd);
struct sendRet *addFriend(const int sender, const int receiver);//TO DO, const char *msgText
struct sendRet *sendMsg(const int sender, const int receiver, const char *timeStamp, const char *msgText);
void logOUt(const int userID);

struct sendRet
{
    char retStr[16], sendStr[65535];
    char sendToOther;
    int confd;
};

struct dataProcessRet
{
    char retStr[65535], sendStr[65535];
    char sendToOther;
    int confd;
};
struct dataProcessRet *makeRetAndSend(struct sendRet *msgRet)
{
    struct dataProcessRet *ret = malloc(sizeof(struct dataProcessRet));
    memset(ret->retStr, 0, sizeof(ret->retStr));
    strcpy(ret->retStr, msgRet->retStr);
    if (ret->sendToOther = msgRet->sendToOther)
    {
        ret->confd = msgRet->confd;
        memset(ret->sendStr, 0, sizeof(ret->sendStr));
        strcpy(ret->sendStr, msgRet->sendStr);
    }
    free(msgRet);
    return ret;
}
struct dataProcessRet *makeRet(const char *retStr)
{
    struct dataProcessRet *ret = malloc(sizeof(struct dataProcessRet));
    memset(ret->retStr, 0, sizeof(ret->retStr));
    strcpy(ret->retStr, retStr);
    ret->sendToOther = 0;
    return ret;
}

struct dataProcessRet *readMsgAndReact(const char *msg, int confd)
{
    unsigned short opr;
    char actionMsg[65535];
    memset(actionMsg, 0, sizeof(actionMsg));
    sscanf(msg, "%d|%s", &opr, actionMsg);
    switch (opr)
    {
    case 1://注册
        char userName[64], password[64];
        memset(userName, 0, sizeof(userName));
        memset(password, 0, sizeof(password));
        sscanf(actionMsg, "%[^|]|%s", userName, password);
        return makeRet(signUp(userName, password));
        // break;
    case 2://登录
        int userID;
        char password[64];
        // memset(userID, 0, sizeof(userID));
        memset(password, 0, sizeof(password));
        sscanf(actionMsg, "%d|%s", &userID, password);
        return makeRet(logIn(userID, password, confd));
        // break;
    case 3://添加好友
        int sender, receiver;
        // char msgText[65535];
        // memset(msgText, 0, sizeof(msgText));
        sscanf(actionMsg, "%d|%d", &sender, &receiver);//|%s , msgText
        return makeRetAndSend(addFriend(sender, receiver));//, msgText
        // break;
    case 4://发送信息
        int sender, receiver;
        char timeStamp[32], msgText[65535];
        // memset(sender, 0, sizeof(sender));
        // memset(receiver, 0, sizeof(receiver));
        memset(timeStamp, 0, sizeof(timeStamp));
        memset(msgText, 0, sizeof(msgText));
        sscanf(actionMsg, "%d|%d|%[^|]|%s", &sender, &receiver, timeStamp, msgText);
        return makeRetAndSend(sendMsg(sender, receiver, timeStamp, msgText));
        // break;
    
    default:
        break;
    }
    return "";
}

int signUpCallBack(void *data, int argc, char **argv, char **azColName)
{
    sscanf(argv[0], "%d", (int *)data);
    return 0;
}

const char *signUp(const char *userName, const char *password)
{
    sqlite3 *handler;
    char *errorMsg;
    char sql[256];
    // memset(retVal, 0, sizeof(retVal));
    int retIntVal;
    memset(sql, 0, sizeof(sql));
    if (sqlite3_open(dbPath, &handler) != 0)
    {
        printf("error");
        return "";
    }
    sprintf(sql, "insert into UserInfo (password, userName) values ('%s', '%s'); select max(userID) from UserInfo;", password, userName);
    if (sqlite3_exec(handler, sql, signUpCallBack, &retIntVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        return "";
    }
    sqlite3_close(handler);
    char retStrVal[16];
    sprintf(retStrVal, "1|0|%d", retIntVal);
    return retStrVal;
}

struct logInData
{
    int intVal, uid;
    char *strVal;
    char strValn[64];
};

int logInCallBack_existID(void *data, int argc, char **argv, char **azColName)
{
    sscanf(argv[0], "%d", (int *)data);
    return 0;
}

int logInCallBack_getPassword(void *data, int argc, char **argv, char **azColName)
{
    // strncpy((char *)data, argv[0],sizeof((char *)data));
    if (strcmp(azColName[0], "password"))
    {
        strcpy(((struct logInData *)data)->strValn, argv[0]);
        strcpy(((struct logInData *)data)->strVal, argv[1]);
    }
    else
    {
        strcpy(((struct logInData *)data)->strVal, argv[0]);
        strcpy(((struct logInData *)data)->strValn, argv[1]);
    }
    return 0;
}

int logInCallBack_getFriends(void *data, int argc, char **argv, char **azColName)
{
    unsigned short uid1, uid2;
    const char *unm1, *unm2;
    char flag1 = 1, flag2 = 1, flag3 = 1;
    for (int i = 0; i < argc; i++)
    {
        if (flag1 && strcmp(azColName[i], "userID1") == 0)
        {
            flag1 = 0;
            sscanf(argv[i], "%d", &uid1);
        }
        else if (flag2 && strcmp(azColName[i], "userName1") == 0)
        {
            flag2 = 0;
            unm1 = argv[i];
        }
        else if (flag3 && strcmp(azColName[i], "userID2") == 0)
        {
            flag3 = 0;
            sscanf(argv[i], "%d", &uid2);
        }
        else
        {
            unm2 = argv[i];
        }
    }
    if (uid1 == ((struct logInData *)data)->uid)
    {
        // strcat(((struct logInData *)data)->strVal, );
        sprintf(((struct logInData *)data)->strVal, "%s|%d|%s", ((struct logInData *)data)->strVal, uid2, unm2);
    }
    else
    {
        sprintf(((struct logInData *)data)->strVal, "%s|%d|%s", ((struct logInData *)data)->strVal, uid1, unm1);
    }
    ((struct logInData *)data)->intVal++;
    return 0;
}

const char *logIn(const int userID, const char *password, int confd)
{
    sqlite3 *handler;
    char *errorMsg;
    char sql[256];
    int retIntVal;
    char retStrVal[65535];
    memset(sql, 0, sizeof(sql));
    memset(retStrVal, 0, sizeof(retStrVal));
    if (sqlite3_open(dbPath, &handler) != 0)
    {
        printf("error");
        return "";
    }
    // sprintf(sql, "select max(userID) from UserInfo;");
    if (sqlite3_exec(handler, "select max(userID) from UserInfo;", logInCallBack_existID, &retIntVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        return "";
    }
    if (userID > retIntVal)
    {
        return "2|1";
    }

    sprintf(sql, "select password, userName from UserInfo where userID = %d;", userID);
    struct logInData structData;
    structData.strVal = retStrVal;
    structData.uid = userID;
    memset(structData.strValn, 0, sizeof(structData.strValn));
    if (sqlite3_exec(handler, sql, logInCallBack_getPassword, &structData, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        return "";
    }
    if (strcmp(retStrVal, password) != 0)
    {
        return "2|2";
    }

    memset(retStrVal, 0, sizeof(retStrVal));
    // sprintf(retStrVal, "2|%s|0|", structData.strValn);
    // memset(structData.strValn, 0, sizeof(structData.strValn));
    // sprintf(structData.strValn, "%d", userID);
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select * from Friendship where userID1 = %d or userID2 = %d;", userID, userID);
    structData.intVal = 0;
    if (sqlite3_exec(handler, sql, logInCallBack_getFriends, &structData, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        return "";
    }
    sprintf(retStrVal, "2|0|%s|%d%s", structData.strValn, structData.intVal, retStrVal);
    //// TO DO: 记录confd，用于转发信息
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "updata UserInfo set confd = %d where userID = %d;", confd, userID);
    if (sqlite3_exec(handler, sql, NULL, NULL, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        return "";
    }
    
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select msgWait from UserInfo where userID = %d", userID);
    if (sqlite3_exec(handler, sql, logInCallBack_existID, &retIntVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        return "";
    }
    if (retIntVal)
    {
        //把离线时的信息一口气扔回去
    }

    sqlite3_close(handler);
    return retStrVal;
}

int addFriendCallBack_getName(void *data, int argc, char **argv, char **azColName)
{
    strcpy((char *)data, argv[0]);
    return 0;
}

struct sendRet *addFriend(const int sender, const int receiver)
{
    sqlite3 *handler;
    char *errorMsg;
    char sql[256];
    int retIntVal;
    char retStrVal[64];
    struct sendRet *ret;
    if (sqlite3_open(dbPath, &handler) != 0)
    {
        printf("error");
        return "";
    }
    // sprintf(sql, "select max(userID) from UserInfo;");
    if (sqlite3_exec(handler, "select max(userID) from UserInfo;", logInCallBack_existID, &retIntVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        return "";
    }
    if (receiver > retIntVal)
    {
        // "3|1"
        ret = malloc(sizeof(struct sendRet));
        ret->sendToOther = 0;
        memset(ret->retStr, 0, sizeof(ret->retStr));
        strcat(ret->retStr, "3|1");
        return ret;
    }
    //可以加上防重复检测在这，暂时没有

    char senderName[64], receiverName[64];
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select userName from UserInfo where userID = %d;", sender);
    memset(retStrVal, 0, sizeof(retStrVal));
    memset(senderName, 0, sizeof(senderName));
    if (sqlite3_exec(handler, sql, addFriendCallBack_getName, retStrVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        return "";
    }
    strcpy(senderName, retStrVal);
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select userName from UserInfo where userID = %d;", receiver);
    memset(retStrVal, 0, sizeof(retStrVal));
    memset(receiverName, 0, sizeof(receiverName));
    if (sqlite3_exec(handler, sql, addFriendCallBack_getName, retStrVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        return "";
    }
    strcpy(receiverName, retStrVal);
    
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "insert into Friendship (userID1, userName1, userID2, userName2) values (%d, '%s', %d, '%s');", sender, senderName, receiver, receiverName);
    if (sqlite3_exec(handler, sql, NULL, NULL, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        return "";
    }
    // sprintf(sql, "insert into Friendship ()", );
}