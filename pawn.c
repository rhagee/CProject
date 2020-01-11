
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
#include <sys/time.h>
#include <time.h>

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
int findOtherPath(int dir[],cell* table,int,int,int*,int*,int,char,struct timespec delay,int type,int moves);
void termination_handler(int sig);

int main(int argc,char* argv[])
{
	struct timespec delay;

	char letter;
	int term;
	int moves;
	int scoreToPlayerMsg_id,posMsg_id,instrMsg_id,leftMoves_id,newPos_id,opt_id,map_id,smap_id,sync_id,index,x,y,w,h;
	int dir[4];
	int failed;
	long type;
	posMsg pos_message;
	instrMsg instr_message;
	numMsg leftMoves;
	numMsg newPos;
	numMsg scored;
	cell* table;
	options* settings;

	signal(SIGINT,termination_handler);
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
	settings=getOptions(opt_id);
	failed=0;
	term=0;

	delay.tv_sec=0;
	delay.tv_nsec=settings->SO_MIN_HOLD_NSEC;
	table=getMap(map_id);
	read_posMsg(posMsg_id,&pos_message,type);
	instr_message.left=1;
	/*printf("pawn : %c - %d , POS: x:%d y:%d\n",letter,type,pos_message.x,pos_message.y);*/
	x=pos_message.x;
	y=pos_message.y;
	w=settings->SO_BASE;
	h=settings->SO_ALTEZZA;
	moves=settings->SO_N_MOVES;
	/* Init of directions */

	/*  ROUND ROUTINE */

	while(1){ 
	dir[N]=0;
	dir[S]=0;
	dir[W]=0;
	dir[E]=0;
	/*WAIT round start and signal that i heard Round start*/
	semWaitZero(sync_id,ROUNDSTART,0);
	semReserve(sync_id,ALLSTARTED,0);
	instr_message.left=1;
	while(instr_message.left>0)
	{
		term=0;
		failed=0;
		read_instrMsg(instrMsg_id,&instr_message,type);
		/*if(instr_message.left!=-1)
			printf("pawn : %c - %d MOVE: N: %d S: %d W: %d E: %d -> left instr %d\n",letter,instr_message.type,instr_message.dir[N],instr_message.dir[S],instr_message.dir[W],instr_message.dir[E],instr_message.left);*/
		dir[N]+=instr_message.dir[N];
		dir[S]+=instr_message.dir[S];
		dir[W]+=instr_message.dir[W];
		dir[E]+=instr_message.dir[E];

		/*Delete useless moves*/
		DelUselessMoves(dir);

		index=0;
		while((dir[N]>0 || dir[S]>0 || dir[W]>0 || dir[E]>0) && term==0 && table[getPos((x+dir[E]-dir[W]),(y+dir[S]-dir[N]),w)].type=='f')
		{
			if(dir[index]<=0 && failed<4)
			{
				if(index<3)
					index++;
				else
					index=0;
			}
			else if(moves==0)
			{
				term=STOPMOVE;
			}
			else if(failed>=4)
			{
				if((dir[N]+dir[S]+dir[W]+dir[E])==1)
				{
					term=STOPMOVE;
					failed=0;
				}
				else
				{
					printf("FIND OTHER PATH\n");
					if((findOtherPath(dir,table,w,h,&x,&y,smap_id,letter,delay,type,moves)) == false)
					{
						term=STOPMOVE;
					}
					else
					{
						failed=0;
					}
				}
				/*else
					alterMove(dir,w,h,x,y);*/
				/*Implementare l'auto movimento cercando di capire se effettivamente sono bloccato o meno*/
				/*Necessito anche di controllare se POSSO muovermi verso il lato che scelgo (E-W), altrimenti vuol dire che sono bloccato*/
				/*Implementare la capacità della pedina di riconoscere la sua possibilità di raggiungimento dell'obiettivo ogni movimento aggiunto.
				Se è incapace deve rimanere immobile, altrimenti deve trovare la soluzione di movimento*/


/*
				term = ADDMOVE;
				failed=0;*/
			}
			else
			{
					if(Move(smap_id,&x,&y,w,dir,&index,index,&failed,&scored,table,letter,((index+1)%4))==true)
					{
						moves--;
					}
					nanosleep(&delay,NULL);
			}
		}
		if(instr_message.left>=0 && table[getPos((x+dir[E]-dir[W]),(y+dir[S]-dir[N]),w)].type!='f' && dir[N]+dir[S]+dir[W]+dir[E]>0)
		{
			scored.q = 0;
			scored.type=type;
			send_numMsg(scoreToPlayerMsg_id,&scored);
			printf("MISS for Pawn : %c-%d -> MY LAST INDEXES N:%d S:%d W:%d E: %d and Moves Left : %d\n",letter,type,dir[N],dir[S],dir[W],dir[E],moves);
		}
		else if(instr_message.left<0)
		{
			printf("I'm NOT moving\n");
		}
		else if(instr_message.left>=0 && dir[N]+dir[S]+dir[W]+dir[E]==0)
		{
			scored.type=type;
			send_numMsg(scoreToPlayerMsg_id,&scored);
			printf("HIT for Pawn : %c-%d -> MY LAST INDEXES N:%d S:%d W:%d E: %d and Moves Left : %d\n",letter,type,dir[N],dir[S],dir[W],dir[E],moves);
		}
		else if(term==2)
		{
			scored.q = 0;
			scored.type=type;
			send_numMsg(scoreToPlayerMsg_id,&scored);
			printf("FAILED MOVING \n");
		}
	}
	/*Send Message With Number of Left Moves*/
	leftMoves.type=type;
	leftMoves.q = moves;	
	send_numMsg(leftMoves_id,&leftMoves);
	newPos.type=type;
	newPos.q=getPos(x,y,w);
	send_numMsg(newPos_id,&newPos);
	semWaitZero(sync_id,ROUNDEND,0);
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
		if(table[nextpos].type='f' && pScore!=NULL)
		{
			pScore->q = table[nextpos].val;
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


int getPos(int x,int y,int w)
{
	return ((y*w)+x);
}

int findOtherPath(int dir[],cell* table,int w,int h,int* px,int* py,int smap_id,char letter,struct timespec delay,int type,int moves)
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
	for(j=0;j<q && ok==false;j++)
	{
		plusMoves=0;
		minusMoves=0;
		foundplus=false;
		foundminus=false;
		ok=false;
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
					ok=true;
					next=S;
					if(plusMoves<minusMoves && foundplus==true)
					{
						thismoves=E;
						dir[E]+=plusMoves;
						dir[W]+=plusMoves;

					}
					else if(minusMoves<=plusMoves && foundminus==true)
					{
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
					ok=true;
					next=N;
					if(plusMoves<minusMoves && foundplus==true)
					{
						thismoves=E;
						dir[E]+=plusMoves;
						dir[W]+=plusMoves;

					}
					else if(minusMoves<=plusMoves && foundminus==true)
					{
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
					ok=true;
					next=E;
					if(plusMoves<minusMoves && foundplus==true)
					{
						
						thismoves=S;
						dir[S]+=plusMoves;
						dir[N]+=plusMoves;

					}
					else if(minusMoves<=plusMoves && foundminus==true)
					{
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
					ok=true;
					next=W;
					if(plusMoves<minusMoves && foundplus==true)
					{
						
						thismoves=S;
						dir[S]+=plusMoves;
						dir[N]+=plusMoves;

					}
					else if(minusMoves<=plusMoves && foundminus==true)
					{
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
			/*printf("I Tried other path %c-%d : dir %d and next %d \n",letter,type,thismoves,next);*/
				myfails=0;
				while(dir[thismoves]>0 && myfails<4)
				{
					if(Move(smap_id,px,py,w,dir,NULL,thismoves,&myfails,NULL,table,letter,0)==true)
						nanosleep(&delay,NULL);
					else
						printf("Failed Moving in NEW Dir \n");
				}

				while(Move(smap_id,px,py,w,dir,NULL,next,&myfails,NULL,table,letter,0)!=true && myfails<4);
						nanosleep(&delay,NULL);
				if(myfails>=4)			
						printf("Failed Moving in OLD Dir \n");
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
		return false;
	}
}


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

void termination_handler(int sig)
{
	exit(0);
}