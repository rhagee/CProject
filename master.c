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
																/*Includes*/
#include "lib/sharedmem.h"

																/* Defines */
#define MENU "Select GameMode : \n1.Easy \n2.Hard \nDefault: Easy \nSelezione: "
#define EASY_MODE \
			"%*s %d %*d"\
			"\n%*s %d %*d"\
			"\n%*s %d %*d"\
			"\n%*s %d %*d"\
			"\n%*s %d %*d"\
			"\n%*s %d %*d"\
			"\n%*s %d %*d"\
			"\n%*s %d %*d"\
			"\n%*s %d %*d"\
			"\n%*s %d %*d"
#define HARD_MODE \
			"%*s %*d %d"\
			"\n%*s %*d %d"\
			"\n%*s %*d %d"\
			"\n%*s %*d %d"\
			"\n%*s %*d %d"\
			"\n%*s %*d %d"\
			"\n%*s %*d %d"\
			"\n%*s %*d %d"\
			"\n%*s %*d %d"\
			"\n%*s %*d %d"
#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));}
#define WARN_NPLAYERS printf("\n\n\n\nWARN!\nNUMERO USERNAME GIOCATORI SUPERATO : possibile errore nella visualizzazione\n");

																	/* Methods Declaration */
void Read_Settings(int*,int*,int*,int*,int*,int*,int*,int*,int*,int*);
void Init_Table(int,int,cell*);
void Print_Table(int,int,cell*);
void Print_Status(int,int,cell*,int);
void createPlayers(int,int,int,int,int*,int,int,int,int);
void Round_Start(int,int,int,int,cell*,int,int,int,int);
void SendNFlags(int,int,int);
int Flag_Calc(int,int);
void Flag_ValAndPlace(int,int,cell*,int,int);
void FlagPlace(cell*,int,int,int);
void ClosingRoutine(cell* table,options* settings,int map_id,int opt_id,int smap_id,int sync_id,int pawn_sync,int* players,int numMsg_id);


int main()
{
																	/*Variables Declaration*/
	/*Larghezza,Altezza,NumeroGiocatori*/
	int w,h,n_players,f_min,f_max,f_tot,n_pawns,score,timeleft;
	/*IDGrigliaGioco,IDOpzioni,IDSemaforiGriglia,IDSemSync*/		
	int map_id,opt_id,smap_id,sync_id,pawn_sync,numMsg_id;
	/*Vettore di PID giocatore*/
	int* players;
	/*Puntatori alle Aree di memoria*/
	cell *table;
	options* settings;
	
																	/*Take Settings*/
	opt_id=instanceOptions();
	settings = getOptions(opt_id);
	Read_Settings(&(settings->SO_NUM_G),&(settings->SO_NUM_P),&(settings->SO_MAX_TIME),&(settings->SO_BASE),&(settings->SO_ALTEZZA),&(settings->SO_FLAG_MIN),&(settings->SO_FLAG_MAX),&(settings->SO_ROUND_SCORE),&(settings->SO_N_MOVES),&(settings->SO_MIN_HOLD_NSEC));														/*Creating Map*/
	
																	/*Local MEM Settings*/
	w=settings->SO_BASE;
	h=settings->SO_ALTEZZA;
	n_players=settings->SO_NUM_G;
	f_min=settings->SO_FLAG_MIN;
	f_max=settings->SO_FLAG_MAX;
	f_tot=settings->SO_ROUND_SCORE;
	n_pawns=settings->SO_NUM_P;
	score=settings->SO_ROUND_SCORE;
	timeleft=settings->SO_MAX_TIME;
																	/*Create Map*/
	players=malloc(sizeof(int)*(n_players));
	map_id=instanceMap(w,h);
	table = getMap(map_id);
	smap_id=instanceSem(w*h);
	sync_id=instanceSem(5);
	pawn_sync=instanceSem(n_players);
	numMsg_id=instanceMsg();
																	/*Initialize Table*/
	Init_Table(w,h,table);

																	/*Create Players*/
	createPlayers(opt_id,map_id,n_players,n_pawns,players,smap_id,sync_id,pawn_sync,numMsg_id);
	Print_Status(w,h,table,smap_id);
	printf("WAITING PLAYERS SYNC...\n",sync_id);
	semWaitZero(sync_id,MASTERSYNC,0);
	printf("Starting Settings up Round...\n");
	/* Round Routine */
	Round_Start(f_min,f_max,f_tot,score,table,smap_id,numMsg_id,(w*h),n_players);	

	semWaitZero(sync_id,ROUNDSYNC,0); /*Wait Players giving info to Pawns*/
	semSet(sync_id,ROUNDSYNC,n_players); /*Reset Semaphore for next round*/
	printf("UNLOCKING ROUND START\n");
	Print_Table(w,h,table);
	Print_Status(w,h,table,smap_id);
	semReserve(sync_id,ROUNDSTART,0);
	alarm(timeleft);
	semWaitZero(sync_id,ALLSTARTED,0);
	semRelease(sync_id,ROUNDSTART,0);
	semSet(sync_id,ALLSTARTED,((n_players*n_pawns)+n_players));

	printf("MASTER RESET SEMAPHORES \n");

	/*Round should be running there, master waiting info about Scores.
	  Probably best way to do it is passing a message queue downside and give it 1 for player, and wait on 0, detect type
	  and give score to the player, then test if all flags are taken, if they are the round should stop and restart, 
	  else master should wait other info about scores. (always catching ALARM that should end round.)*/

														/*End Routine*/
	while(wait(NULL)!=-1);
	Print_Table(w,h,table);
	Print_Status(w,h,table,smap_id);
	ClosingRoutine(table,settings,map_id,opt_id,smap_id,sync_id,pawn_sync,players,numMsg_id);
}



