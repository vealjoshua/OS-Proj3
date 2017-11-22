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
#include <errno.h>
#include "share.h"


#define  NANOSECOND    1000000000 

void menu (int argc, char **argv,int *x,int *z, char **filename);
void displayHelpMesg();
void validate(int *x,int temp,char y);
void test(int x, int z,char*file);
void savelog(char *filename, message userMsg, shared_clock *sharedTime);
void CTRLhandler(int sig);
void on_alarm(int signal);
void releaseMem();

pid_t pidArr[100];
int psNumber = 0;
int x = 5;
int userRemaning  = 0;
int z = 20;
int shareID;
int critID;
int dieID;

 key_t key = 1994;
 key_t dieMsgKey = 1991;
 key_t criticalMsgKey = 1989;

int main(int argc, char **argv)
{
  signal(SIGALRM, on_alarm);
  signal(SIGQUIT,CTRLhandler);
  signal(SIGINT, CTRLhandler);
  

  char *filename = "test.out";
  if (argc<2)
  {
     printf("Running Program without any Commands, default options will be apply.\n");
  }
  else
  {
    menu(argc,argv,&x,&z,&filename);
  }
  test(x,z,filename);
  
  
  shared_clock *sharedTime;
  if((shareID = shmget(key, sizeof(shared_clock), IPC_CREAT | 0666)) < 0)  // creating the shared memory
  {
    perror("shmget in parent");
    exit(1);
  }
   
  sharedTime = (shared_clock*)shmat(shareID, NULL, 0);
  if (sharedTime->nanoSec ==(int)-1)
  {
    perror("Shmat error in Main");
    exit(1);
  }
  sharedTime->nanoSec = 0;
  sharedTime->second = 0;

  if ((critID = msgget(criticalMsgKey, IPC_CREAT | 0666)) < 0)
  {
    perror("Error in msgeet Master ");
	exit(1);
  }
  if ((dieID = msgget(dieMsgKey, IPC_CREAT | 0666)) < 0)
  {
    perror("Error in msgeet Master ");
	exit(1);
  }
  
  message msg;
  msg.myPid = getpid();
  if (msgsnd(critID, &msg, sizeof(msg), 0) < 0)
  {
    perror("critID Parent : msgsnd");
	releaseMem();
	exit(1);
  }

    char keypass[32];
  sprintf(keypass, "%d",key);
  char dieMsgKeyPass[32];
  sprintf(dieMsgKeyPass, "%d" , dieMsgKey);
  char criticalMsgKeyPass[32];
  sprintf(criticalMsgKeyPass, "%d" , criticalMsgKey); 
  
  
  int i = 0;
  for (i = 0; i < x; i++)
  { 
    pid_t pid;
    pidArr[i] = pid;
    pid = fork();

    if (pid < 0)
    {
      perror("Fork() Failed"); 
      exit(1); 
    }
    else if (pid == 0)
    {
      printf("Running User #%d Pid: %d\n\n", i,pidArr[i]);
      execl("./user",keypass,dieMsgKeyPass,criticalMsgKeyPass,NULL);
	  
      perror("Child failed to execl");
    }
  }
  
  int runningUsers = x;
  userRemaning = x;
  extern int errno;
  message usrMsg;
  int count = 0;
  struct msqid_ds buf;
  
    alarm(z);
  
  while (1)
   {
	  usrMsg.dieFlag = 0;   
	  sharedTime->nanoSec += 10;
      if(sharedTime->nanoSec == 1000000000)
      {
        sharedTime->nanoSec = 0;
		printf("sharedTime->nanoSec %d\n",sharedTime->nanoSec);
        sharedTime->second++;
        printf("sharedTime->second %d\n",sharedTime->second);		
      }

	 
	  
      if( msgrcv(dieID, &usrMsg, sizeof(usrMsg), 0,IPC_NOWAIT))
	  {
		if (usrMsg.dieFlag == 1)
		{	
	      savelog(filename, usrMsg, sharedTime);
	      printf("User ID %d | User time %d.%010d\n", usrMsg.myPid, usrMsg.death_Time.second, usrMsg.death_Time.nanoSec);
	      kill(usrMsg.myPid, SIGUSR1);
	      wait(NULL);
		  runningUsers -= 1;
		  
		  if (userRemaning < 100)
          {
            runningUsers +=1;
            userRemaning +=1;
 
            pid_t pid;
            pidArr[userRemaning]=pid;
            pid = fork();

            if (pid < 0)
            {
               perror("Fork() Failed."); 
               exit(1); 
            }
      
            if (pid == 0)
            {
               printf("Running User #%d pid: %d\n\n", userRemaning,pidArr[userRemaning]);
               execl("./user",keypass,dieMsgKeyPass,criticalMsgKeyPass,NULL);
               perror("Child failed to execl");
            }	   
		 }  
		 if(runningUsers  == 0)
         {
            break;
         }

	   }
	 }
	 if (sharedTime->second == 2)
       {
         printf("User has run out of oss time, the clock is %d.%010d\n", sharedTime->second,sharedTime->nanoSec);
		 kill(getpid(),SIGALRM);
       }
	}
  
  printf("Program Finished.\n");
  wait(NULL);
  releaseMem();
  
  return 0;
}

