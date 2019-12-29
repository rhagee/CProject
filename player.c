
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
#include <math.h>

#include "lib/sharedmem.h"

#define intconvert(X) atoi(argv[X]);
#define FIRSTPLAYER 0

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

void SpawnPawns(int opt_id,int map_id,int smap_id,int posMsg_id,int instrMsg_id,int sync_id,int leftPawnMoves_id,int newPawnPos_id,int n_pawns,char letter[],int* pawns,int w,int h);
int rndPos(int w,int h,int smap_id);
void GameStart(int n_pawns,int n_players,int pawn_sync,int posMsg_id,int mynumber,int w,int h,int smap_id,cell* table,int letter,int* pawnPos);
void getFlagPos(int size,cell* table,int flagPos[]);
void Algorithm(int w,int h,int pos,int pawnPos,int flagPos[],int n_flags,Move assoc[]);
void insertPath(PathObj* Path,int n_flags,int flagPos[],int pos,int flagIndex,int flagDistance,int w);
void PathSender(int instrMsg_id,instrMsg* message,long type,int pawn,PathObj pawnFlag[],int length,int pawnPos[],int flagPos[],int w);
void PathCalculator(int flagPos,int pawnPos,instrMsg* message,int w);
void SendDirective(int,int,int pawnPos[],int,cell*,int,int flagPos[],int n_flags,int letter);
void PawnInit(int w,int h,int smap_id,int posMsg_id,cell* table,int letter,long type,int* pawnPos);
void Print_Table(int w,int h,cell* table);
void Print_TStatus(int w,int h,cell* table);
void getPos(int* x,int* y,int pos,int w);
void InitPawnMoves(int pawnMoves[],options* settings);
void readLeftMoves(int leftPawnMoves_id,int pawnMoves[],int n_pawns);
void readNewPos(int newPawnPos_id,int pawnPos[],int n_pawns);
void ClosingRoutine(int,int);

int main(int argc,char* argv[])
{
	int n_pawns,w,h;
	int* pawns;
	int* pawnPos;
	int* pawnMoves;
	int* flagPos;
	int i=0,j=0;
	int opt_id,map_id,smap_id,sync_id,pawn_sync,mynumber,n_players,posMsg_id,instrMsg_id,numMsg_id,leftPawnMoves_id,newPawnPos_id;
	long type;
	char letter;
	char* letter_s;
	options* settings;
	cell* table;
	numMsg n_flags;
	
	/* Take Passed Arguments */
	opt_id = intconvert(1);
	map_id = intconvert(2);
	smap_id = intconvert(3)
	sync_id = intconvert(4);
	letter = intconvert(5);
	letter_s=argv[5];
	mynumber=intconvert(6);
	pawn_sync=intconvert(7);
	numMsg_id=intconvert(8);
	type=mynumber+1;

	posMsg_id=instanceMsg();
	instrMsg_id=instanceMsg();
	leftPawnMoves_id=instanceMsg();
	newPawnPos_id=instanceMsg();
	/*Taking Shared Memory*/
	settings = getOptions(opt_id);
	table = getMap(map_id);
	w=settings->SO_BASE;
	h=settings->SO_ALTEZZA;
	n_pawns=settings->SO_NUM_P;
	n_players=settings->SO_NUM_G;
	pawnPos=malloc(sizeof(int)*n_pawns);
	pawnMoves=malloc(sizeof(int)*n_pawns);
	
	printf("Waiting ALLPLAYERSYNC\n");
	/*Wait all players Spawned*/
	semWaitZero(sync_id,ALLPLAYERSYNC,0);

	/* Create Pawns and Set them up */
	printf("Syncronizing and Setting Up Pawns \n");

	SpawnPawns(opt_id,map_id,smap_id,posMsg_id,instrMsg_id,sync_id,leftPawnMoves_id,newPawnPos_id,n_pawns,letter_s,pawns,w,h);
	GameStart(n_pawns,n_players,pawn_sync,posMsg_id,mynumber,w,h,smap_id,table,letter,pawnPos);

	printf("End Placing Pawns %c - PID : %d \n",letter,getpid());

	semReserve(sync_id,MASTERSYNC,0);

	InitPawnMoves(pawnMoves,settings);

	read_numMsg(numMsg_id,&n_flags,type);
	

	flagPos=malloc(sizeof(int)*n_flags.q);
	SendDirective(w,h,pawnPos,instrMsg_id,table,n_pawns,flagPos,n_flags.q,letter);
	semReserve(sync_id,ROUNDSYNC,0);
	semWaitZero(sync_id,ROUNDSTART,0);
	semReserve(sync_id,ALLSTARTED,0);

	readLeftMoves(leftPawnMoves_id,pawnMoves,n_pawns);
	readNewPos(newPawnPos_id,pawnPos,n_pawns);

	while(wait(NULL)!=-1);
	free(pawnPos);
	free(pawns);
	ClosingRoutine(instrMsg_id,posMsg_id);
	/*Print_Table(w,h,table);*/
	exit(0);
}

