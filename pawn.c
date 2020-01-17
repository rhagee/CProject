

#include "lib/sharedmem.h"


#define intconvert(X) atoi(argv[X])
#define longconvert(X) atol(argv[X])

#define STOPMOVE 2
#define ADDMOVE 1

#define true 1
#define false 0



void DelUselessMoves(int dir[]);
int Move(int smap_id,int* px,int* py,int w,int dir[],int* pindex,int index,int* pfailed,numMsg* pScore,cell* table,char letter,int nextCardinal);
int getPos(int,int,int);
int findOtherPath(int dir[],cell* table,int,int,int*,int*,int,char,int type,int moves);
void termination_handler(int sig);

/*Useful Global Variables (A volte sarebbero passate ad altri processi solo per poterle passare a processi ancora più "in basso")*/
int scoreToPlayerMsg_id;
int moves;
long type;
int semPawn_id;
struct timespec delay;


int main(int argc,char* argv[])
{
	/*Standard Variables Declaration*/
	char letter;
	int term;

	int posMsg_id,instrMsg_id,leftMoves_id,newPos_id,opt_id,map_id,smap_id,sync_id,index,x,y,w,h;
	int dir[4];
	int failed;
	
	/*Message/Struct Variables Declaration*/
	posMsg pos_message;
	instrMsg instr_message;
	numMsg leftMoves;
	numMsg newPos;
	numMsg scored;
	cell* table;
	options* settings;

	/*Handlers*/
	signal(SIGTERM,termination_handler);
	signal(SIGINT,termination_handler);
	signal(SIGTSTP,termination_handler);
	signal(SIGQUIT,termination_handler);
	
	/*Local get of ARGV vars*/
	opt_id=intconvert(1);
	map_id=intconvert(2);
	smap_id=intconvert(3);
    letter = intconvert(4);
    posMsg_id = intconvert(5);
	type = longconvert(6);
	instrMsg_id=intconvert(7);
	sync_id=intconvert(8);
	leftMoves_id=intconvert(9);
	newPos_id = intconvert(10);
	scoreToPlayerMsg_id = intconvert(11);
	semPawn_id=intconvert(12);
	settings=getOptions(opt_id);

	/*Variables Initialization*/
	failed=0;
	term=0;
	scored.type=type;

	delay.tv_sec=0;
	delay.tv_nsec=settings->SO_MIN_HOLD_NSEC;

	table=getMap(map_id);

	
	w=settings->SO_BASE;
	h=settings->SO_ALTEZZA;
	moves=settings->SO_N_MOVES;

	/*Waiting my Position on the Chessboard*/
	read_posMsg(posMsg_id,&pos_message,type);
	instr_message.left=1;

	/*Saving my position in Local variables*/
	x=pos_message.x;
	y=pos_message.y;

	/*  ROUND ROUTINE */
	while(1){ 
	/*NEW ROUND : Re-Initalize the positions*/
	dir[N]=0;
	dir[S]=0;
	dir[W]=0;
	dir[E]=0;

	/*WAIT round start and signal that i heard Round start*/
	semWaitZero(sync_id,ROUNDSTART,0);
	semReserve(sync_id,ALLSTARTED,0);
	instr_message.left=1;

	/*ISTRUCTIONS ROUTINE*/
	while(instr_message.left>0)
	{
		term=0;
		failed=0;
		/*Read Message*/
		read_instrMsg(instrMsg_id,&instr_message,type);

		/*Add current Instruction directions to the OLDER one*/
		dir[N]+=instr_message.dir[N];
		dir[S]+=instr_message.dir[S];
		dir[W]+=instr_message.dir[W];
		dir[E]+=instr_message.dir[E];

		/*Delete useless moves*/
		DelUselessMoves(dir);

		/*Current INSTRUCTION Execution*/
		index=0;
		while((dir[N]>0 || dir[S]>0 || dir[W]>0 || dir[E]>0) && term==0 && table[getPos((x+dir[E]-dir[W]),(y+dir[S]-dir[N]),w)].type=='f')
		{
			/*If I'v finished the moves on the current index, go next*/
			if(dir[index]<=0 && failed<4)
			{
				index = ((index+1)%4);
			}
			/*If I'v no more moves i'll stay*/
			else if(moves==0) 
			{
				term=STOPMOVE;
			}
			/*If I failed too much times*/
			else if(failed>=4)
			{
				/*If it was my last move , so someone has stolen my flag*/
				if((dir[N]+dir[S]+dir[W]+dir[E])==1)
				{
					term=STOPMOVE;
					failed=0;
				}
				/*If isn't my last move I have to calculate new path*/
				else
				{
					/*printf("FIND OTHER PATH\n");*/
					if((findOtherPath(dir,table,w,h,&x,&y,smap_id,letter,type,moves)) == false)
					{
						term=STOPMOVE;
					}
					/*If I'v found a new Path continue Normally*/
					else
					{
						failed=0;
					}
				}
			} /*If all is fine I'm MOVING*/
			else
			{
					/*When Moves goes right: MoveCounter - 1 , else i'll wait(delayTIME) without decresing it.*/
					if(Move(smap_id,&x,&y,w,dir,&index,index,&failed,&scored,table,letter,((index+1)%4))==true)
					{
						moves--;
					}
					nanosleep(&delay,NULL);
			}
		}

		/*
			DEBUG PRINTS AFTER TERMINATING A FULL INSTRUCTION

			if(instr_message.left>=0 && table[getPos((x+dir[E]-dir[W]),(y+dir[S]-dir[N]),w)].type!='f' && dir[N]+dir[S]+dir[W]+dir[E]>0)
			{
				printf("MISS for Pawn : %c-%d -> MY LAST INDEXES N:%d S:%d W:%d E: %d and Moves Left : %d\n",letter,type,dir[N],dir[S],dir[W],dir[E],moves);
			}
			else if(instr_message.left<0)
			{
				printf("I'm NOT moving\n");
			}
			else if(instr_message.left>=0 && dir[N]+dir[S]+dir[W]+dir[E]==0)
			{
				printf(" --- REAL HIT for Pawn : %c-%d -> MY LAST INDEXES N:%d S:%d W:%d E: %d and Moves Left : %d\n",letter,type,dir[N],dir[S],dir[W],dir[E],moves);
			}
			else if(term==2)
			{
				printf("FAILED MOVING \n");
			}
		*/

	}

	/*printf("-->Pawn %c-%d STOP MOVING WAITING ROUND END\n",letter,type-1);*/

	/*Waiting round End If I'v finished to move for this round*/
	semWaitZero(sync_id,ROUNDEND,0);

	/*printf("-->Pawn %c-%d UNLOCKED BY ROUND END\n",letter,type-1);*/

	/*Sending to player a EMPTY Score , to unlock him in case he is blocked on his loop*/
	scored.type=type;
	scored.q = 0;
	send_numMsg(scoreToPlayerMsg_id,&scored);

	/*Tell to player I'v sent my message (all pawns send one, so the player will EMPTY the message queue after this goes to 0)*/
	semReserve(semPawn_id,0,0);

	/*Sending left moves and new pos for calculation*/
	leftMoves.type=type;
	leftMoves.q = moves;	
	send_numMsg(leftMoves_id,&leftMoves);
	newPos.type=type;
	newPos.q=getPos(x,y,w);
	send_numMsg(newPos_id,&newPos);

	/*Tell to master I'v read ROUND-END (Made all routins of ending)*/
	semReserve(sync_id,ALLREADEND,0);
	}
	

	exit(0);
}

