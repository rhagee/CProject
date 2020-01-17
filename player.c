															/*Includes*/
#include "lib/sharedmem.h"
															/*Defines*/														
#define intconvert(X) atoi(argv[X]);
#define FIRSTPLAYER 0
															/*Struct Defines*/
struct BestMove
{
	int distance;
	int pawnPos;
};
typedef struct BestMove Move;

struct PathObject
{
	int flagIndex;
	int flagDistance;
};
typedef struct PathObject PathObj;

															/*Functions Definition*/

/*Pawns Creation and Calculation*/
void SpawnPawns(char letter[],int opt_id,int map_id,int smap_id,int sync_id);
int rndPos(int smap_id);

/*Game Starting*/
void GameStart(cell* table,int n_players,int letter,int pawn_sync,int mynumber,int smap_id);

/*Algorithm for BEST Paths, Path Calculation and Path Sending*/
void Algorithm(int pos,int pawnPos,int moves,int n_flags,Move assoc[]);
void insertPath(PathObj* Path,int n_flags,int pos,int flagIndex,int flagDistance);
void PathSender(instrMsg* message,long type,int pawn,PathObj pawnFlag[],int length);
void PathCalculator(int flagPos,int pawnPos,instrMsg* message);
void SendDirective(int,cell*,int,int n_flags,int letter);

/*Getters*/
void getFlagPos(int size,cell* table);
void getPos(int* x,int* y,int pos);

/**Initializations*/
void PawnInit(cell* table,int letter,long type,int smap_id);
void InitPawnMoves(options* settings);

/*Readers*/
void readLeftMoves();
void readNewPos();

/*Calculation of TotalMoves and Sends to Master*/
int calcTotMoves();

/*Waiting Scores*/
void waitScores(int n_flags,int scoreMsg_id,int sync_id);

/*Closing Functions*/
void termination_handler(int sig);
void ClosingRoutine();


/*Global Variables that I have to free or use after the SIGNAL*/
int instrMsg_id,posMsg_id,leftPawnMoves_id,newPawnPos_id,pawnScoredMsg_id,movesMsg_id,semPawn_id;
int n_pawns;
long type;

int* pawnPos;
int* flagPos;
int* pawnMoves;
int* pawns;

/*Useful as Global*/
int w,h;