void SpawnPawns(int opt_id,int map_id,int smap_id,int posMsg_id,int instrMsg_id,int sync_id,int leftPawnMoves_id,int newPawnPos_id,int n_pawns,char letter[],int* pawns,int w,int h)
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
	char* argv[12];
	int i=0;
	pawns=malloc(sizeof(int)*(n_pawns));

	/*Convert from int to string*/
	sprintf(opt_id_s, "%d", opt_id);
	sprintf(map_id_s, "%d", map_id);
	sprintf(smap_id_s,"%d", smap_id);
	sprintf(posMsg_id_s,"%d",posMsg_id);
	sprintf(instrMsg_id_s,"%d",instrMsg_id);
	sprintf(sync_id_s,"%d",sync_id);
	sprintf(leftPawnMoves_id_s,"%d",leftPawnMoves_id);
	sprintf(newPawnPos_id_s,"%d",newPawnPos_id);

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
	argv[11] = NULL;
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
			break;
		}
	}

}

int rndPos(int w,int h,int smap_id)
{
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
		return rndPos(w,h,smap_id);
	}
}


void GameStart(int n_pawns,int n_players,int pawn_sync,int posMsg_id,int mynumber,int w,int h,int smap_id,cell* table,int letter,int *pawnPos)
{
	int i=0;
	srand(time(NULL));
	for(i=0;i<n_pawns;i++)
	{
		semReserve(pawn_sync,mynumber,0);
		/*printf("Init for %d",i);*/
		PawnInit(w,h,smap_id,posMsg_id,table,letter,i,pawnPos);
		if(mynumber==(n_players-1))
			semRelease(pawn_sync,0,0);
		else
			semRelease(pawn_sync,(mynumber+1),0);
	}
}

void SendDirective(int w,int h,int pawnPos[],int instrMsg_id,cell* table,int n_pawns,int flagPos[],int n_flags,int letter)
{
	int i=0,j=0,cont=0;
	Move* assoc;
	PathObj* PawnFlag;
	instrMsg message;
	assoc=malloc(sizeof(Move)*n_flags);
	PawnFlag=malloc(sizeof(PathObj)*n_flags);

	for(i=0;i<n_flags;i++)
	{
		assoc[i].pawnPos=-1;
	}

	getFlagPos(w*h,table,flagPos);
	/*Algorithm aplicated n_flags times because of the Recalculation of Direct taking flag of a Pawn*/
	for(j=0;j<n_flags;j++)
		for(i=0;i<n_pawns;i++)
		{
			Algorithm(w,h,pawnPos[i],i,flagPos,n_flags,assoc);
		}

	printf("			---------------------------------letter %c------------------------------\n",letter);

	for(i=0;i<n_flags;i++)
	{
		printf("			%c	Flag at position : %d , Best distance is %d by Pawn %d\n",letter,flagPos[i],assoc[i].distance,assoc[i].pawnPos+1);
	}

	printf("			------------------------------------------------------------------------\n");
	/*ATTENZIONE LA PAWNPOS ASSEGNATA E' GIUSTA NEL VETTORE MA IL TYPE E' SEMPRE +1 rispetto la posizione (dato che type 0 è generico)*/
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
		if(cont!=0)
		{
			PathSender(instrMsg_id,&message,(i+1),i,PawnFlag,cont,pawnPos,flagPos,w);
		}
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
	/*free(assoc);*/
}