int Move(int smap_id,int* px,int* py,int w,int dir[],int* pindex,int index,int* pfailed,numMsg* pScore,cell* table,char letter,int nextCardinal)
{
	int statement;
	int x,y,actualpos,nextpos;
	int incX,incY;
	int res;
	x=*px;
	y=*py;
	/*Switch case to standard Routine (Non-Retoundand CODE)*/
	switch(index)
	{
		case N:
		{
			incY=-1;
			incX=0;
		}
		break;
		case S:
		{
			incY=1;
			incX=0;
		}
		break;
		case W:
		{
			incY=0;
			incX=-1;
		}
		break;
		case E:
		{
			incY=0;
			incX=1;
		}
		break;
		default:
		break;
	}

	actualpos=getPos(x,y,w);
	nextpos=getPos(x+incX,y+incY,w);

	/*I try to get my next pos, if I can I'll Take next writing and editing semaphores, else I'm failing and going to set FAILURES +1 */
	if(semReserve(smap_id,nextpos,IPC_NOWAIT)==-1)
	{
		/*printf("\n ERROR x:%d y:%d\n",x+1,y);*/

		if(pindex!=NULL)
			*pindex=nextCardinal;
		*pfailed=*pfailed+1;
		statement = false;
	}
	else
	{
		/*If the next is a FLAG I'll send score to player*/
		if(table[nextpos].type=='f' && pScore!=NULL)
		{
			pScore->q = table[nextpos].val;
			send_numMsg(scoreToPlayerMsg_id,pScore);
			printf("HIT for Pawn : %c-%d -> MY LAST INDEXES N:%d S:%d W:%d E: %d and Moves Left : %d\n",letter,type,dir[N],dir[S],dir[W],dir[E],(moves-1));
		}

		table[actualpos].val=0;
		table[actualpos].type='z';
		semRelease(smap_id,actualpos,0);
		*pfailed=0;
		table[nextpos].val=letter;
		table[nextpos].type='p';
		*px = x+incX;
		*py= y+incY;
		dir[index]--;
		statement=true;
	}
	
	return statement;
}

