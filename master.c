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

void Init_Table();
void Init_Scores();

void Print_Table();
void Print_Status();

void createPlayers();

/*Flags Routines and functions*/
void Round_Start(int,int,int,int,int,int*);
void SendNFlags(int);
int Flag_Calc(int,int);
void Flag_ValAndPlace(int,int,int);
void FlagPlace(int,int);

/*In Round and Post-Round Routines*/
void SignScores(int);
void readTotMoves();

/*END GAME AND STOP HANDLE/DELETERS*/
void ClosingRoutine();
void termination_handler(int sig);

void endgame_handler(int sig);

/*Global Variables (Useful in END-GAME prints and calculations)*/
int w,h,nmoves,n_players,totrounds,n_pawns;
float timeplayed;

/*Global Pointers*/
int* players;
int* scores;
int *pMoves;
cell* table;
options* settings;

/*ID's (Should stay GLOBAL because I use them in after Handlers works*/
int map_id,opt_id,smap_id,sync_id,pawn_sync,numMsg_id,scoreMsg_id,movesMsg_id,timeleft;


int main()
{
																	/*Variables Declaration*/
	int f_min,f_max,f_tot,score,f_num;
	timeplayed=0.0f;
																	/*Handlers Declaration*/
	/*
	STANDRAD METHOD:

	struct sigaction termination;
	termination.sa_handler=termination_handler;
	termination.sa_flags=0;
	sigaction(SIGTSTP,&termination,NULL);
	sigaction(SIGINT,&termination,NULL);

	Found out function "signal" that only requires SIGNAl,VoidHandler:
	*/
	signal(SIGTERM,termination_handler);
	signal(SIGINT,termination_handler);
	signal(SIGTSTP,termination_handler);
	signal(SIGQUIT,termination_handler);

	signal(SIGALRM,endgame_handler);
	
																		/*Take Settings*/
	opt_id=instanceOptions();
	settings = getOptions(opt_id);

	/*Inutile passare parametri di una variabile globale, ma rendo la funzione più leggibile*/
	Read_Settings(&(settings->SO_NUM_G),&(settings->SO_NUM_P),&(settings->SO_MAX_TIME),&(settings->SO_BASE),&(settings->SO_ALTEZZA),&(settings->SO_FLAG_MIN),&(settings->SO_FLAG_MAX),&(settings->SO_ROUND_SCORE),&(settings->SO_N_MOVES),&(settings->SO_MIN_HOLD_NSEC));														/*Creating Map*/
	
																		/*Local MEM Settings*/
	totrounds=0;
	timeplayed=0;
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
																		/*SYSTEM CALLS*/
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
	Init_Table();
	Init_Scores();

																	/*Create Players*/
	createPlayers();

	semWaitZero(sync_id,MASTERSYNC,0);
																	
																	/* Round Routine */
	while(1)
	{
	/*printf("Start Settings up Round...\n");*/
	printf("				    _____________________________________________________\n");
	printf("						        ROUND START                    \n\n");
	Round_Start(f_min,f_max,f_tot,score,(w*h),&f_num);	
	semWaitZero(sync_id,ROUNDSYNC,0); /*Wait Players giving info to Pawns*/
	semSet(sync_id,ROUNDSYNC,n_players); /*Reset Semaphore for next round*/

																/*Runtime Game Status Print*/
	Print_Table();
	Print_Status();
	printf("\n\n HITS : \n\n");
	
	/*Double Check if all are started*/
	semReserve(sync_id,ROUNDSTART,0);
	semWaitZero(sync_id,ALLSTARTED,0);
	
																/*Unlocking Semaphores for the NEXT ROUND */
	semSet(sync_id,ALLSTARTED,((n_players*n_pawns)+n_players));
	semRelease(sync_id,ROUNDSTART,0);

	alarm(timeleft);/*TimeDrop RoundStart*/

	/*Reading until all flags are taken*/
	SignScores(f_num);

	/*Update TimePlayed and Total Rounds Played*/
	timeplayed+=timeleft-alarm(0);
	totrounds++;
	/*Unlock RoundEnds with DoubleCheck for the reset*/
	semReserve(sync_id,ROUNDEND,0);
	semWaitZero(sync_id,ALLREADEND,0);
	
	/*Read Remaining Moves*/
	readTotMoves();

																	/*Standard Ruotine Print*/
	Print_Table();
	Print_Status();
	
	/*Reset Semaphores for the RoundEnd (Impossible De-Sync caused by the LOCK of the "Round Start")*/
	semSet(sync_id,ALLREADEND,((n_players*n_pawns)+n_players));
	semRelease(sync_id,ROUNDEND,0);

	/*printf("MASTER RESET SEMAPHORES \n");*/

	}
														/*End Routine*/

	/*Game Should NEVER come here.*/
	while(wait(NULL)!=-1);
	ClosingRoutine(table,settings,map_id,opt_id,smap_id,sync_id,pawn_sync,players,numMsg_id,scoreMsg_id);
	/*END MAIN*/
}



