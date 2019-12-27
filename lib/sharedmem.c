#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

struct myOptions
{
	int SO_NUM_G,SO_NUM_P,SO_MAX_TIME,SO_BASE,SO_ALTEZZA,SO_FLAG_MIN,SO_FLAG_MAX,SO_ROUND_SCORE,SO_N_MOVES,SO_MIN_HOLD_NSEC;
};
typedef struct myOptions options;


struct oneCell
{
	int val;
	char type;
};
typedef struct oneCell cell;

struct PositionMessage
{
	long type;
	int x;
	int y;
};
typedef struct PositionMessage posMsg;

struct InstructionMessage
{
	long type;
	int dir[4];
	int left;
};
typedef struct InstructionMessage instrMsg;

struct QuantityMessage
{
	long type;
	int q;
};
typedef struct QuantityMessage numMsg;




/* Only master Call (Others passed by parameter*/
int instanceMap(int w,int h)
{
	return shmget(IPC_PRIVATE,sizeof(cell)*w*h,IPC_CREAT|0666);
}

int instanceOptions()
{
	return  shmget(IPC_PRIVATE,sizeof(options),IPC_CREAT|0666);
}

int instanceSem(int size)
{
	return semget(IPC_PRIVATE,size,IPC_CREAT|0666);
}

int instanceMsg()
{
	return msgget(IPC_PRIVATE,IPC_CREAT|0666);
}


/* Generic passing ID*/

void deleteMap(int map)
{
	shmctl(map,IPC_RMID,0);
}

void deleteOptions(int opt)
{
	shmctl(opt,IPC_RMID,0);
}

cell* getMap(int map)
{
		return (cell*)shmat(map,NULL,0); 
}

options* getOptions(int opt)
{
		return (options*)shmat(opt,NULL,0);

}


/*Semaphore Operations*/
int semSetAll(int sem,int value)
{
	struct semid_ds semid_ds;
	int retrn,length,i=0;
	short* semvals;
	union semun
	{
		int val;
		struct semid_ds *buf;
		short *array;
	}arg;
	arg.buf=&semid_ds;
	retrn=semctl(sem,0,IPC_STAT,arg);
	length=arg.buf->sem_nsems;
	semvals=malloc(sizeof(short)*length);
	for(i=0;i<length;i++)
	{
		semvals[i]=value;
	}
	arg.array=semvals;
	return semctl(sem,0,SETALL,arg);
}

int semSet(int sem,int pos,int value)
{
	return semctl(sem,pos,SETVAL,value);
}

int semGet(int sem,int pos)
{
	return semctl(sem,pos,GETVAL);
}
int semReserve(int sem,int pos,int FLAG) 
{
	struct sembuf sops;
	sops.sem_num = pos;
	sops.sem_op = -1;
	sops.sem_flg = FLAG;
	return semop(sem, &sops, 1);
}

int semWaitZero(int sem,int pos,int FLAG) 
{
	struct sembuf sops;
	sops.sem_num = pos;
	sops.sem_op = 0;
	sops.sem_flg = FLAG;
	return semop(sem, &sops, 1);
}

int semRelease(int sem,int pos,int FLAG)
{
	struct sembuf sops;
	sops.sem_num = pos;
	sops.sem_op = 1;
	sops.sem_flg = FLAG;
	return semop(sem, &sops, 1);
}

int send_posMsg(int id,posMsg* message)
{
	return msgsnd(id,message,sizeof(int)*2,0);
}

int read_posMsg(int id,posMsg* message,long type)
{
	return msgrcv(id,message,sizeof(int)*2,type,0);
}

int send_instrMsg(int id,instrMsg* message)
{
	return msgsnd(id,message,sizeof(int)*5,0);
}

int read_instrMsg(int id,instrMsg* message,long type)
{
	return msgrcv(id,message,sizeof(int)*5,type,0);
}

int send_numMsg(int id,numMsg* message)
{
	return msgsnd(id,message,sizeof(int),0);
}

int read_numMsg(int id,numMsg* message,long type)
{
	return msgrcv(id,message,sizeof(int),type,0);
}