int main(int argc,char* argv[])
{
	/*Variables Declarations*/
	char* letter_s;
	options* settings;
	cell* table;
	numMsg n_flags;
	numMsg totalMoves;
	char letter;
	int opt_id,map_id,smap_id,sync_id,pawn_sync,mynumber,n_players,numMsg_id,scoreMsg_id;

	/*Signal Handlers*/
	signal(SIGTERM,termination_handler);
	signal(SIGINT,termination_handler);
	signal(SIGTSTP,termination_handler);
	signal(SIGQUIT,termination_handler);

	/* Take Passed Arguments */
	opt_id = intconvert(1);
	map_id = intconvert(2);
	smap_id = intconvert(3)
	sync_id = intconvert(4);
	letter = intconvert(5);
	letter_s = argv[5];
	mynumber=intconvert(6);
	type=mynumber+1;
	pawn_sync=intconvert(7);
	numMsg_id=intconvert(8);
	scoreMsg_id=intconvert(9);
	movesMsg_id = intconvert(10);
	posMsg_id=instanceMsg();
	instrMsg_id=instanceMsg();
	leftPawnMoves_id=instanceMsg();
	newPawnPos_id=instanceMsg();
	pawnScoredMsg_id=instanceMsg();
	semPawn_id=instanceSem(1);

	/*Taking Shared Memory*/
	settings = getOptions(opt_id);
	table = getMap(map_id);
	w=settings->SO_BASE;
	h=settings->SO_ALTEZZA;
	n_pawns=settings->SO_NUM_P;
	n_players=settings->SO_NUM_G;

	pawnPos=malloc(sizeof(int)*n_pawns);
	pawnMoves=malloc(sizeof(int)*n_pawns);
	pawns=malloc(sizeof(int)*(n_pawns));
	
	/*printf("Waiting ALLPLAYERSYNC\n");*/
	/*Wait all players Spawned*/
	semWaitZero(sync_id,ALLPLAYERSYNC,0);

	/* Create Pawns and Set them up */
	printf("Syncronizing and Setting Up Pawns \n");

	SpawnPawns(letter_s,opt_id,map_id,smap_id,sync_id);
	
	GameStart(table,n_players,letter,pawn_sync,mynumber,smap_id);

	printf("End Placing Pawns %c - PID : %d \n",letter,getpid());

	semReserve(sync_id,MASTERSYNC,0);

	InitPawnMoves(settings);

	/*ROUND ROUTINE*/
	while(1)
	{
		/*Wait master giving the Number of placed flags*/
		read_numMsg(numMsg_id,&n_flags,type);
		flagPos=malloc(sizeof(int)*n_flags.q);

		/*Start Calculating and Sending Directive to Pawns*/
		SendDirective(instrMsg_id,table,n_pawns,n_flags.q,letter);

		/*Telling master I'v finished sending*/
		semReserve(sync_id,ROUNDSYNC,0);

		/*Waiting master Starting Round and notify master about that*/
		semWaitZero(sync_id,ROUNDSTART,0);
		semReserve(sync_id,ALLSTARTED,0);

		/*Player start waiting HITS*/
		waitScores(n_flags.q,scoreMsg_id,sync_id);

		/*Reading left moves and new position (pawns)*/
		readLeftMoves();
		readNewPos();

		/*Deallocating FlagPos and reallocating (probability of new size)*/
		free(flagPos);

		/*Send Remaining Moves*/
		totalMoves.type=type;
		totalMoves.q = calcTotMoves();
		send_numMsg(movesMsg_id,&totalMoves);

		/*Notify master that I'v done my END ROUTINE*/
		semReserve(sync_id,ALLREADEND,0);
	}

	/*Code should never go there but just in-case of bugs*/
	while(wait(NULL)!=-1);
	ClosingRoutine();

	exit(0);
}

void SpawnPawns(char letter[],int opt_id,int map_id,int smap_id,int sync_id)
{
	char opt_id_s[10];
	char map_id_s[10];
	char smap_id_s[10];
	char posMsg_id_s[10];
	char instrMsg_id_s[10];
	char sync_id_s[10];
	char pos_s[10];
	char leftPawnMoves_id_s[10];
	char newPawnPos_id_s[10];
	char pawnScoredMsg_id_s[10];
	char semPawn_id_s[10];
	char* argv[14];
	int i=0;

	/*Convert from int to string*/
	sprintf(opt_id_s, "%d", opt_id);
	sprintf(map_id_s, "%d", map_id);
	sprintf(smap_id_s,"%d", smap_id);
	sprintf(posMsg_id_s,"%d",posMsg_id);
	sprintf(instrMsg_id_s,"%d",instrMsg_id);
	sprintf(sync_id_s,"%d",sync_id);
	sprintf(leftPawnMoves_id_s,"%d",leftPawnMoves_id);
	sprintf(newPawnPos_id_s,"%d",newPawnPos_id);
	sprintf(pawnScoredMsg_id_s,"%d",pawnScoredMsg_id);
	sprintf(semPawn_id_s,"%d",semPawn_id);
	semSet(semPawn_id,0,n_pawns);

	/*Prepare Argv*/
	argv[0] = "./pawn";
	argv[1] = opt_id_s;
	argv[2] = map_id_s;
	argv[3] = smap_id_s;
	argv[4] = letter;
	argv[5] = posMsg_id_s;
	argv[7] = instrMsg_id_s;
	argv[8] = sync_id_s;
	argv[9] = leftPawnMoves_id_s;
	argv[10] = newPawnPos_id_s;
	argv[11] = pawnScoredMsg_id_s;
	argv[12] = semPawn_id_s;
	argv[13] = NULL;
	for(i=0;i<n_pawns;i++)
	{
		sprintf(pos_s,"%d",(i+1));
		argv[6]=pos_s;
		switch(pawns[i]=fork())
		{
			case 0:
				execve("./pawn",argv,NULL);
			break;
			case -1:
				printf("ERRORE\n");
			break;
			default:

			/*printf("%d\n",pawns[i]);*/

			break;
		}
	}

}