void createPlayers()
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

	/*Alphabet isn't that long*/
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
					/*printf("Player %c - PID : %d\n",atoi(letter),players[i]);*/
					scores[i] = 0;
			break;
		}
	}

	/*Unlock all players to let them position the Pawns*/
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
	printf("						    _____________________________________________________\n");
	printf("								         GAME START                    \n\n");
	if(gamemode==2)
	{
		fscanf(file,HARD_MODE,SO_NUM_G,SO_NUM_P,SO_MAX_TIME,SO_BASE,SO_ALTEZZA,SO_FLAG_MIN,SO_FLAG_MAX,SO_ROUND_SCORE,SO_N_MOVES,SO_MIN_HOLD_NSEC);
	}
	else
	{
		fscanf(file,EASY_MODE,SO_NUM_G,SO_NUM_P,SO_MAX_TIME,SO_BASE,SO_ALTEZZA,SO_FLAG_MIN,SO_FLAG_MAX,SO_ROUND_SCORE,SO_N_MOVES,SO_MIN_HOLD_NSEC);
	}
}

void Init_Table()
{
	int i=0;

	while(i<w*h)
	{
		table[i].val = 0;
		table[i].type = 'z';
		i++;
	}
}

void Init_Scores()
{
	int i=0;
	for(i=0;i<n_players;i++)
	{
		scores[i]=0;
	}
}

void Round_Start(int f_min,int f_max,int f_tot,int score,int size,int* pf_num)
{
	/*Flag Number*/
	*pf_num = Flag_Calc(f_min,f_max);
	/*Evaluates flags and Positioning them*/
	Flag_ValAndPlace(*pf_num,score,size);
	/*Send to all players a Message to know how more flag are available on the Chessboard*/
	printf("\n\n			------------------------- Players Instructions : ------------------------\n\n");
	SendNFlags(*pf_num);
}

void SendNFlags(int f_num)
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
{	
	int range = f_max-f_min;
	return (f_min)+(rand()%(range+1));
}

void Flag_ValAndPlace(int num,int score,int size)
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
		FlagPlace(size,value);
	}
}

void FlagPlace(int size,int value)
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
		FlagPlace(size,value);
	}
}




void SignScores(int n_flags)
{
	numMsg scoreFromPlayer;
	int i=0;
	/*printf("WAITING %d FLAGS\n",n_flags);*/
	/*Wait n_flags messages*/
	for(i=0;i<n_flags;i++)
	{
		read_numMsg(scoreMsg_id,&scoreFromPlayer,0);
		scores[(scoreFromPlayer.type-1)]+=scoreFromPlayer.q;
		/*printf("MASTER SIGN SCORE FOR %d VLAUE : %d\n",scoreFromPlayer.type-1,scoreFromPlayer.q);*/
	}
}

/*All the players Should communicate how more Total Moves left*/
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



void Print_Table()
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

void Print_Status()
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

/*Clearing IPCS and mallocs*/
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
	msgctl(movesMsg_id,IPC_RMID,NULL);
}


/*CTRL+C HANDLER*/
void termination_handler(int sig)
{
	int i=0;
	for(i=0;i<n_players;i++)
	{
		kill(players[i],SIGINT);
	}
	while(wait(NULL)!=-1);
	ClosingRoutine();
	printf("TERMINATED for signal SIGINT (%d)\n",sig);
	exit(0);
}

/*ALARM HANDLER*/
void endgame_handler(int sig)
{
	int i,temp,tp;
	char playerLetter;
	float m_m,s_m,t_s;
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
	t_s= ((tp*1.00f)/timeplayed*1.00f);
	Print_Table();
	printf("\n\n\n          GAME OVER\n");
	printf("------------------------------------\n");
	printf("|Total Rounds : %d\n|Total Points : %d \n|Time Played : %fs \n|Points per Time : %f\n",totrounds,tp,timeplayed,t_s);
	printf("------------------------------------\n\n\n");
	readTotMoves();
	for(i=0;i<n_players;i++)
	{

		if(i<=25)
			playerLetter=i+97;
		else
			playerLetter=i-25+65;

		printf("                Player %c                 \n",playerLetter);
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