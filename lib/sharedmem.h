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

#define MASTERSYNC 0
#define ALLPLAYERSYNC 1
#define ROUNDSYNC 2
#define ROUNDSTART 3
#define ALLSTARTED 4

#define N 0
#define E 1
#define S 2
#define W 3

#define _GNU_SOURCE
/*OPTIONS SHARED MEMORY*/
struct myOptions
{
	int SO_NUM_G,SO_NUM_P,SO_MAX_TIME,SO_BASE,SO_ALTEZZA,SO_FLAG_MIN,SO_FLAG_MAX,SO_ROUND_SCORE,SO_N_MOVES;
	long SO_MIN_HOLD_NSEC;
};
typedef struct myOptions options;

/*CELL OF MAP GRID*/
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

/*Width and Height*/
int instanceMap(int w,int h);
/*Create Shared Memory for Options*/
int instanceOptions();
/*SIZE of semaphore Array*/
int instanceSem(int size);
int instanceMsg();
/*MAP ID*/
void deleteMap(int map);
/*Option ID*/
void deleteOptions(int opt);
/*Get Map Pointer (request MapID)*/
cell* getMap(int map);
/*Get Options Pointer (request OptionsID)*/
options* getOptions(int opt);

/*SemID and Value to set all*/
int semSetAll(int sem,int value);
/*SemID,Pos of sem, Value to set*/
int semSet(int sem,int pos,int value);

int semGet(int sem,int pos);
/*SemID,Pos of sem,Value to change(+1 -1 or others), FLAGS (optional -> For NOFLAGS = 0)*/
int semReserve(int sem,int pos,int FLAG);
int semWaitZero(int sem,int pos,int FLAG);
int semRelease(int sem,int pos,int FLAG);

int send_posMsg(int id,posMsg* message);
int read_posMsg(int id,posMsg* message,int type);

int send_instrMsg(int id,instrMsg* message);
int read_instrMsg(int id,instrMsg* message,long type);

int send_numMsg(int id,numMsg* message);
int read_numMsg(int id,numMsg* message,long type);