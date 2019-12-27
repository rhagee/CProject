
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

#include "lib/sharedmem.h"

#define intconvert(X) atoi(argv[X]);
#define longconvert(X) atol(argv[X]);

void Move(int smap_id,int* px,int* py,int w,int dir[],int* pindex,int index,int* pfailed,cell* table,char letter,int nextCardinal);


int main(int argc,char* argv[])
{
	char letter;
	int term;
	int posMsg_id,instrMsg_id,opt_id,map_id,smap_id,sync_id,index,x,y,w,h;
	int dir[4];
	int failed;
	long type;
	posMsg pos_message;
	instrMsg instr_message;
	cell* table;
	options* settings;

	opt_id=intconvert(1);
	map_id=intconvert(2);
	smap_id=intconvert(3);
    letter = intconvert(4);
    posMsg_id = intconvert(5);
	type = longconvert(6);
	instrMsg_id=intconvert(7);
	sync_id=intconvert(8);
	settings=getOptions(opt_id);
	failed=0;
	term=0;

	table=getMap(map_id);
	read_posMsg(posMsg_id,&pos_message,type);
	instr_message.left=1;
	printf("pawn : %c - %d , POS: x:%d y:%d\n",letter,type,pos_message.x,pos_message.y);
	x=pos_message.x;
	y=pos_message.y;
	w=settings->SO_BASE;
	h=settings->SO_ALTEZZA;
	semWaitZero(sync_id,ROUNDSTART,0);
	semReserve(sync_id,ALLSTARTED,0);

	while(instr_message.left>0)
	{
		read_instrMsg(instrMsg_id,&instr_message,type);
		if(instr_message.left!=-1)
		printf("pawn : %c - %d MOVE: N: %d S: %d W: %d E: %d -> left instr %d\n",letter,instr_message.type,instr_message.dir[N],instr_message.dir[S],instr_message.dir[W],instr_message.dir[E],instr_message.left);
		dir[N]=instr_message.dir[N];
		dir[S]=instr_message.dir[S];
		dir[W]=instr_message.dir[W];
		dir[E]=instr_message.dir[E];
		index=0;
		while((dir[N]>0 || dir[S]>0 || dir[W]>0 || dir[E]>0) && term==0)
		{
			if(dir[index]==0)
			{
				if(index<3)
					index++;
				else
					index=0;
			}
			else if(failed>=4)
			{
				if((dir[N]+dir[S]+dir[W]+dir[E])==1)
					term=2;



				/*Implementare l'auto movimento cercando di capire se effettivamente sono bloccato o meno*/
				/*Necessito anche di controllare se POSSO muovermi verso il lato che scelgo (E-W), altrimenti vuol dire che sono bloccato*/
				/*Implementare la capacità della pedina di riconoscere la sua possibilità di raggiungimento dell'obiettivo ogni movimento aggiunto.
				Se è incapace deve rimanere immobile, altrimenti deve trovare la soluzione di movimento*/



				/*else
				{

					if(dir[N]>0)
					{
						dir[W]++;
						dir[E]++;
						if(dir[E]>0)
							Move(smap_id,&x,&y,w,dir,&index,W,&failed,table,letter,N);
						else
							Move(smap_id,&x,&y,w,dir,&index,E,&failed,table,letter,N);
					}
					else if(dir[S]>0)
					{
						dir[W]++;
						dir[E]++;
						if(dir[E]>0)
							Move(smap_id,&x,&y,w,dir,&index,W,&failed,table,letter,S);
						else
							Move(smap_id,&x,&y,w,dir,&index,E,&failed,table,letter,S);
					}
					else if(dir[E]>0)
					{
						dir[S]++;
						dir[N]++;
						if(dir[N]>0)
							Move(smap_id,&x,&y,w,dir,&index,S,&failed,table,letter,E);
						else
							Move(smap_id,&x,&y,w,dir,&index,N,&failed,table,letter,E);
					}
					else if(dir[W]>0)
					{
						dir[S]++;
						dir[N]++;
						if(dir[N]>0)
							Move(smap_id,&x,&y,w,dir,&index,S,&failed,table,letter,W);
						else
							Move(smap_id,&x,&y,w,dir,&index,N,&failed,table,letter,W);
					}
				}*/
				term = 1;
				failed=0;
			}
			else
			{
				if(index<3)
					Move(smap_id,&x,&y,w,dir,&index,index,&failed,table,letter,index+1);
				else
					Move(smap_id,&x,&y,w,dir,&index,index,&failed,table,letter,0);
			}
		}
	}
	exit(0);
}

void Move(int smap_id,int* px,int* py,int w,int dir[],int* pindex,int index,int* pfailed,cell* table,char letter,int nextCardinal)
{
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
		printf("\n ERROR x:%d y:%d\n",x+1,y);
		*pindex=nextCardinal;
		*pfailed=*pfailed+1;
	}
	else
	{
		semRelease(smap_id,actualpos,0);
		table[actualpos].val=0;
		table[actualpos].type='z';
		*pfailed=0;
		table[nextpos].val=letter;
		table[nextpos].type='p';
		*px = x+incX;
		*py= y+incY;
		dir[index]--;
	}
	nanosleep(100000000);
}


int getPos(int x,int y,int w)
{
	return ((y*w)+x);
}