int rndPos(int smap_id)
{
	/*calculating number position for the current pawn, if the position is locked , recursive call me and try new one*/
	int range = w*h;
	int p=0;
	p=(rand()%(range));

	if(semGet(smap_id,p)==1)
	{
		semReserve(smap_id,p,0);
		return p;
	}
	else
	{
		return rndPos(smap_id);
	}
}


void GameStart(cell* table,int n_players,int letter,int pawn_sync,int mynumber,int smap_id)
{
	int i=0;
	srand(time(NULL));
	for(i=0;i<n_pawns;i++)
	{
		/*Wait my turn*/
		semReserve(pawn_sync,mynumber,0);
		/*printf("Init for %d",i);*/

		/*Initialize Pawn Call for all pawns*/
		PawnInit(table,letter,i,smap_id);

		/*Unlock my next*/
		if(mynumber==(n_players-1))
			semRelease(pawn_sync,0,0);
		else
			semRelease(pawn_sync,(mynumber+1),0);
	}
}

void SendDirective(int instrMsg_id,cell* table,int n_pawns,int n_flags,int letter)
{

	int i=0,j=0,cont=0;
	Move* assoc;
	PathObj* PawnFlag;
	instrMsg message;
	assoc=malloc(sizeof(Move)*n_flags);
	PawnFlag=malloc(sizeof(PathObj)*n_flags);

	/*Initializing the ASSOC array to a default pawnPos value*/
	for(i=0;i<n_flags;i++)
	{
		assoc[i].pawnPos=-1;
	}

	/*Get all flags position and memorize it in flagPos*/
	getFlagPos(w*h,table);

	/*Algorithm aplicated n_flags times because of the Recalculation of Direct taking flag of a Pawn*/
	for(j=0;j<n_flags;j++)
		for(i=0;i<n_pawns;i++)
		{
			/*If my pawn has more moves than 0*/
			if(pawnMoves[i]>0)
				Algorithm(pawnPos[i],i,pawnMoves[i],n_flags,assoc);
		}

	printf("			--------------------------------- letter %c ------------------------------\n",letter);

	/*Printing cycle (commentable)*/
	for(i=0;i<n_flags;i++)
	{
		if(assoc[i].pawnPos+1!=0)
		{
			printf("			|%c	Flag at position : %d , Best distance is %d by Pawn %d\n",letter,flagPos[i],assoc[i].distance,assoc[i].pawnPos+1);
		}
		else
		{
			printf("			|%c	Flag at position : %d , Best distance is UNDEFINED\n",letter,flagPos[i]);
		}
	}

	printf("			-------------------------------------------------------------------------\n");

	/*Cycle for Path Sender*/
	for(i=0;i<n_pawns;i++)
	{
		cont=0;
		for(j=0;j<n_flags;j++)
		{
			if(assoc[j].pawnPos==i)
			{
				PawnFlag[cont].flagIndex=j;
				PawnFlag[cont].flagDistance=assoc[j].distance;
				cont++;
			}
		}
		/*If current pawn has atleast 1 path*/
		if(cont!=0)
		{
			PathSender(&message,(i+1),i,PawnFlag,cont);
		}
		/*Else the pawn get the DEFAULT "NO MOVE" values*/
		else
		{
			message.type=i+1;
			message.left=-1;
			message.dir[N]=0;
			message.dir[S]=0;
			message.dir[W]=0;
			message.dir[E]=0;
			send_instrMsg(instrMsg_id,&message);
		}
		
	}
	/*Cleaning assoc space avoiding useless memory used*/
	free(assoc);
}

