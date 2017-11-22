#ifndef SHARE_H
#define SHARE_H

typedef struct Clock
{
    int nanoSec;
    int second;
}shared_clock;

typedef struct Queue
{
    char     stype;
    int      dieFlag;

    pid_t    myPid;
    shared_clock   death_Time;

} message;

#define MSGSZ     500
typedef struct msgbuf
{
    long    mtype;
    char    mtext[MSGSZ];
} message_buf;








#endif