void getFlagPos(int size,cell* table,int flagPos[])
{
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

void Algorithm(int w,int h,int pos,int pawnPos,int flagPos[],int n_flags,Move assoc[])
{
	int i=0,j=0,z=0,found=0;
	int startX,startY,endX,endY,fX,fY;
	PathObj* Path;
	int currdist,pathdist=0;
	getPos(&startX,&startY,pos,w);
	Path = malloc(sizeof(PathObj)*n_flags);
	for(i=0;i<n_flags;i++)
	{
		Path[i].flagIndex=-1;
	}
	for(i=0;i<n_flags;i++)
	{
		found=0;
		getPos(&endX,&endY,flagPos[i],w);
		currdist=(abs(startX-endX)+abs(startY-endY));
		if(currdist<=assoc[i].distance || assoc[i].pawnPos==-1)
		{
			for(j=0;j<n_flags && found!=1;j++)
			{
				if(j!=0)
				{
					getPos(&fX,&fY,flagPos[Path[j-1].flagIndex],w);
					pathdist=(abs(endX-fX)+abs(endY-fY))+Path[j-1].flagDistance;
					if((pathdist<Path[j].flagDistance || Path[j].flagIndex==-1) && (pathdist<=assoc[i].distance || assoc[i].pawnPos==-1))
					{
						insertPath(Path,n_flags,flagPos,j,i,pathdist,w);
						found=1;
					}
				}
				else
				{
					if((currdist<Path[j].flagDistance || Path[j].flagIndex==-1) && (currdist<=assoc[i].distance || assoc[i].pawnPos==-1))
					{
						insertPath(Path,n_flags,flagPos,j,i,currdist,w);
						found=1;
					}
				}
			}
			
		}
	}
	found =0;
	for(i=0;i<n_flags && found!=1;i++)
	{
		if(Path[i].flagDistance <= assoc[Path[i].flagIndex].distance || assoc[Path[i].flagIndex].pawnPos==-1)
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

void insertPath(PathObj Path[],int n_flags,int flagPos[],int pos,int flagIndex,int flagDistance,int w)
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
			getPos(&startX,&startY,flagPos[flagIndex],w);
			getPos(&endX,&endY,flagPos[tempIndex],w);
			flagDistance=flagDistance+abs(startX-endX)+abs(startY-endY);
			flagIndex=tempIndex;
		}
	}
}
void PathSender(int instrMsg_id,instrMsg* message,long type,int pawn,PathObj PawnFlag[],int length,int pawnPos[],int flagPos[],int w)
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
			PathCalculator(flagPos[PawnFlag[i].flagIndex],pawnPos[pawn],message,w);
		else
			PathCalculator(flagPos[PawnFlag[i].flagIndex],flagPos[PawnFlag[i-1].flagIndex],message,w);
		message->type=type;
		message->left=left;
		send_instrMsg(instrMsg_id,message);
			printf("Pawn %d -> Flag %d , N %d, S %d, W %d , E %d\n",pawn+1,PawnFlag[i].flagIndex,message->dir[N],message->dir[S],message->dir[W],message->dir[E]);
	}

}
void PathCalculator(int flagPos,int pawnPos,instrMsg* message,int w)
{
	int fX,fY,pX,pY,dirX,dirY,distX,distY;
	getPos(&fX,&fY,flagPos,w);
	getPos(&pX,&pY,pawnPos,w);
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

void PawnInit(int w,int h,int smap_id,int posMsg_id,cell* table,int letter,long type,int* pawnPos)
{
	int p=0;
	posMsg message;
	/*printf("---->INITIALIZE MY PAWNS\n");*/
	p=rndPos(w,h,smap_id);

	pawnPos[type]=p;
	message.type=type+1;
	message.x=(p%w);
	message.y=(p/w);
	send_posMsg(posMsg_id,&message);

	table[p].val=letter;
	table[p].type='p';
}

void Print_Table(int w,int h,cell* table)
{
	int i=0,j=0;
	while(i<w*h)
	{
		if(j<w)
		{
			if(table[i].type!='p')
				printf("%d|",table[i].val);
			else
				printf("%c|",table[i].val);
		}
		else
		{
			if(table[i].type!='p')
				printf("\n%d|",table[i].val);
			else
				printf("\n%c|",table[i].val);
			j=0;
		}
		i++;
		j++;
	}
	printf("\n");
}

void Print_TStatus(int w,int h,cell* table)
{
	int i=0,j=0;
	while(i<w*h)
	{
		if(j<w)
			printf("%c",table[i].type);
		else
		{
			printf("\n%c",table[i].type);
			j=0;
		}
		i++;
		j++;
	}
	printf("\n");
}
void getPos(int* x,int* y,int pos,int w)
{
	*x=(pos%w);
	*y=(pos/w);
}

void InitPawnMoves(int pawnMoves[],options* settings)
{
	int i=0;
	for(i=0;i<settings->SO_NUM_P;i++)
	{
		pawnMoves[i]=settings->SO_N_MOVES;
	}
}

void readLeftMoves(int leftPawnMoves_id,int pawnMoves[],int n_pawns)
{
	int i=0;
	numMsg leftPawnMoves;
	for(i<0;i<n_pawns;i++)
	{
		read_numMsg(leftPawnMoves_id,&leftPawnMoves,0);
		pawnMoves[(leftPawnMoves.type-1)]=leftPawnMoves.q;
	}
}

void readNewPos(int newPawnPos_id,int pawnPos[],int n_pawns)
{
	int i=0;
	numMsg newPawnPos;
	for(i<0;i<n_pawns;i++)
	{
		read_numMsg(newPawnPos_id,&newPawnPos,0);
		pawnPos[(newPawnPos.type-1)]=newPawnPos.q;
	}
}

void ClosingRoutine(int instrMsg_id,int posMsg_id)
{
	msgctl(instrMsg_id, IPC_RMID, NULL);
	msgctl(posMsg_id, IPC_RMID, NULL);
}