void getFlagPos(int size,cell* table)
{
	/*Get flag position looking through the chessboard*/
	int i=0;
	int cont = 0;
	for(i=0;i<size;i++)
	{
		if(table[i].type=='f')
		{
			flagPos[cont]=i;
			cont++;
		}
	}
}


/*MOVE ALGORITHM EXPLANATION : 
								I'm calculating the BEST path for every pawns.
								I have the ASSOC array that as index indicate the Flag index, and contains : PawnIndex and Distance from current Flag (assoc index)
								If my pawn is closer than others DIRECTLY taking the Flag he check if is the closer to him, or add it to his path calculating distance
								from last flag "taken" to next will take. After the full calculation I change the ASSOC checking all my PATH array, and checking if my
								DISTANCE in Path for the Flag X is less than others in assoc[X].distance.
								It iterates and check "nFlags" times (called by the "father function") this avoid a bug caused by the Algorithm application to pawn Ordered
*/
void Algorithm(int pos,int pawnPos,int moves,int n_flags,Move assoc[])
{
	int i=0,j=0,z=0,found=0;
	int startX,startY,endX,endY,fX,fY;
	PathObj* Path;
	int currdist,pathdist=0;

	/*printf("moves left %d for pawn %d\n",moves,pawnPos+1);*/

	getPos(&startX,&startY,pos);
	Path = malloc(sizeof(PathObj)*n_flags);
	for(i=0;i<n_flags;i++)
	{
		Path[i].flagIndex=-1;
	}
	for(i=0;i<n_flags;i++)
	{
		found=0;
		getPos(&endX,&endY,flagPos[i]);
		currdist=(abs(startX-endX)+abs(startY-endY));
		if((currdist<assoc[i].distance || assoc[i].pawnPos==-1 || assoc[i].pawnPos==pawnPos) && currdist<=moves)
		{
			for(j=0;j<n_flags && found!=1;j++)
			{
				if(j!=0)
				{
					getPos(&fX,&fY,flagPos[Path[j-1].flagIndex]);
					pathdist=(abs(endX-fX)+abs(endY-fY))+Path[j-1].flagDistance;
					if((pathdist<Path[j].flagDistance || Path[j].flagIndex==-1) && (pathdist<assoc[i].distance || assoc[i].pawnPos==-1 || assoc[i].pawnPos==pawnPos) && pathdist<=moves)
					{
						insertPath(Path,n_flags,j,i,pathdist);
						found=1;
					}
				}
				else
				{
					if((currdist<Path[j].flagDistance || Path[j].flagIndex==-1) && (currdist<assoc[i].distance || assoc[i].pawnPos==-1 || assoc[i].pawnPos==pawnPos) && currdist<=moves)
					{
						insertPath(Path,n_flags,j,i,currdist);
						found=1;
					}
				}
			}
			
		}
	}
	found =0;
	for(i=0;i<n_flags && found!=1;i++)
	{
		if((Path[i].flagDistance < assoc[Path[i].flagIndex].distance || assoc[Path[i].flagIndex].pawnPos==-1 || assoc[Path[i].flagIndex].pawnPos==pawnPos) && Path[i].flagDistance<=moves && Path[i].flagIndex!=-1)
		{
			assoc[Path[i].flagIndex].distance=Path[i].flagDistance;
			assoc[Path[i].flagIndex].pawnPos=pawnPos;
		}
		else
		{
			found=1;
		}
	}
	free(Path);
	/*printf("My pos is : x %d , y %d \n Best is : x %d,y %d \n pos : %d\n Distance : %d\n",myX,myY,bX,bY,best,dist);*/
}