void menu (int argc, char **argv , int *x, int *z, char **filename)
{ 
  int c = 0;
  int temp = 0;
  static struct option long_options[] = 
  { 
    {"help", no_argument, 0, 'h'},
    { 0,     0          , 0,  0 } 
  };
  int long_index = 0;

  while((c = getopt_long_only(argc, argv, "hs:t:l:", long_options, &long_index)) != -1)
  {
    switch (c)
    {
      case 'h':
        displayHelpMesg();
      break;
	  
      case 's':
       temp = *x;
       *x = atoi(optarg);
        if (*x > 20)
	{
          printf("Inputed: %d is to big. (Limit 20). Reverting back to default 5.\n", *x);
          *x = temp;
        }
	validate(x,temp,'x');
      break;

      case 't':
	 temp = *z;
	 *z = atoi(optarg);
	 validate(z,temp,'z');
      break;

      case 'l':
        if (optopt == 'n')
        {
          printf("Please enter a valid filename.");
          return;
        }
        *filename = optarg;
      break;
      
      case '?':
        if (optopt == 'l')
        {
          printf("Command -l requires filename. Ex: -lfilename.txt | -l filename.txt.\n");
	  exit(0);
        }
        else if (optopt == 's')
        {
          printf("Commands -s requires int value. Ex: -s213| -s 2132\n");
	  exit(0);
        }
	else if (optopt == 'i')
	{
	  printf("Command -y requires int value. Ex: -i213| -i 2132\n");
	  exit(0);
	}
	else if (optopt == 't')
	{
	  printf("Command -z requires int value. Ex: -t13| -t 2132\n");
	  exit(0);	
	}
        else
        {
          printf("You have used an invalid command, please use -h or -help for command options, closing program.\n"); 
	  exit(0);
        }
      return;
	  
      default :
        if (optopt == 'l')
        {
          printf ("Please enter filename after -l \n");
          exit(0);
        }
	else if (optopt == 'n')
        { 
          printf ("Please enter integer x after -n \n");
		  exit(1);
        }
        printf("Running Program without Commands.\n");
      break;
    }
  }
}
void validate(int *x,int temp,char y)
{
  char *print;
  char *print2;
  if (y == 'z')
  {
    print = "z";
    print2 = "-t";
  }
  else if (y == 'x')
  {
    print = "x";
    print2 = "-s";
  }


  if (*x == 0)
  {
    printf("Intput invalid for %s changing %s back or default.\n",print2,print);
    *x = temp;
  }
  else if (*x < 0)
  {
    printf("Intput invalid for %s changing %s back or default.\n",print2,print);
    *x = temp;
  }
}
void displayHelpMesg()
{
  printf ("---------------------------------------------------------------------------------------------------------\n");
  printf ("Please run oss or oss -arguemtns.\n");
  printf ("----------Arguments---------------------------------------------\n");
  printf (" -h or -help  : shows steps on how to use the program \n");
  printf (" -s x         : x is the maximum number of slave processes spawned (default 5) \n");
  printf (" -l filename  : change the log file name \n");
  printf (" -t z         : parameter z is the time in seconds when the master will terminate itself (default 20) \n");
  printf ("---------------------------------------------------------------------------------------------------------\n");
  printf ("\nClosing Program.............\n\n");
  exit(0);
}
void test (int x,int z, char *file)
{
  printf ("--------------------------------\n");
  printf ("Number of Slaves (x): %d\n", x);
  printf ("Time limit       (z): %d\n", z);
  printf ("Filename            : %s\n", file);
  printf ("--------------------------------\n\n");
  printf("Running Program.\n");
}
void savelog(char *filename, message userMsg, shared_clock *sharedTime)
{
  FILE  *log = fopen(filename, "a");
  if (log == NULL)
  {
    perror ("File did not open: ");
    releaseMem();
    exit(1);
  }
  fprintf(log, "#%d Master(oss): %d is terminating(user) at %d.%010d because it reached %d.%010d\n",
          psNumber,userMsg.myPid, sharedTime->second, sharedTime->nanoSec, userMsg.death_Time.second, userMsg.death_Time.nanoSec);

   psNumber +=1;
  fclose(log);
}
void CTRLhandler(int sig)
{
  signal(sig, SIG_IGN);
  printf("\nCtrl^C Called. Closing All Process.\n");
  fflush(stdout);
  releaseMem();
  int i =0;
  for (i=0; i<100;i++)
  {
    kill(pidArr[x], SIGQUIT);
  }

  exit(0);
}
void on_alarm(int signal)
{
  printf("Out of time killing all slave processes.\n", z);
  releaseMem();
  int i = 0;
  for (i=0; i<100;i++)
  {
    kill(pidArr[x], SIGTERM);
  }
    exit(0);
}
void releaseMem()
{
  if((shmctl(shareID, IPC_RMID, NULL)) == -1)
  {
    perror("Error in shmdt in Parent:");
  }
  if((msgctl(critID, IPC_RMID, NULL)) == -1)
  {
    printf("Error in shmclt");
  }
  if ((msgctl(dieID, IPC_RMID, NULL) == -1))
  {
    perror("Erorr in msgctl ");
  }
}