void createPlayers(int opt_id,int map_id,int n_players,int n_pawns,int* players,int smap_id,int sync_id,int pawn_sync,int numMsg_id)
{
	char opt_id_s[10];
	char map_id_s[10];
	char smap_id_s[10];
	char sync_id_s[10];
	char pawn_sync_s[10];
	char numMsg_id_s[10];
	char mynumber[10];
	char letter[10];
	char* argv[10];
	int i=0;

	sprintf(opt_id_s, "%d", opt_id);
	sprintf(map_id_s, "%d", map_id);
	sprintf(smap_id_s,"%d", smap_id);
	sprintf(sync_id_s,"%d", sync_id);
	sprintf(pawn_sync_s,"%d", pawn_sync);
	sprintf(numMsg_id_s,"%d", numMsg_id);
	argv[0] = "./player";
	argv[1] = opt_id_s;
	argv[2] = map_id_s;
	argv[3] = smap_id_s;
	argv[4] = sync_id_s;
	argv[7] = pawn_sync_s;
	argv[8] = numMsg_id_s;
	argv[9] = NULL;
	semSet(sync_id,MASTERSYNC,n_players);
	semSet(sync_id,ALLPLAYERSYNC,1);
	semSet(sync_id,ROUNDSYNC,n_players);
	semSet(sync_id,ROUNDSTART,1);
	semSet(sync_id,ALLSTARTED,((n_players*n_pawns)+n_players));
	semSetAll(pawn_sync,0);
	semSet(pawn_sync,0,1);
	semSetAll(smap_id,1);
	if(n_players>50)
	{
		WARN_NPLAYERS;
		sleep(3);
	}
	for(i=0;i<n_players;i++)
	{
		sprintf(mynumber,"%d",i);
		if(i<=25)
			sprintf(letter,"%d", i+97);
		else if(i>25 && i<=50)
			sprintf(letter,"%d", i-25+65);
		else
			sprintf(letter,"%d", i+97);
		argv[5]=letter;
		argv[6]=mynumber;
		switch(players[i]=fork())
		{
			case 0:
				execve("./player",argv,NULL);
			break;
			case -1:
				printf("ERRORE \n");
			break;
			default:
					printf("Player %c - PID : %d\n",atoi(letter),players[i]);
			break;
		}
	}
	semReserve(sync_id,ALLPLAYERSYNC,0);
}
/* 
SETTINGS TEMPLATE
SO_NUM_G 2 4
SO_NUM_P 10 400
SO_MAX_TIME 3 1
SO_BASE 60 120
SO_ALTEZZA 20 40
SO_FLAG_MIN 5 5
SO_FLAG_MAX	5 40
SO_ROUND_SCORE 10 200
SO_N_MOVES 20 200
SO_MIN_HOLD_NSEC 100000000 100000000
*/

void Read_Settings(int* SO_NUM_G,int* SO_NUM_P,int* SO_MAX_TIME,int* SO_BASE,int* SO_ALTEZZA,int* SO_FLAG_MIN,int* SO_FLAG_MAX,int* SO_ROUND_SCORE,int* SO_N_MOVES,int* SO_MIN_HOLD_NSEC)
{
	FILE * file;
	int gamemode;
	file = fopen("settings.txt","r");
	printf(MENU);
	scanf("%d",&gamemode);
	if(gamemode==2)
	{
		fscanf(file,HARD_MODE,SO_NUM_G,SO_NUM_P,SO_MAX_TIME,SO_BASE,SO_ALTEZZA,SO_FLAG_MIN,SO_FLAG_MAX,SO_ROUND_SCORE,SO_N_MOVES,SO_MIN_HOLD_NSEC);
	}
	else
	{
		fscanf(file,EASY_MODE,SO_NUM_G,SO_NUM_P,SO_MAX_TIME,SO_BASE,SO_ALTEZZA,SO_FLAG_MIN,SO_FLAG_MAX,SO_ROUND_SCORE,SO_N_MOVES,SO_MIN_HOLD_NSEC);
	}
}