/*Insert new Flag to PATH array (in algorithm) shifting the others in the array*/
void insertPath(PathObj Path[],int n_flags,int pos,int flagIndex,int flagDistance)
{
	int i=0,fine=0;
	int tempIndex,tempDistance;
	int startX,startY,endX,endY;
	for(i=pos;i<n_flags && fine!=1;i++)
	{
		if(Path[i].flagIndex==-1)
		{
			Path[i].flagIndex=flagIndex;
			Path[i].flagDistance=flagDistance;
			fine=1;
		}
		else
		{
			tempIndex=Path[i].flagIndex;
			Path[i].flagIndex=flagIndex;
			Path[i].flagDistance=flagDistance;
			getPos(&startX,&startY,flagPos[flagIndex]);
			getPos(&endX,&endY,flagPos[tempIndex]);
			flagDistance=flagDistance+abs(startX-endX)+abs(startY-endY);
			flagIndex=tempIndex;
		}
	}
}

/*Sending all good Paths*/
void PathSender(instrMsg* message,long type,int pawn,PathObj PawnFlag[],int length)
{
	int i=0,j=0,tempDist,tempIndex,left;

	/* ORDER PATHS */
	for(i=1;i<length;i++)
	{
		for (j=i;j>0 && PawnFlag[j-1].flagDistance>PawnFlag[j].flagDistance;j--)
		{
			tempDist=PawnFlag[j].flagDistance;
			tempIndex=PawnFlag[j].flagIndex;
			PawnFlag[j].flagDistance = PawnFlag[j-1].flagDistance;
			PawnFlag[j].flagIndex=PawnFlag[j-1].flagIndex;
			PawnFlag[j-1].flagDistance = tempDist;
			PawnFlag[j-1].flagIndex = tempIndex;
		}
	}

	/*Start Sending to Calculator and to the Pawns after that : 1 message per Flag*/
	for(i=0;i<length;i++)
	{
		left=(length-i)-1;
		if(i==0)
			PathCalculator(flagPos[PawnFlag[i].flagIndex],pawnPos[pawn],message);
		else
			PathCalculator(flagPos[PawnFlag[i].flagIndex],flagPos[PawnFlag[i-1].flagIndex],message);
		message->type=type;
		message->left=left;
		send_instrMsg(instrMsg_id,message);
			/*printf("Pawn %d -> Flag %d , N %d, S %d, W %d , E %d\n",pawn+1,PawnFlag[i].flagIndex,message->dir[N],message->dir[S],message->dir[W],message->dir[E]);*/
	}

}

/*Calculate the Path from a Pawn to his own FLAG, and set the DIRECTION array*/
void PathCalculator(int flagPos,int pawnPos,instrMsg* message)
{
	int fX,fY,pX,pY,dirX,dirY,distX,distY;
	getPos(&fX,&fY,flagPos);
	getPos(&pX,&pY,pawnPos);
	dirX = pX-fX;
	dirY = pY-fY;
	distX=abs(pX-fX);
	distY=abs(pY-fY);
	if(dirX>0)
	{
		message->dir[W]=distX;
		message->dir[E]=0;
	}
	else if(dirX<0)
	{
		message->dir[E]=distX;
		message->dir[W]=0;
	}
	else
	{
		message->dir[E]=0;
		message->dir[W]=0;
	}

	if(dirY>0)
	{
		 message->dir[N]=distY;
		 message->dir[S]=0;
	}
	else if(dirY<0)
	{
		message->dir[S]=distY;
		message->dir[N]=0;
	}
	else
	{
		message->dir[N]=0;
		message->dir[S]=0;
	}
}

/*Initialize the pawns position*/
void PawnInit(cell* table,int letter,long type,int smap_id)
{
	int p=0;
	posMsg message;
	/*printf("---->INITIALIZE MY PAWNS\n");*/
	p=rndPos(smap_id);

	pawnPos[type]=p;
	message.type=type+1;
	message.x=(p%w);
	message.y=(p/w);
	send_posMsg(posMsg_id,&message);

	table[p].val=letter;
	table[p].type='p';
}

/*Get position in array function*/
void getPos(int* x,int* y,int pos)
{
	*x=(pos%w);
	*y=(pos/w);
}

