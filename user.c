#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>
#include <error.h>
#include <assert.h>
#include <sys/msg.h>
#include "share.h"


void CTRLhandler(int sig);
void TimeHandler(int sig);
void sigDie(int sig);
int random_number(int min_num, int max_num);
void releaseMem();

shared_clock *ossClock;
message_buf critMsg;
size_t buf_length; 

int shareID;
int critID;
int dieID;


int main(int argc, char *argv[])
{
  signal(SIGTERM, TimeHandler);
  signal(SIGQUIT,CTRLhandler);
  signal(SIGINT, CTRLhandler);
  signal(SIGUSR1, sigDie);
  
  key_t key = atoi(argv[0]);
  key_t dieMsgKey = atoi(argv[1]);
  key_t criticalMsgKey = atoi(argv[2]);
  
  if((shareID = shmget(key, sizeof(ossClock),0666)) < 0)
  {
    perror("shmget in parent");
    exit(1);
  }
  ossClock = (shared_clock*)shmat(shareID, (void *)0, 0);
 
  int startTime = ossClock->nanoSec;
  int life = 0;
  int checkLife = 0;
  if (startTime >= 80000000)
  {
	  life = random_number(1,1000000000);
	  checkLife = life+90000;
  }
  else
  {
	  life = random_number(startTime,10000000);
	  checkLife = life + startTime;
  }
  printf("User %d | Check Life %d\n", getpid(), checkLife);
  
  if ((critID = msgget(criticalMsgKey, 0666)) < 0)
  {
    perror("Error in msgeet Master ");
    exit(1);
  }
  
  if ((dieID = msgget(dieMsgKey, 0666)) < 0)
  {
    perror("Error in msgeet Master ");
    exit(1);
  }
  
  message msg;
  while(1)
  {
    msgrcv(critID, &critMsg, sizeof(critMsg), 0, 0);
       
    if(ossClock->nanoSec >= checkLife)
    {
	  msg.dieFlag = 1;
	  msg.myPid = getpid();
	  msg.death_Time.nanoSec = ossClock->nanoSec;
	  msg.death_Time.second = ossClock->second;
	  // printf("User %d | Dying message		\n", getpid());
	  msgsnd(dieID, &msg, sizeof(msg),0);
    }
    else
    {
    	 msgsnd(critID, &critMsg, sizeof(critMsg), 0);
    }
  } 
  return 0;
}

void CTRLhandler(int sig)
{ 
  signal(sig, SIG_IGN);
  fprintf(stderr, "\nCtrl^C Called, Process Exiting\n");
  releaseMem();
  msgsnd(critID, &critMsg, sizeof(critMsg), 0);
  kill(getpid(), SIGKILL);
}
void TimeHandler(int sig)
{
  releaseMem();
  fprintf(stderr, "\nOut of Time, Process %d Exiting\n", getpid());
  kill(getpid(), SIGKILL);
}
void sigDie(int sig)
{ 
  msgsnd(critID, &critMsg, sizeof(critMsg), 0);
  releaseMem();
  exit(1);
}
int random_number(int min_num, int max_num)
{
  int result =0,low_num=0,hi_num=0;
  if(min_num < max_num)
  {
    low_num=min_num;
    hi_num=max_num+1;
  }
  else
  {
    low_num=max_num+1;
    hi_num=min_num;
  }
  srand(time(NULL) - getpid()*2);
  result = (rand()%(hi_num-low_num))+low_num;
  return result;
}
void releaseMem()
{
   if((shmdt(ossClock)) == -1)
   {
   }
}