/* 
SETTINGS TEMPLATE

Initialize to 0 and z (z means EMPTY)

*/
void Init_Table(int w,int h,cell* table)
{
	int i=0;

	while(i<w*h)
	{
		table[i].val = 0;
		table[i].type = 'z';
		i++;
	}
	Print_Table(w,h,table);
}

void Round_Start(int f_min,int f_max,int f_tot,int score,cell* table,int smap_id,int numMsg_id,int size,int n_players)
{

	int f_num = Flag_Calc(f_min,f_max);
	Flag_ValAndPlace(f_num,score,table,smap_id,size);
	SendNFlags(f_num,numMsg_id,n_players);

}
void SendNFlags(int f_num,int numMsg_id,int n_players)
{
	int i=0;
	numMsg message;
	message.q=f_num;
	for(i=1;i<=n_players;i++)
	{
		message.type=i;
		send_numMsg(numMsg_id,&message);
	}
}
int Flag_Calc(int f_min,int f_max)

{	int range = f_max-f_min;
	return (f_min)+(rand()%(range+1));

}
void Flag_ValAndPlace(int num,int score,cell* table,int smap_id,int size)
{
	int i=0;
	int value=0;
	for(i=num;i>0;i--)
	{
		if(i!=1)
		{
			value=(rand()%(score/i))+1;
			score=score-value;
		}
		else
		{
			value=score;
		}
		FlagPlace(table,smap_id,size,value);
	}
}

void FlagPlace(cell* table,int smap_id,int size,int value)
{
	int pos=0;
	pos=rand()%(size);
	if(semGet(smap_id,pos)!=0 && table[pos].type!='f')
	{
		table[pos].val = value;
		table[pos].type='f';
	}
	else
	{
		FlagPlace(table,smap_id,size,value);
	}
}



void ClosingRoutine(cell* table,options* settings,int map_id,int opt_id,int smap_id,int sync_id,int pawn_sync,int* players,int numMsg_id)
{
	free(players);
	shmdt(table);
	shmdt(settings);
	deleteMap(map_id);
	deleteOptions(opt_id);
	semctl(smap_id,0,IPC_RMID);
	semctl(sync_id,0,IPC_RMID);
	semctl(pawn_sync,0,IPC_RMID);
	msgctl(numMsg_id,IPC_RMID, NULL);
}

void Print_Table(int w,int h,cell* table)
{
	int i=0,j=0;
	printf("\n\nTABLE VIEW\n\n");
	while(i<w*h)
	{
		if(j<w)
		{
			if(table[i].type=='z')
			{
				printf("\033[1;37m");
				printf("%d ",table[i].val);
				printf("\033[0m");
				printf("|");
			}
			else if(table[i].type=='p')
			{
				printf("\033[1;34m");
				printf("%c ",table[i].val);
				printf("\033[0m");
				printf("|");
			}
			else
			{
				printf("\033[1;31m");
				if(table[i].val<10)
					printf("%d ",table[i].val);
				else
					printf("%d",table[i].val);
				printf("\033[0m");
				printf("|");
			}
		}
		else
		{
			if(table[i].type=='z')
			{
				printf("\033[1;37m");
				printf("\n%d ",table[i].val);
				printf("\033[0m");
				printf("|");
			}
			else if(table[i].type=='p')
			{
				printf("\033[1;34m");
				printf("\n%c ",table[i].val);
				printf("\033[0m");
				printf("|");
			}
			else
			{
				printf("\033[1;31m");
				if(table[i].val<10)
					printf("\n%d ",table[i].val);
				else
					printf("\n%d",table[i].val);
				printf("\033[0m");
				printf("|");
			}
			j=0;
		}
		i++;
		j++;
	}
	printf("\n");
}
/*errno == EAGAIN*/

void Print_Status(int w,int h,cell* table,int smap_id)
{
	int i=0,j=0;
	printf("\n\nTABLE STATUS\n\n");
	while(i<w*h)
	{
		if(j<w)
		{
			if(semGet(smap_id,i)==0)
			{
				printf("\033[1;31m");
				printf("%d ",semGet(smap_id,i));
				printf("\033[0m");
				printf("|");
			}
			else if(semGet(smap_id,i)==1)
			{
				printf("\033[1;34m");
				printf("%d ",semGet(smap_id,i));
				printf("\033[0m");
				printf("|");
			}
				
		}
		else
		{
				if(semGet(smap_id,i)==0)
			{
				printf("\033[1;31m");
				printf("\n%d ",semGet(smap_id,i));
				printf("\033[0m");
				printf("|");
			}
			else if(semGet(smap_id,i)==1)
			{
				printf("\033[1;34m");
				printf("\n%d ",semGet(smap_id,i));
				printf("\033[0m");
				printf("|");
			}
			j=0;
		}
		i++;
		j++;
	}
	printf("\n");
}