/*GETTIG vect Position*/
int getPos(int x,int y,int w)
{
	return ((y*w)+x);
}

/*Other PATH algorithm*/
int findOtherPath(int dir[],cell* table,int w,int h,int* px,int* py,int smap_id,char letter,int type,int moves)
{
	int i,j,x,y;
	int thismoves,next;
	int q;
	int ok,myfails;
	int nextdir[4];
	int foundplus,foundminus;
	int minusMoves,plusMoves;

	x=*px;
	y=*py;
	foundminus=false;
	foundplus=false;
	ok=false;
	q=0;

	/*I CHECK how much DIRECTION have I to finish*/
	if(dir[N]>0)
	{
		nextdir[q]=N;
		q++;
	}
    if(dir[S]>0)
    {
    	nextdir[q]=S;
		q++;
    }
    if(dir[E]>0)
    {
    	nextdir[q]=E;
		q++;
    }
	if(dir[W]>0)
	{
		nextdir[q]=W;
		q++;
	}
	/*While i'v not checked all directions or i found a new path*/
	for(j=0;j<q && ok==false;j++)
	{

		plusMoves=0;
		minusMoves=0;
		foundplus=false;
		foundminus=false;
		ok=false;

		/*This switch is used, based on the Next DIR i want to go:
		If i find other path (Example : want to go S but can't, check if i can do a E-S or W-S , if i can't try E-E-S or W-W-S
		then check which - right or left - is the less MOVES way , after that i'll set new path to be done.) I move to this path , and if it fails i'm bocked.
		Else if I can't find one, I'm blocked*/
		switch(nextdir[j])
		{
			case S:
			{
				for(i=x+1;i<w && foundplus==false;i++)
				{
					plusMoves++;
					if(table[getPos(i,y+1,w)].type!='p' && table[getPos(i,y,w)].type!='p')
					{
						foundplus=true;
					}
				}
				for(i=x-1;i>-1 && foundminus==false;i--)
				{
					minusMoves++;
					if(table[getPos(i,y+1,w)].type!='p' && table[getPos(i,y,w)].type!='p')
					{
						foundminus=true;
					}
				}
				if(foundminus==true || foundplus==true)
				{
					next=S;
					if(plusMoves<minusMoves && foundplus==true)
					{
						ok=true;
						thismoves=E;
						dir[E]+=plusMoves;
						dir[W]+=plusMoves;

					}
					else if(minusMoves<=plusMoves && foundminus==true)
					{
						ok=true;
						thismoves=W;
						dir[E]+=minusMoves;
						dir[W]+=minusMoves;
					}
				}
			}
			break;
			case N:
			{
				for(i=x+1;i<w && foundplus==false;i++)
				{
					plusMoves++;
					if(table[getPos(i,y-1,w)].type!='p' && table[getPos(i,y,w)].type!='p')
					{
						foundplus=true;
					}
				}
				for(i=x-1;i>-1 && foundminus==false;i--)
				{
					minusMoves++;
					if(table[getPos(i,y-1,w)].type!='p' && table[getPos(i,y,w)].type!='p')
					{
						foundminus=true;
					}
				}
				if(foundminus==true || foundplus==true)
				{
					next=N;
					if(plusMoves<minusMoves && foundplus==true)
					{
						ok=true;
						thismoves=E;
						dir[E]+=plusMoves;
						dir[W]+=plusMoves;

					}
					else if(minusMoves<=plusMoves && foundminus==true)
					{
						ok=true;
						thismoves=W;
						dir[E]+=minusMoves;
						dir[W]+=minusMoves;
					}
				}
			}
			break;
			case E:
			{
				for(i=y+1;i<h && foundplus==false;i++)
				{
					plusMoves++;
					if(table[getPos(x+1,i,w)].type!='p' && table[getPos(x,i,w)].type!='p')
					{
						foundplus=true;
					}
				}
				for(i=y-1;i>-1 && foundminus==false;i--)
				{
					minusMoves++;
					if(table[getPos(x+1,i,w)].type!='p' && table[getPos(x,i,w)].type!='p')
					{
						foundminus=true;
					}
				}
					if(foundminus==true || foundplus==true)
				{
					next=E;
					if(plusMoves<minusMoves && foundplus==true)
					{
						ok=true;	
						thismoves=S;
						dir[S]+=plusMoves;
						dir[N]+=plusMoves;

					}
					else if(minusMoves<=plusMoves && foundminus==true)
					{
						ok=true;
						thismoves=N;
						dir[S]+=minusMoves;
						dir[N]+=minusMoves;
					}
				}
			}
			break;
			case W:
			{
				for(i=y+1;i<h && foundplus==false;i++)
				{
					plusMoves++;
					if(table[getPos(x-1,i,w)].type!='p' && table[getPos(x,i,w)].type!='p')
					{
						foundplus=true;
					}
				}
				for(i=y-1;i>-1 && foundminus==false;i--)
				{
					minusMoves++;
					if(table[getPos(x-1,i,w)].type!='p' && table[getPos(x,i,w)].type!='p')
					{
						foundminus=true;
					}
				}
				if(foundminus==true || foundplus==true)
				{
					
					next=W;
					if(plusMoves<minusMoves && foundplus==true)
					{
						ok=true;
						thismoves=S;
						dir[S]+=plusMoves;
						dir[N]+=plusMoves;

					}
					else if(minusMoves<=plusMoves && foundminus==true)
					{
						ok=true;
						thismoves=N;
						dir[S]+=minusMoves;
						dir[N]+=minusMoves;
					}
				}
			}
			break;
		}
		if(ok==true)
		{
				numMsg scored;
				scored.type=type;
			/*printf("I Tried other path %c-%d : dir %d and next %d \n",letter,type,thismoves,next);*/
				myfails=0;
				while(dir[thismoves]>0 && myfails<4)
				{
					if(Move(smap_id,px,py,w,dir,NULL,thismoves,&myfails,&scored,table,letter,0)==true)
						nanosleep(&delay,NULL);
				/*	else
						printf("Failed Moving in NEW Dir \n");*/
				}

				while(Move(smap_id,px,py,w,dir,NULL,next,&myfails,&scored,table,letter,0)!=true && myfails<4);
						nanosleep(&delay,NULL);
				/*if(myfails>=4)			
						printf("Failed Moving in OLD Dir \n");*/
			if(myfails<4 && ok==true)
			{
				/*printf("GOOD FAILS = %d\n",myfails);*/
				return true;
			}
			else
				ok=false;
		}
	}

	if(ok==false)
	{
		/*printf("NEW DIR FALSE\n");*/
		return false;
	}
}

/*IF my flag is taken by someone else I'm STOPPING so I have to get next path (calculated from my FLAG) to current , and delete the useless moves.*/
void DelUselessMoves(int dir[])
{
	if(dir[N]>0 && dir[S]>0)
		{
			if(dir[N]>dir[S])
			{
				dir[N]=dir[N]-dir[S];
				dir[S]=0;
			}
			else
			{
				dir[S]=dir[S]-dir[N];
				dir[N]=0;
			}
		}
		if(dir[W]>0 && dir[E]>0)
		{
			if(dir[W]>dir[E])
			{
				dir[W]=dir[W]-dir[E];
				dir[E]=0;
			}
			else
			{
				dir[E]=dir[E]-dir[W];
				dir[W]=0;
			}
		}
}

/*Termination handler is made to handle ALL possible Signals that come, with a simple exit(0)*/
void termination_handler(int sig)
{
	exit(0);
}