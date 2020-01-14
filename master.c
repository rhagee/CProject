
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
			"\n%*s %ld %*ld"
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
			"\n%*s %*ld %ld"
#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));}
#define WARN_NPLAYERS printf("\n\n\n\nWARN!\nNUMERO USERNAME GIOCATORI SUPERATO : possibile errore nella visualizzazione\n");

																	/* Methods Declaration */
void Read_Settings(int*,int*,int*,int*,int*,int*,int*,int*,int*,long*);
void Init_Table(int,int,cell*);
void Init_Scores(int*,int);
void Print_Table(int,int,cell*);
void Print_Status(int,int,cell*,int);
void createPlayers(int,int,int*,int*,int,int,int,int,int);
void Round_Start(int,int,int,int,cell*,int,int,int,int,int*);
void SendNFlags(int,int,int);
int Flag_Calc(int,int);
void Flag_ValAndPlace(int,int,cell*,int,int);
void FlagPlace(cell*,int,int,int);
void SignScores(int scoreMsg_id,int* scores,int n_flags);

void readTotMoves();
/*END GAME AND STOP HANDLE/DELETERS*/
void ClosingRoutine();
void termination_handler(int sig);
void endgame_handler(int sig);


cell* table;
options* settings;
int w,h,nmoves;
int map_id,opt_id,smap_id,sync_id,pawn_sync,numMsg_id,scoreMsg_id,n_players,movesMsg_id,timeleft;
int* players;
int* scores;
int totrounds,n_pawns;
int *pMoves;
long timeplayed;


int main()
{
																	/*Variables Declaration*/
	/*Larghezza,Altezza,NumeroGiocatori*/
	int f_min,f_max,f_tot,score,f_num;
	/*IDGrigliaGioco,IDOpzioni,IDSemaforiGriglia,IDSemSync*/		
	/*int map_id,opt_id,smap_id,sync_id,pawn_sync,numMsg_id,scoreMsg_id;*/
	/*Vettore di PID giocatore*/
	/*int* players;
	int* scores;*/
	/*Puntatori alle Aree di memoria*/
	/*cell* table;
	options* settings;*/


	signal(SIGINT,termination_handler);
	signal(SIGALRM,endgame_handler);
	totrounds=0;
															/*Take Settings*/
	opt_id=instanceOptions();
	settings = getOptions(opt_id);
	Read_Settings(&(settings->SO_NUM_G),&(settings->SO_NUM_P),&(settings->SO_MAX_TIME),&(settings->SO_BASE),&(settings->SO_ALTEZZA),&(settings->SO_FLAG_MIN),&(settings->SO_FLAG_MAX),&(settings->SO_ROUND_SCORE),&(settings->SO_N_MOVES),&(settings->SO_MIN_HOLD_NSEC));														/*Creating Map*/
	
	timeplayed=0;														/*Local MEM Settings*/
	w=settings->SO_BASE;
	h=settings->SO_ALTEZZA;
	n_players=settings->SO_NUM_G;
	f_min=settings->SO_FLAG_MIN;
	f_max=settings->SO_FLAG_MAX;
	f_tot=settings->SO_ROUND_SCORE;
	n_pawns=settings->SO_NUM_P;
	score=settings->SO_ROUND_SCORE;
	timeleft=settings->SO_MAX_TIME;
	nmoves=settings->SO_N_MOVES;
																	/*Create Map*/
	players=malloc(sizeof(int)*(n_players));
	scores=malloc(sizeof(int)*(n_players));
	pMoves=malloc(sizeof(int)*(n_players));
	map_id=instanceMap(w,h);
	table = getMap(map_id);
	smap_id=instanceSem(w*h);
	sync_id=instanceSem(7);
	pawn_sync=instanceSem(n_players);
	numMsg_id=instanceMsg();
	scoreMsg_id=instanceMsg();
	movesMsg_id=instanceMsg();
																	/*Initialize Table*/
	Init_Table(w,h,table);
	Init_Scores(scores,n_players);

																	/*Create Players*/
	createPlayers(n_players,n_pawns,players,scores,smap_id,sync_id,pawn_sync,numMsg_id,scoreMsg_id);
	printf("WAITING PLAYERS SYNC...\n",sync_id);
	semWaitZero(sync_id,MASTERSYNC,0);
	/* Round Routine */
	while(1){
	printf("Starting Settings up Round...\n");
	Round_Start(f_min,f_max,f_tot,score,table,smap_id,numMsg_id,(w*h),n_players,&f_num);	
	semWaitZero(sync_id,ROUNDSYNC,0); /*Wait Players giving info to Pawns*/
	semSet(sync_id,ROUNDSYNC,n_players); /*Reset Semaphore for next round*/
	printf("UNLOCKING ROUND START\n");
	if(totrounds==0)
	{
		Print_Table(w,h,table);
		Print_Status(w,h,table,smap_id);
	}
	semReserve(sync_id,ROUNDSTART,0);
	semWaitZero(sync_id,ALLSTARTED,0);
	
	alarm(timeleft);

	/*Dovrò spostarlo in un HANDLER di fine round, questi semafori verranno SBLOCCATI prima di sbloccare l'END Round ai player
	Ovviamente ENDRound avrà un controllo su "tutti hanno letto", a quel punto verrà resettato insieme al TUTTI hanno letto
	prima di Sbloccare nuovamente RoundStart->AllRead e aver piazzato le bandierine*/
	
	semSet(sync_id,ALLSTARTED,((n_players*n_pawns)+n_players));
	semRelease(sync_id,ROUNDSTART,0);

	SignScores(scoreMsg_id,scores,f_num);
	printf("END SCORES , ALARM RESET \n");
	/*if all flags are catched -> RESET alarm*/
	timeplayed+=alarm(0);
	totrounds++;
	semReserve(sync_id,ROUNDEND,0);
	semWaitZero(sync_id,ALLREADEND,0);
	printf("ALL ENDS \n");
	readTotMoves();
	printf("\n\n\n\n\n ROUND : %d\n\n\n\n\n",totrounds);
	semSet(sync_id,ALLREADEND,((n_players*n_pawns)+n_players));
	semRelease(sync_id,ROUNDEND,0);
	printf("MASTER RESET SEMAPHORES \n");

	}
	

	
	/*Round should be running there, master waiting info about Scores.
	  Probably best way to do it is passing a message queue downside and give it 1 for player, and wait on 0, detect type
	  and give score to the player, then test if all flags are taken, if they are the round should stop and restart, 
	  else master should wait other info about scores. (always catching ALARM that should end round.)*/

														/*End Routine*/
	while(wait(NULL)!=-1);
	Print_Table(w,h,table);
	Print_Status(w,h,table,smap_id);
	ClosingRoutine(table,settings,map_id,opt_id,smap_id,sync_id,pawn_sync,players,numMsg_id,scoreMsg_id);
}



