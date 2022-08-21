#include<stdio.h>
#include<string.h>
#include<sqlite3.h>

const char *dbPath = "generalDB.db";

// #include"serverResponse.c"
// #include"databaseAction.c"

const char *signUp(const char *userName, const char *password);
struct dataProcessRet *logIn(const int userID, const char *password, int confd);
struct sendRet *addFriend(const int sender, const int receiver);//TO DO, const char *msgText
struct sendRet *sendMsg(const int sender, const int receiver, const char *timeStamp, const char *msgText);
void logOut(const int confd);

struct sendRet
{
    char retStr[16], sendStr[65535];//ret是返回给发信者的信息，也即反馈信息；send是发送给在线用户的其他用户的信息（离线的用户没有，信息在logIn中发送）
    char sendToOther;//被发信者在线时为1（立刻向在线用户发信），否则为0
    int confd;//被发信者的confd，从UserInfo中获取
};

struct dataProcessRet
{
    char retStr[65535];//, sendStr[65535]；retStr意义同上
    char sendToOther, sendToSelf;//ToOther意义同上，ToSelf只有logIn时可能为1（详见logIn发送离线信息）；两者不会同时为1
    int confd, cycle;//confd同上，cycle为ToSelf时的离线信息数
    char msgList[][65535];
    //当ToOther为1时，如makeREtAndSend提供65535个char大小空间，msgList[0]发挥sendStr作用；
    //当ToSelf为1时，如logIn中提供cycle*65535个char大小空间即cycle个65535大小字符数组用于储存离线信息；
    //当都为0时不占空间，相当于无sendStr
};
struct dataProcessRet *makeRetAndSend(struct sendRet *msgRet)
{
    struct dataProcessRet *ret = malloc(sizeof(struct dataProcessRet) + 65535 * sizeof(char));
    memset(ret->retStr, 0, sizeof(ret->retStr));
    strcpy(ret->retStr, msgRet->retStr);
    ret->sendToSelf = 0;
    if (ret->sendToOther = msgRet->sendToOther)
    {
        ret->confd = msgRet->confd;
        // memset(ret->sendStr, 0, sizeof(ret->sendStr));
        // strcpy(ret->sendStr, msgRet->sendStr);
        memset(ret->msgList[0], 0, sizeof(ret->msgList[0]));
        strcpy(ret->msgList[0], msgRet->sendStr);
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
    ret->sendToSelf = 0;
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
        return logIn(userID, password, confd);
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
        sqlite3_close(handler);
        return "";
    }
    sprintf(sql, "insert into UserInfo (password, userName) values ('%s', '%s'); select max(userID) from UserInfo;", password, userName);
    if (sqlite3_exec(handler, sql, signUpCallBack, &retIntVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
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
    int uid1, uid2;
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
int logInCallBack_readMsg(void *data, int argc, char **argv, char **azColName)
{
    int sender;
    const char *msg, *time;
    char flag1 = 1, flag2 = 1;
    for (int i = 0; i < argc; i++)
    {
        if (flag1 && strcmp(azColName[i], "sender") == 0)
        {
            flag1 = 0;
            sscanf(argv[i], "%d", &sender);
        }
        else if (flag2 && strcmp(azColName[i], "msg") == 0)
        {
            flag2 = 0;
            msg = argv[i];
        }
        else
        {
            time = argv[i];
        }
    }
    struct dataProcessRet *tmp = (struct dataProcessRet *)data;
    sprintf(tmp->msgList[tmp->cycle], "5|%d|%s|%s", sender, time, msg);
    tmp->cycle++;
    return 0;
}

struct dataProcessRet *logIn(const int userID, const char *password, int confd)
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
        sqlite3_close(handler);
        return ;
    }
    // sprintf(sql, "select max(userID) from UserInfo;");
    if (sqlite3_exec(handler, "select max(userID) from UserInfo;", logInCallBack_existID, &retIntVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
        return ;
    }
    if (userID > retIntVal)
    {
        sqlite3_close(handler);
        return makeRet("2|1");
    }

    sprintf(sql, "select password, userName from UserInfo where userID = %d;", userID);
    struct logInData structData;
    structData.strVal = retStrVal;
    structData.uid = userID;
    memset(structData.strValn, 0, sizeof(structData.strValn));
    if (sqlite3_exec(handler, sql, logInCallBack_getPassword, &structData, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
        return ;
    }
    if (strcmp(retStrVal, password) != 0)
    {
        sqlite3_close(handler);
        return makeRet("2|2");
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
        sqlite3_close(handler);
        return ;
    }
    sprintf(retStrVal, "2|0|%s|%d%s", structData.strValn, structData.intVal, retStrVal);
    //// TO DO: 记录confd，用于转发信息
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "update UserInfo set confd = %d where userID = %d;", confd, userID);
    if (sqlite3_exec(handler, sql, NULL, NULL, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
        return ;
    }
    
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select msgWait from UserInfo where userID = %d", userID);
    if (sqlite3_exec(handler, sql, logInCallBack_existID, &retIntVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
        return ;
    }
    struct dataProcessRet *retVal;
    if (retIntVal)
    {
        //把离线时的信息一口气扔回去
        retVal = malloc(sizeof(sizeof(struct dataProcessRet) + retIntVal * 65535 * sizeof(char)));
        memset(retVal, 0, sizeof(retVal));
        strcpy(retVal->retStr, retStrVal);
        retVal->sendToOther = 0;
        retVal->sendToSelf = 1;
        retVal->confd = confd;
        retVal->cycle = 0;
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "select sender, msg, timestamp from MessageList where receiver = %d", userID);
        if (sqlite3_exec(handler, sql, logInCallBack_readMsg, retVal, &errorMsg) != 0)
        {
            printf("%s", errorMsg);
            sqlite3_close(handler);
            return ;
        }
        memset(sql, 0, sizeof(sql));
        sprintf(sql, "update UserInfo set msgWait = 0 where userID = %d;", userID);
        if (sqlite3_exec(handler, sql, NULL, NULL, &errorMsg) != 0)
        {
            printf("%s", errorMsg);
            sqlite3_close(handler);
            return ;
        }
    }
    else
    {
        retVal = makeRet(retStrVal);
    }
    sqlite3_close(handler);
    return retVal;
}

struct friendData
{
    int confd;
    char isNotNull;
};

int addFriendCallBack_getName(void *data, int argc, char **argv, char **azColName)
{
    strcpy((char *)data, argv[0]);
    return 0;
}

int addFriendCallBack_getConfd(void *data, int argc, char **argv, char **azColName)
{
    if (((struct friendData *)data)->isNotNull = argv[0]? 1: 0)
    {
        sscanf(argv[0], "%d", &(((struct friendData *)data)->confd));
    }
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
        sqlite3_close(handler);
        return ;
    }
    // sprintf(sql, "select max(userID) from UserInfo;");
    if (sqlite3_exec(handler, "select max(userID) from UserInfo;", logInCallBack_existID, &retIntVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
        return ;
    }
    if (receiver > retIntVal)
    {
        // "3|1"
        ret = malloc(sizeof(struct sendRet));
        ret->sendToOther = 0;
        memset(ret->retStr, 0, sizeof(ret->retStr));
        strcpy(ret->retStr, "3|1");
        sqlite3_close(handler);
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
        sqlite3_close(handler);
        return ;
    }
    strcpy(senderName, retStrVal);
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select userName from UserInfo where userID = %d;", receiver);
    memset(retStrVal, 0, sizeof(retStrVal));
    memset(receiverName, 0, sizeof(receiverName));
    if (sqlite3_exec(handler, sql, addFriendCallBack_getName, retStrVal, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
        return ;
    }
    strcpy(receiverName, retStrVal);
    
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "insert into Friendship (userID1, userName1, userID2, userName2) values (%d, '%s', %d, '%s');", sender, senderName, receiver, receiverName);
    if (sqlite3_exec(handler, sql, NULL, NULL, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
        return ;
    }
    
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "select confd from UserInfo where userID = %d;", receiver);
    struct friendData tmp;
    if (sqlite3_exec(handler, sql, addFriendCallBack_getConfd, &tmp, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
        return ;
    }
    if (tmp.isNotNull)
    {
        ret = malloc(sizeof(struct sendRet));
        memset(ret, 0, sizeof(ret));
        ret->sendToOther = 1;
        ret->confd = tmp.confd;
        strcpy(ret->retStr, "3|0");
        // char sendStr[150];
        // memset(sendStr, 0, sizeof(sendStr));
        sprintf(ret->sendStr, "6|%d|%s", sender, senderName);
        // strcpy(ret->sendStr, sendStr);
    }
    else
    {
        ret = malloc(sizeof(struct sendRet));
        ret->sendToOther = 0;
        memset(ret->retStr, 0, sizeof(ret->retStr));
        strcpy(ret->retStr, "3|0");
    }
    
    // sprintf(sql, "insert into Friendship ()", );
    sqlite3_close(handler);
	return ret;
}

struct sendRet *sendMsg(const int sender, const int receiver, const char *timeStamp, const char *msgText)
 {
 	sqlite3 *handler;
 	char *errorMsg;
 	char sql[256];
 	if (sqlite3_open(dbPath, &handler) != 0)
 	{
 		printf("error");
        sqlite3_close(handler);
 		return ;
	}
    struct sendRet *ret;
	// sprintf(sql, "insert into MessageList(sender, reciever, msg, timestamp) values(%d, %d, %s, %s);", sender, reviever, msgText, timeStamp);
	// if(sqlite3_exec(handler, sql, NULL, NULL, &errorMsg) != 0)
	// {
	// 	printf("%s",errorMsg);
	// 	return ;
	// }
	// memset(sql, 0, sizeof(sql));
	// sprintf(sql, "update UserInfo set Msgwait = Msfwait + 1 where userID = %d;", reciever);
	// if(sqlite3_exec(handler, sql, NULL, NULL, &errorMsg) != 0)
	// {
	// 	printf("%s",erroMsg);
	// 	return ;
	// }
 	memset(sql, 0, sizeof(sql));
    sprintf(sql, "select confd from UserInfo where userID = %d;", receiver);
    struct friendData tmp;
    if (sqlite3_exec(handler, sql, addFriendCallBack_getConfd, &tmp, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
        return ;
    }
    if (tmp.isNotNull)
    {
        ret = malloc(sizeof(struct sendRet));
        memset(ret, 0, sizeof(ret));
        ret->sendToOther = 1;
        ret->confd = tmp.confd;
        strcpy(ret->retStr, "4|0");
        // char sendStr[65535];
        // memset(sendStr, 0, sizeof(sendStr));
        sprintf(ret->sendStr, "5|%d|%s|%s", sender, timeStamp, msgText);
    }
    else
    {
        memset(sql, 0, sizeof(sql));
        sprintf(sql,
        "insert into MessageList(receiver, sender, msg, timestamp) values(%d, %d, %s, %s); update UserInfo set msgWait = msgWait + 1 where userID = %d;",
        receiver, sender, msgText, timeStamp, receiver);
        if(sqlite3_exec(handler, sql, NULL, NULL, &errorMsg) != 0)
        {
            printf("%s",errorMsg);
            sqlite3_close(handler);
            return ;
        }
        ret = malloc(sizeof(struct sendRet));
        memset(ret->retStr, 0, sizeof(ret->retStr));
        ret->sendToOther = 0;
        strcpy(ret->retStr, "4|0");
    }
    
	sqlite3_close(handler);
	return ret;
 }

void logOut(const int confd)
{
    sqlite3 *handler;
 	char *errorMsg;
 	char sql[256];
 	if (sqlite3_open(dbPath, &handler) != 0)
 	{
 		printf("error");
        sqlite3_close(handler);
 		return ;
	}
    memset(sql, 0, sizeof(sql));
    sprintf(sql, "update UserInfo set confd = NULL where confd = %d;", confd);
    if (sqlite3_exec(handler, sql, NULL, NULL, &errorMsg) != 0)
    {
        printf("%s", errorMsg);
        sqlite3_close(handler);
        return ;
    }
    sqlite3_close(handler);
}