/*Initalize the PawnMoves Array*/
void InitPawnMoves(options* settings)
{
	int i=0;
	for(i=0;i<settings->SO_NUM_P;i++)
	{
		pawnMoves[i]=settings->SO_N_MOVES;
	}
}

/*Waiting all scores*/
void waitScores(int n_flags,int scoreMsg_id,int sync_id)
{
	int i=0;
	struct msqid_ds buf;
	numMsg scoreFromPawn;
	numMsg scoreToMaster;

	/*I wait for the ROUND-END semaphore set by master in IPC_NOWAIT*/
	while(semWaitZero(sync_id,ROUNDEND,IPC_NOWAIT)==-1)
	{
		/*If round is not ENDED I wait a message (maybe it will be a HIT)*/
		/*I will get the EMPTY message if the last Hit has been made (and 1 of my pawns has been unlocked on ROUND-END), 
		and unlock me to check again ROUND-END (Surely ended now) */
		read_numMsg(pawnScoredMsg_id,&scoreFromPawn,0);
		if(scoreFromPawn.q>0)
		{

			/*printf("PLAYER : %c read HIT of %d\n",letter,scoreFromPawn.q);*/

			scoreToMaster.q=scoreFromPawn.q;
			scoreToMaster.type=type;
			send_numMsg(scoreMsg_id,&scoreToMaster);
		}
	}
	/*Waiting all pawns sending the "default ZERO message"*/
	semWaitZero(semPawn_id,0,0);

	/*Reset Semaphore*/
	semSet(semPawn_id,0,n_pawns);

	/*Number of message in the Queue*/
	msgctl(pawnScoredMsg_id,IPC_STAT,&buf);

	/*Cleaning QUEUE Routine*/
	for(i=0;i<buf.msg_qnum;i++)
	{
		read_numMsg(pawnScoredMsg_id,&scoreFromPawn,0);
	}
}

/*Reading left moves from pawn*/
void readLeftMoves()
{
	int i=0;
	numMsg leftPawnMoves;
	for(i<0;i<n_pawns;i++)
	{
		read_numMsg(leftPawnMoves_id,&leftPawnMoves,0);
		pawnMoves[(leftPawnMoves.type-1)]=leftPawnMoves.q;
	}
}

/*Reading new moves from pawn*/
void readNewPos()
{
	int i=0;
	numMsg newPawnPos;
	for(i<0;i<n_pawns;i++)
	{
		read_numMsg(newPawnPos_id,&newPawnPos,0);
		pawnPos[(newPawnPos.type-1)]=newPawnPos.q;
	}
}

/*Calculate my total moves left*/
int calcTotMoves()
{
	int i=0;
	int tot=0;
	for(i=0;i<n_pawns;i++)
	{
		tot+=pawnMoves[i];
	}
	return tot;
}

/*Closing routines*/
void ClosingRoutine()
{
	free(pawnPos);
	free(flagPos);
	free(pawnMoves);
	free(pawns);
	semctl(semPawn_id,0,IPC_RMID);
	msgctl(instrMsg_id, IPC_RMID, NULL);
	msgctl(posMsg_id, IPC_RMID, NULL);
	msgctl(leftPawnMoves_id, IPC_RMID, NULL);
	msgctl(newPawnPos_id, IPC_RMID, NULL);
	msgctl(pawnScoredMsg_id,IPC_RMID,NULL);

}

void termination_handler(int sig)
{
	int i=0;
	numMsg msg;
	/*Killing all Pawns*/
	for(i=0;i<n_pawns;i++)
	{
		kill(pawns[i],SIGINT);
	}
	while(wait(NULL)!=-1);

	/*Send last message to player*/
	msg.type=type;
	msg.q=calcTotMoves();
	send_numMsg(movesMsg_id,&msg);

	/*Clean the IPCS*/
	ClosingRoutine();

	/*printf("\n \n PLAYER ENDED FOR ROUND TERM \n \n ");*/
	exit(0);
}