void createPlayers(int n_players,int n_pawns,int* players,int* scores,int smap_id,int sync_id,int pawn_sync,int numMsg_id,int scoreMsg_id)
{
	char opt_id_s[10];
	char map_id_s[10];
	char smap_id_s[10];
	char sync_id_s[10];
	char pawn_sync_s[10];
	char numMsg_id_s[10];
	char scoreMsg_id_s[10];
	char movesMsg_id_s[10];
	char mynumber[10];
	char letter[10];
	char* argv[12];
	int i=0;

	sprintf(opt_id_s, "%d", opt_id);
	sprintf(map_id_s, "%d", map_id);
	sprintf(smap_id_s,"%d", smap_id);
	sprintf(sync_id_s,"%d", sync_id);
	sprintf(pawn_sync_s,"%d", pawn_sync);
	sprintf(numMsg_id_s,"%d", numMsg_id);
	sprintf(scoreMsg_id_s,"%d",scoreMsg_id);
	sprintf(movesMsg_id_s,"%d",movesMsg_id);
	argv[0] = "./player";
	argv[1] = opt_id_s;
	argv[2] = map_id_s;
	argv[3] = smap_id_s;
	argv[4] = sync_id_s;
	argv[7] = pawn_sync_s;
	argv[8] = numMsg_id_s;
	argv[9] = scoreMsg_id_s;
	argv[10] = movesMsg_id_s;
	argv[11] = NULL;
	semSet(sync_id,MASTERSYNC,n_players);
	semSet(sync_id,ALLPLAYERSYNC,1);
	semSet(sync_id,ROUNDSYNC,n_players);
	semSet(sync_id,ROUNDSTART,1);
	semSet(sync_id,ALLSTARTED,((n_players*n_pawns)+n_players));
	semSet(sync_id,ROUNDEND,1);
	semSet(sync_id,ALLREADEND,((n_players*n_pawns)+n_players));
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
					scores[i] = 0;
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

void Read_Settings(int* SO_NUM_G,int* SO_NUM_P,int* SO_MAX_TIME,int* SO_BASE,int* SO_ALTEZZA,int* SO_FLAG_MIN,int* SO_FLAG_MAX,int* SO_ROUND_SCORE,int* SO_N_MOVES,long* SO_MIN_HOLD_NSEC)
{
	FILE * file;
	int gamemode;
	file = fopen("settings.txt","r");
	printf(MENU);
	scanf("%d",&gamemode);
	printf("						_____________________________________________________\n");
	printf("								     GAME START                    \n\n");
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
}

void Round_Start(int f_min,int f_max,int f_tot,int score,cell* table,int smap_id,int numMsg_id,int size,int n_players,int* pf_num)
{

	*pf_num = Flag_Calc(f_min,f_max);
	Flag_ValAndPlace(*pf_num,score,table,smap_id,size);
	SendNFlags(*pf_num,numMsg_id,n_players);

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

void Init_Scores(int* scores,int n_players)
{
	int i=0;
	for(i=0;i<n_players;i++)
	{
		scores[i]=0;
	}
}
void SignScores(int scoreMsg_id,int*scores,int n_flags)
{
	numMsg scoreFromPlayer;
	int i=0;
	printf("WAITING %d FLAGS\n",n_flags);
	for(i=0;i<n_flags;i++)
	{
		read_numMsg(scoreMsg_id,&scoreFromPlayer,0);
		scores[(scoreFromPlayer.type-1)]+=scoreFromPlayer.q;
		printf("MASTER SIGN SCORE FOR %d VLAUE : %d\n",scoreFromPlayer.type-1,scoreFromPlayer.q);
	}
}

void readTotMoves()
{
	numMsg msg;
	int i=0;
	for(i=0;i<n_players;i++)
	{
		read_numMsg(movesMsg_id,&msg,0);
		pMoves[((msg.type)-1)]=msg.q;
	}
}


void ClosingRoutine()
{
	free(players);
	free(scores);
	shmdt(table);
	shmdt(settings);
	deleteMap(map_id);
	deleteOptions(opt_id);
	semctl(smap_id,0,IPC_RMID);
	semctl(sync_id,0,IPC_RMID);
	semctl(pawn_sync,0,IPC_RMID);
	msgctl(numMsg_id,IPC_RMID, NULL);
	msgctl(scoreMsg_id,IPC_RMID,NULL);
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



void termination_handler(int sig)
{
	int i=0;
	for(i=0;i<n_players;i++)
	{
		kill(players[i],SIGINT);
	}
	while(wait(NULL)!=-1);
	ClosingRoutine();
	printf("TERMINATED for sig %d\n",sig);
	exit(0);
}

void endgame_handler(int sig)
{
	int i,temp,tp;
	float m_m,s_m;
	tp=0;
	printf("GAME OVER -> Alarm 0\n");
	for(i=0;i<n_players;i++)
	{
		kill(players[i],SIGINT);
	}
	while(wait(NULL)!=-1);
	for(i=0;i<n_players;i++)
	{
		tp+=scores[i];
	}
	timeplayed+=timeleft;
	Print_Table(w,h,table);
	printf("\n\n\n          GAME OVER\n");
	printf("------------------------------------\n");
	printf("|Total Rounds : %d\n|Total Points : %d \n|Time Played : %ds \n",totrounds,tp,timeplayed);
	printf("------------------------------------\n\n\n");
	readTotMoves();
	for(i=0;i<n_players;i++)
	{
		printf("                Player %d                 \n",i);
		printf("------------------------------------\n");
		printf("|Score : %d\n",scores[i]);
		printf("|Moves Left : %d\n",pMoves[i]);
		m_m = ((((nmoves*n_pawns)-pMoves[i])*100)*1.00f/(nmoves*n_pawns)*1.00f)*1.00f;
		printf("|Used Moves : %f%%\n",m_m);
		s_m = (scores[i]*1.00f/((nmoves*n_pawns)-pMoves[i])*1.00f);
		printf("|Move for a Point %f\n",s_m);
		printf("------------------------------------\n\n");
	}
	ClosingRoutine();
	exit(0);
}