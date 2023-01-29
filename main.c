#include "local.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GL/glut.h>


//struct message msg;
//static char buffer[B_SIZ];
int WINDOW_HEIGHT = 1000 ;
int WINDOW_WIDTH = 1700 ;
int updateTime = 500; //microseconds

int minA,minB,minC,maxA,maxB,maxC;
int Aline[3],Bline[2],Cline[2];
int AChoco=0,BChoco=0,CChoco=0;
int AChoco_basket=0,BChoco_basket=0,CChoco_basket=0;
int APatches=0,BPatches=0,CPatches=0;
int AStorage=0, BStorage=0, CStorage=0;
int exp_print_time;
int storage_time;
int storage_max_capacity;
int storage_min_capacity;
int AContainer=0,BContainer=0,CContainer=0;
int ACarton=0,BCarton=0,CCarton=0;
int exec_time=0;
int terminate=0;

int TruckOrder;
int CurrentTruck[3];
int AShipped=0,BShipped=0,CShipped=0;



int A_truck_max,B_truck_max,C_truck_max;
int truck_time;
int A_max_shipped,B_max_shipped,C_max_shipped;

char MISSING;

char patch_in_EXP = 'N';

pthread_mutex_t Achoc_mutex, Bchoc_mutex, Cchoc_mutex;
pthread_mutex_t APatch_mutex, BPatch_mutex, CPatch_mutex;
pthread_mutex_t Aline_mutex[3],Bline_mutex[2],Cline_mutex[2],EXP_mutex,FC_mutex[3],CARTON_mutex[3],STORAGE_mutex[2],LOAD_mutex[2],TRUCK_mutex[3];
pthread_cond_t Aline_cond=PTHREAD_COND_INITIALIZER,
Bline_cond=PTHREAD_COND_INITIALIZER,
Cline_cond=PTHREAD_COND_INITIALIZER;

pthread_cond_t EXP_cond=PTHREAD_COND_INITIALIZER,
FC_cond=PTHREAD_COND_INITIALIZER,
CARTON_cond=PTHREAD_COND_INITIALIZER,
STORAGE_cond=PTHREAD_COND_INITIALIZER,
TRUCK_cond=PTHREAD_COND_INITIALIZER,
LOAD_cond=PTHREAD_COND_INITIALIZER;

//pthread_mutex_t line_mutex[3]; //mutex for every line.
int Anum_chocs_at_stage[3][8];  //number of chocolates ready for / being processed at every stage.
int Bnum_chocs_at_stage[2][6];  //number of chocolates ready for / being processed at every stage.
int Cnum_chocs_at_stage[2][5];  //number of chocolates ready for / being processed at every stage.

void LOADtypeEmployee(struct emp_info*);
void AtypeEmployee(struct emp_info*);
void BtypeEmployee(struct emp_info*);
void CtypeEmployee(struct emp_info*);
void EXPtypeEmployee(struct emp_info*);
void FCtypeEmployee(struct emp_info*);
void CARTONtypeEmployee(struct emp_info*);
void STORAGEtypeEmployee(struct emp_info*);
void TRUCKtypeEmployee(struct emp_info*);

void opengl();


void time_exceeded();
void printChocStage();

int should_exit = 0;
int suspend_line = 0;

void read_file(){
	char buffer[INPUT_LINES][MAX_BUF];

	FILE *data;
	char filename[MAX_BUF]="data.txt";
	data=fopen(filename,"r");
	if(data==NULL){
		perror("Error opening input file");
		exit(-1);
	}

	for(int i=0;i<INPUT_LINES;i++){
		while(fgets(buffer[i],sizeof(buffer[i]),data)!=NULL){
			i++;
		}
	}

	//parse values to variables:
	int stats[INPUT_LINES];
	for(int i=0;i<INPUT_LINES;i++){
		char *token;
		token=strtok(buffer[i]," ");
		token=strtok(NULL," ");
		stats[i]=atoi(token);
	}
	
	minA=stats[0];
	maxA=stats[1];
	minB=stats[2];
	maxB=stats[3];
	minC=stats[4];
	maxC=stats[5];
	exp_print_time=stats[6];
	storage_time=stats[7];
	storage_max_capacity=stats[8];
	storage_min_capacity=stats[9];
	A_truck_max=stats[10];
	B_truck_max=stats[11];
	C_truck_max=stats[12];
	truck_time=stats[13];
	A_max_shipped=stats[14];
	B_max_shipped=stats[15];
	C_max_shipped=stats[16];
	exec_time=stats[17];
	printf("\n\t\tEXEC TIME:%dmin",exec_time);
	
}






void changeMutexVar(pthread_mutex_t* mut, int* var, int difference){
	pthread_mutex_lock(mut);
	*var = *var + difference;
	pthread_mutex_unlock(mut);
}



//_____________________________________________________________________________________Main

int main(){

	read_file();	
	sigset(SIGALRM,time_exceeded);

	pthread_t opengl_thread;
	if(pthread_create(&opengl_thread, NULL, (void*)opengl,NULL))
	exit(-1);
	sleep(3);
	pthread_t AtypeThreads[3][8];//3 lines with 8 employees each
	pthread_t BtypeThreads[2][6];
	pthread_t CtypeThreads[2][5];
	pthread_t EXPtypeThreads[2];//patches employees
	pthread_t FCtypeThreads[3];//filling containers
	pthread_t CARTONtypeThreads[3];//filling cartoons
	pthread_t STORAGEtypeThreads[2];//filling storage
	pthread_t LOADtypeThreads[2];
	pthread_t TRUCKtypeThreads[3];
	TruckOrder=0;


	pthread_mutex_init(&Achoc_mutex, NULL);
	pthread_mutex_init(&Bchoc_mutex, NULL);
	pthread_mutex_init(&Cchoc_mutex, NULL);

	pthread_mutex_init(&APatch_mutex, NULL);
	pthread_mutex_init(&BPatch_mutex, NULL);
	pthread_mutex_init(&CPatch_mutex, NULL);
	
	//initialize line A variables
	for(int i=0;i<3;i++)for(int j=0;j<8;j++)Anum_chocs_at_stage[i][j]=0;
	for(int i=0; i<3;i++)pthread_mutex_init(&Aline_mutex[i], NULL);
	
	//type A chocolate threads creation
	for(int i=0; i<3; i++){
		for(int j=0;j<8;j++){
			struct emp_info * info;
			info=malloc(sizeof(struct emp_info));
			(*info).line_id=i;
			(*info).emp_id=8*i+j;
			char *type="A";
			strcpy((*info).chocolate_type, type);
			if(pthread_create(&AtypeThreads[i][j],NULL,(void*)AtypeEmployee,info))
				exit(-1);
		}
	}

	

	//initialize line B variables
	for(int i=0;i<2;i++)for(int j=0;j<6;j++)Bnum_chocs_at_stage[i][j]=0;
	for(int i=0; i<2; i++)pthread_mutex_init(&Bline_mutex[i], NULL);
	
	//type B chocolate threads creation
	for(int i=0; i<2; i++){
		for(int j=0;j<6;j++){
			struct emp_info * info;
			info=malloc(sizeof(struct emp_info));
			(*info).line_id=i;
			(*info).emp_id=6*i+j;
			char *type="B";
			strcpy((*info).chocolate_type, type);
			if(pthread_create(&BtypeThreads[i][j],NULL,(void*)BtypeEmployee,info))
				exit(-1);
		}
	}

		//initialize line C variables
	for(int i=0;i<2;i++)for(int j=0;j<5;j++)Cnum_chocs_at_stage[i][j]=0;
	for(int i=0; i<2; i++)pthread_mutex_init(&Cline_mutex[i], NULL);
	

	//type C chocolate threads creation
	for(int i=0; i<2; i++){
		for(int j=0;j<5;j++){
			struct emp_info * info;
			info=malloc(sizeof(struct emp_info));
			(*info).line_id=i;
			(*info).emp_id=5*i+j;
			char *type="C";
			strcpy((*info).chocolate_type, type);
			if(pthread_create(&CtypeThreads[i][j],NULL,(void*)CtypeEmployee,info))
				exit(-1);
		}
	}
  
        //_____________________________________________________________addition

	    //patches of 10 of each type (exp date)
        for(int i=0;i<2;i++){
		struct emp_info * info;
		info=malloc(sizeof(struct emp_info));
		(*info).line_id=8;
		(*info).emp_id=i;
		char *type="EXP";
		strcpy((*info).chocolate_type, type);
		if(pthread_create(&EXPtypeThreads[i],NULL,(void*)EXPtypeEmployee,info))
			exit(-1);
		}
		pthread_mutex_init(&EXP_mutex, NULL);
        
	/*
        //fill containers
        for(int i=0;i<3;i++){
		struct emp_info * info;
		info=malloc(sizeof(struct emp_info));
		(*info).line_id=9;
		(*info).emp_id=i;
		char *type="FC";
		strcpy((*info).chocolate_type, type);
		if(pthread_create(&FCtypeThreads[i],NULL,(void*)FCtypeEmployee,info))
			exit(-1);
	}
	for (int i=0; i<3; i++){
		pthread_mutex_init(&FC_mutex[i], NULL);
        }
*/
	//__________________________________________________________________End

	 //fill containers
        for(int i=0;i<3;i++){
		struct emp_info * info;
		info=malloc(sizeof(struct emp_info));
		(*info).line_id=9;
		(*info).emp_id=i;
		char *type="FC";
		strcpy((*info).chocolate_type, type);
		if(pthread_create(&FCtypeThreads[i],NULL,(void*)FCtypeEmployee,info))
			exit(-1);
	}
	for (int i=0; i<3; i++){
		pthread_mutex_init(&FC_mutex[i], NULL);
        }
        
        //fill cartons
        for(int i=0;i<3;i++){
		struct emp_info * info;
		info=malloc(sizeof(struct emp_info));
		(*info).line_id=10;
		(*info).emp_id=i;
		char *type="CRT";
		strcpy((*info).chocolate_type, type);
		if(pthread_create(&CARTONtypeThreads[i],NULL,(void*)CARTONtypeEmployee,info))
			exit(-1);
	}
	for (int i=0; i<3; i++){
		pthread_mutex_init(&CARTON_mutex[i], NULL);
        }
        
        //storage employees
        for(int i=0;i<2;i++){
		struct emp_info * info;
		info=malloc(sizeof(struct emp_info));
		(*info).line_id=11;
		(*info).emp_id=i;
		char *type="STG";
		strcpy((*info).chocolate_type, type);
		if(pthread_create(&STORAGEtypeThreads[i],NULL,(void*)STORAGEtypeEmployee,info))
			exit(-1);
	}
	for (int i=0; i<3; i++){
		pthread_mutex_init(&STORAGE_mutex[i], NULL);
        }
        
        //load truck employees
        for(int i=0;i<2;i++){
		struct emp_info * info;
		info=malloc(sizeof(struct emp_info));
		(*info).line_id=12;
		(*info).emp_id=i;
		char *type="LOD";
		strcpy((*info).chocolate_type, type);
		if(pthread_create(&LOADtypeThreads[i],NULL,(void*)LOADtypeEmployee,info))
			exit(-1);
	}
	for (int i=0; i<2; i++){
		pthread_mutex_init(&LOAD_mutex[i], NULL);
        }
        
        //trucks
        for(int i=0;i<3;i++){
		struct emp_info * info;
		info=malloc(sizeof(struct emp_info));
		(*info).line_id=13;
		(*info).emp_id=i;
		char *type="TRK";
		strcpy((*info).chocolate_type, type);
		if(pthread_create(&TRUCKtypeThreads[i],NULL,(void*)TRUCKtypeEmployee,info))
			exit(-1);
	}
	for (int i=0; i<3; i++){
		pthread_mutex_init(&TRUCK_mutex[i], NULL);
        }
        
        alarm(exec_time*60);
		
	while(1){//temporary because we do not want parent to exit
		usleep(700000);
		if(terminate){
			for(int i=0;i<3;i++){
				for(int j=0;j<8;j++){
					pthread_cancel(AtypeThreads[i][j]);
				}
			}
			for(int i=0;i<2;i++){
				for(int j=0;j<6;j++){
					pthread_cancel(BtypeThreads[i][j]);
				}
			}
			for(int i=0;i<2;i++){
				for(int j=0;j<5;j++){
					pthread_cancel(CtypeThreads[i][j]);
				}
			}
			for(int i=0;i<2;i++){
				pthread_cancel(EXPtypeThreads[i]);
			}
			for(int i=0;i<3;i++){
				pthread_cancel(FCtypeThreads[i]);
			}
			for(int i=0;i<3;i++){
				pthread_cancel(CARTONtypeThreads[i]);
			}
			for(int i=0;i<2;i++){
				pthread_cancel(STORAGEtypeThreads[i]);
			}
			for(int i=0;i<2;i++){
				pthread_cancel(LOADtypeThreads[i]);
			}
			for(int i=0;i<3;i++){
				pthread_cancel(TRUCKtypeThreads[i]);
			}
		
			return 0;
		}
	}

	while(1){
		printChocStage();
		sleep(10);
	}
	return 0;

}


void printChocStage(){
	for(int i=0;i<3;i++){
	printf("\n");
	for(int j=0;j<8;j++){
		printf(MAGENTA"%d "RESET, Anum_chocs_at_stage[i][j]);
	}
	printf("\n");
	}

	for(int i=0;i<2;i++){
	printf("\n");
	for(int j=0;j<6;j++){
		printf(MAGENTA"%d "RESET, Bnum_chocs_at_stage[i][j]);
	}
	printf("\n");
	}

	for(int i=0;i<2;i++){
	printf("\n");
	for(int j=0;j<5;j++){
		printf(MAGENTA"%d "RESET, Cnum_chocs_at_stage[i][j]);
	}
	printf("\n");
	}
}



void AtypeEmployee(struct emp_info* data){
	int line=data->line_id;
	int id=data->emp_id;
	char chocolate_type[4];
	strcpy(chocolate_type, data->chocolate_type);

	printf(RED"\n\t\t\tEmployee thread created: line is = %d, id is = %d, Chocolate Type is %s created,ID=%ld"RESET, line, id, chocolate_type, pthread_self());
	fflush(stdout);
	
	id=id%8;
	int chocolates=0;
	srand(time(NULL));
	//sleep(2);
	
	
	while(1){
		if(should_exit)
        {
            break;
        }
        while(suspend_line)
        {
            sleep(1);
		}
		sleep(1);
		if(id==0){   
			int time=rand()%(maxA-minA+1)+minA;
			sleep(time);
			pthread_mutex_lock(&Aline_mutex[line]);
			Anum_chocs_at_stage[line][1]++;
			//printf(RED"\nA Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
			pthread_mutex_unlock(&Aline_mutex[line]);
			usleep(500000);
		}
		else if(id==1 && Anum_chocs_at_stage[line][1] > 0){
			int time=rand()%(maxA-minA+1)+minA;
			sleep(time);
			pthread_mutex_lock(&Aline_mutex[line]);
			Anum_chocs_at_stage[line][1]--;
			Anum_chocs_at_stage[line][2]++;
			//printf(RED"\nA Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
			pthread_mutex_unlock(&Aline_mutex[line]);
		}
		else if(id==2 && Anum_chocs_at_stage[line][2] > 0){
			int time=rand()%(maxA-minA+1)+minA;
			sleep(time);
			pthread_mutex_lock(&Aline_mutex[line]);
			Anum_chocs_at_stage[line][2]--;
			Anum_chocs_at_stage[line][3]++;
			//printf(RED"\nA Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
			pthread_mutex_unlock(&Aline_mutex[line]);
		}
		else if(id==3 && Anum_chocs_at_stage[line][3] > 0){
			int time=rand()%(maxA-minA+1)+minA;
			sleep(time);
			pthread_mutex_lock(&Aline_mutex[line]);
			Anum_chocs_at_stage[line][3]--;
			Anum_chocs_at_stage[line][4]++;
			//printf(RED"\nA Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
			pthread_mutex_unlock(&Aline_mutex[line]);
		}
		else if(id>=4){
			for(int i=4;i<8;i++){  
				pthread_mutex_lock(&Aline_mutex[line]);
				if(Anum_chocs_at_stage[line][i] > 0){
						Anum_chocs_at_stage[line][i]--;
						if(i != 7)Anum_chocs_at_stage[line][i+1]++;
						pthread_mutex_unlock(&Aline_mutex[line]);
						int time=rand()%(maxA-minA+1)+minA;
						sleep(time);
						//printf(RED"\nA Emp %d line %d passed choc in %d mins"RESET,id,line,time);
						if(i==7){
							changeMutexVar(&Achoc_mutex, &AChoco, 1);
							//changeMutexVar(&Achoc_mutex, &AChoco_basket, 1);
							if(AChoco > 10){
								changeMutexVar(&Achoc_mutex, &AChoco, -10);printf(RED"\n		+	Apatch %d \n"RESET, APatches);
								changeMutexVar(&APatch_mutex, &APatches, 1);printf(RED"\n		+	Apatch %d \n"RESET, APatches);
							}
							//printf(YELLOW"\n			AChocolate count = %d last made by line %d \n"RESET,AChoco, line);
						}
						//fflush(stdout);
				}else pthread_mutex_unlock(&Aline_mutex[line]);
				sleep(rand()%2);
						

			}
		}
	}
}
	





	void BtypeEmployee(struct emp_info* data){
	int line=data->line_id;
	int id=data->emp_id;
	char chocolate_type[4];
	strcpy(chocolate_type, data->chocolate_type);

	printf(RED"\n\t\t\tEmployee thread created: line is = %d, id is = %d, Chocolate Type is %s created,ID=%ld"RESET, line, id, chocolate_type, pthread_self());
	fflush(stdout);
	
	id=id%8;

	srand(time(NULL));
	//sleep(2);
	
	
	while(1){
		if(should_exit)
        {
            break;
        }
        while(suspend_line)
        {
            sleep(1);
		}
		sleep(1);
		if(id==0){   
			int time=rand()%(maxB-minB+1)+minB;
			sleep(time);
			pthread_mutex_lock(&Bline_mutex[line]);
			Bnum_chocs_at_stage[line][1]++;
			pthread_mutex_unlock(&Bline_mutex[line]);
			//printf(RED"\nB Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
			usleep(500000);
		}
		else if(id==1 && Bnum_chocs_at_stage[line][1] > 0){
			int time=rand()%(maxB-minB+1)+minB;
			sleep(time);
			pthread_mutex_lock(&Bline_mutex[line]);
			Bnum_chocs_at_stage[line][1]--;
			Bnum_chocs_at_stage[line][2]++;
			pthread_mutex_unlock(&Bline_mutex[line]);
			//printf(RED"\nB Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
		}
		else if(id==2 && Bnum_chocs_at_stage[line][2] > 0){
			int time=rand()%(maxB-minB+1)+minB;
			sleep(time);
			pthread_mutex_lock(&Bline_mutex[line]);
			Bnum_chocs_at_stage[line][2]--;
			Bnum_chocs_at_stage[line][3]++;
			pthread_mutex_unlock(&Bline_mutex[line]);
			//printf(RED"\nB Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
		}
		else if(id==3 && Bnum_chocs_at_stage[line][3] > 0){
			int time=rand()%(maxB-minB+1)+minB;
			sleep(time);
			pthread_mutex_lock(&Bline_mutex[line]);
			Bnum_chocs_at_stage[line][3]--;
			Bnum_chocs_at_stage[line][4]++;
			pthread_mutex_unlock(&Bline_mutex[line]);
			//printf(RED"\nB Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
		}
		else if(id==4 && Bnum_chocs_at_stage[line][4] > 0){
			int time=rand()%(maxB-minB+1)+minB;
			sleep(time);
			pthread_mutex_lock(&Bline_mutex[line]);
			Bnum_chocs_at_stage[line][4]--;
			Bnum_chocs_at_stage[line][5]++;
			pthread_mutex_unlock(&Bline_mutex[line]);
			//printf(RED"\nB Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
		}
		else if(id==5 && Bnum_chocs_at_stage[line][5] > 0){
			int time=rand()%(maxB-minB+1)+minB;
			sleep(time);
			pthread_mutex_lock(&Bline_mutex[line]);
			Bnum_chocs_at_stage[line][5]--;
			pthread_mutex_unlock(&Bline_mutex[line]);
			//printf(RED"\nB Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			changeMutexVar(&Bchoc_mutex, &BChoco, 1);
			changeMutexVar(&Bchoc_mutex, &BChoco_basket, 1);
			if(BChoco > 10){
				changeMutexVar(&Bchoc_mutex, &BChoco, -10);
				changeMutexVar(&BPatch_mutex, &BPatches, 1);
			}
			//printf(YELLOW"\n			BChocolate count = %d last made by line %d \n"RESET,BChoco, line);
			//fflush(stdout);
		}

	}
}






void CtypeEmployee(struct emp_info* data){
	int line=data->line_id;
	int id=data->emp_id;
	char chocolate_type[4];
	strcpy(chocolate_type, data->chocolate_type);

	printf(RED"\n\t\t\tEmployee thread created: line is = %d, id is = %d, Chocolate Type is %s created,ID=%ld"RESET, line, id, chocolate_type, pthread_self());
	fflush(stdout);
	
	id=id%8;
	srand(time(NULL));
	//sleep(2);
	
	
	while(1){
		if(should_exit)
        {
            break;
        }
        while(suspend_line)
        {
            sleep(1);
		}
		sleep(1);
		if(id==0){   
			int time=rand()%(maxC-minC+1)+minC;
			sleep(time);
			pthread_mutex_lock(&Cline_mutex[line]);
			Cnum_chocs_at_stage[line][1]++;
			pthread_mutex_unlock(&Cline_mutex[line]);
			//printf(RED"\nC Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
			usleep(500000);
		}
		else if(id==1 && Cnum_chocs_at_stage[line][1] > 0){
			int time=rand()%(maxC-minC+1)+minC;
			sleep(time);
			pthread_mutex_lock(&Cline_mutex[line]);
			Cnum_chocs_at_stage[line][1]--;
			Cnum_chocs_at_stage[line][2]++;
			pthread_mutex_unlock(&Cline_mutex[line]);
			//printf(RED"\nC Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
		}
		else if(id==2 && Cnum_chocs_at_stage[line][2] > 0){
			int time=rand()%(maxC-minC+1)+minC;
			sleep(time);
			pthread_mutex_lock(&Cline_mutex[line]);
			Cnum_chocs_at_stage[line][2]--;
			Cnum_chocs_at_stage[line][3]++;
			pthread_mutex_unlock(&Cline_mutex[line]);
			//printf(RED"\nC Emp %d line %d passed choc in %d mins"RESET,id,line,time);
			//fflush(stdout);
		}
		else if(id>=3){
			for(int i=3;i<5;i++){  
				pthread_mutex_lock(&Cline_mutex[line]);
				if(Cnum_chocs_at_stage[line][i] > 0){
						Cnum_chocs_at_stage[line][i]--;
						if(i != 4)Cnum_chocs_at_stage[line][i+1]++;
						pthread_mutex_unlock(&Cline_mutex[line]);
						int time=rand()%(maxC-minC+1)+minC;
						sleep(time);
						//printf(RED"\nC Emp %d line %d passed choc in %d mins"RESET,id,line,time);
						if(i==4){
							changeMutexVar(&Cchoc_mutex, &CChoco, 1);
							changeMutexVar(&Cchoc_mutex, &CChoco_basket, 1);
							//printf(YELLOW"\n			CChocolate count = %d last made by line %d \n"RESET,CChoco, line);
							if(CChoco > 10){
								changeMutexVar(&Cchoc_mutex, &CChoco, -10);
								changeMutexVar(&CPatch_mutex, &CPatches, 1);
								//pthread_cond_signal(&EXP_cond);
							}
						}
						//fflush(stdout);
				}else pthread_mutex_unlock(&Cline_mutex[line]);
				sleep(rand()%2);
			}
		}
	}
}



void EXPtypeEmployee(struct emp_info* data){
	int line=data->line_id;
	int id=data->emp_id;
	char chocolate_type[4];
	strcpy(chocolate_type, data->chocolate_type);
	printf(YELLOW"\t\t\n\nEXP EMPLOYEE %d READY"RESET,id);
	fflush(stdout);
	
	while(1){

			if(AStorage+BStorage+CStorage<storage_max_capacity){

			pthread_mutex_lock(&EXP_mutex);
			///pthread_cond_wait(&EXP_cond,&EXP_mutex);
			if(APatches + BPatches + CPatches > 0){
				if(APatches > 0){
					//changeMutexVar(&APatch_mutex, &APatches, -1);
					patch_in_EXP = 'A';
					pthread_cond_signal(&FC_cond);
				}else if(BPatches > 0){
					//changeMutexVar(&BPatch_mutex, &BPatches, -1);
					patch_in_EXP = 'B';
					pthread_cond_signal(&FC_cond);
				}else if(CPatches > 0){
					//changeMutexVar(&CPatch_mutex, &CPatches, -1);
					patch_in_EXP = 'C';
					pthread_cond_signal(&FC_cond);
				}
				printf("\n (%c) \n",patch_in_EXP);
				printf(YELLOW"\t\t\n\n patch entered into EXP: A PATCHES:%d, B PATCHES:%d, C PATCHES:%d"RESET,APatches,BPatches,CPatches);
				fflush(stdout);
				//sleep(exp_print_time);
				usleep(1500000);
				patch_in_EXP = 'N';
			}
			pthread_mutex_unlock(&EXP_mutex);

			}else{
				pthread_mutex_lock(&EXP_mutex);
				while(AStorage+BStorage+CStorage >= storage_min_capacity){
					usleep(500000);
					pthread_cond_broadcast(&FC_cond);
					
				}
				pthread_mutex_unlock(&EXP_mutex);
			}
			usleep(500000);
	}

			
	}
	


//fill containers employees
void FCtypeEmployee(struct emp_info* data){
	int line=data->line_id;
	int id=data->emp_id;
	char chocolate_type[4];
	strcpy(chocolate_type, data->chocolate_type);
	
	while(1){
	
		if(AStorage+BStorage+CStorage<storage_max_capacity){
	
		
		
		pthread_mutex_lock(&FC_mutex[id]);
		//printf(YELLOW"\t\t\n\nFC EMP %d WAITING FOR PATCHES OF ANY TYPE"RESET,id);
		//fflush(stdout);
		pthread_cond_wait(&FC_cond,&FC_mutex[id]);
		//printf(YELLOW"\t\t\n\nFC EMP %d CONDITION SATISFIED"RESET,id);
		//fflush(stdout);
		

		if(APatches>0){
			AContainer++;
			APatches--;
			printf(GREEN"\t\t\n\n + A cont %d "RESET,AContainer);
		}else if(BPatches>0){
			BContainer++;
			BPatches--;
			printf(GREEN"\t\t\n\n + B cont %d "RESET,BContainer);
		}else if(CPatches>0){
			CContainer++;
			CPatches--;
			printf(GREEN"\t\t\n\n + C cont %d "RESET,CContainer);
		}
		
		//printf(YELLOW"\t\t\n\nFC EMP %d UNLOCKED MUTEX"RESET,id);
		printf(YELLOW"\t\t\n\nA Container:%d, B Container:%d, C Container:%d"RESET,AContainer,BContainer,CContainer);
		fflush(stdout);
		
		//send signal to carton employees
		if(AContainer>1 || BContainer>1 || CContainer>1){
			pthread_cond_signal(&CARTON_cond);
		}
		sleep(1);
		pthread_mutex_unlock(&FC_mutex[id]);

		}else{
			pthread_mutex_lock(&FC_mutex[id]);
			while(AStorage+BStorage+CStorage >= storage_min_capacity){
				usleep(500000);
				pthread_cond_signal(&CARTON_cond);
			}
			pthread_mutex_unlock(&FC_mutex[id]);
		}
	
	}
	
}

//fill cartons employees
void CARTONtypeEmployee(struct emp_info* data){
	int line=data->line_id;
	int id=data->emp_id;
	char chocolate_type[4];
	strcpy(chocolate_type, data->chocolate_type);
	
	while(1){
		
		if(AStorage+BStorage+CStorage<storage_max_capacity){
	
			
			pthread_mutex_lock(&CARTON_mutex[id]);

			pthread_cond_wait(&CARTON_cond,&CARTON_mutex[id]);

			
			if(BContainer>1){
				BCarton++;
				BContainer-=2;
				//printf(RED"\t\t\n\nCARTONs B %d will take 1 min"RESET,BCarton);
				sleep(1);

			}else if(CContainer>1){
				CCarton++;
				CContainer-=2;
				//printf(RED"\t\t\n\nCARTONs C %d will take 1 min"RESET,CCarton);
				sleep(1);
			}else if(AContainer>1){
				ACarton++;
				AContainer-=2;
			//	printf(RED"\t\t\n\nCARTONs A %d will take 1 min"RESET,ACarton);
				sleep(1);
			}
			sleep(1);
			//printf(YELLOW"\t\t\n\nFC EMP %d UNLOCKED MUTEX"RESET,id);
			printf(YELLOW"\t\t\n\nA Cartons:%d, B Cartons:%d, C Cartons:%d"RESET,ACarton,BCarton,CCarton);
			fflush(stdout);
			
			if((ACarton>0 || BCarton>0 || CCarton>0) && AStorage+BStorage+CStorage < storage_max_capacity){
				pthread_cond_signal(&STORAGE_cond);
			}
			else if(ACarton==0 && BCarton==0 && CCarton==0){
				printf(CYAN"\t\t\n\nSTORAGE CURRENTLY EMPTY"RESET);
				fflush(stdout);
			}
			pthread_mutex_unlock(&CARTON_mutex[id]);
		}else{
			pthread_mutex_lock(&CARTON_mutex[id]);
			while(AStorage+BStorage+CStorage>=storage_min_capacity){
				usleep(500000);
				pthread_cond_signal(&STORAGE_cond);
			}
			pthread_mutex_unlock(&CARTON_mutex[id]);
		}
	
	}
	
}

void STORAGEtypeEmployee(struct emp_info* data){
	int line=data->line_id;
	int id=data->emp_id;
	char chocolate_type[4];
	strcpy(chocolate_type, data->chocolate_type);
	
	while(1){
	

		
		pthread_mutex_lock(&STORAGE_mutex[id]);
		//printf(YELLOW"\t\t\n\nFC EMP %d WAITING FOR PATCHES OF ANY TYPE"RESET,id);
		//fflush(stdout);
		pthread_cond_wait(&STORAGE_cond,&STORAGE_mutex[id]);
		//printf(YELLOW"\t\t\n\nFC EMP %d CONDITION SATISFIED"RESET,id);
		//fflush(stdout);
		
		
		if(CCarton>0 && AStorage+BStorage+CStorage<storage_max_capacity){
			CCarton--;
			CStorage++;
			pthread_cond_signal(&LOAD_cond);
			pthread_mutex_unlock(&STORAGE_mutex[id]);
		}else if(BCarton>0 && AStorage+BStorage+CStorage<storage_max_capacity){
			BCarton--;
			BStorage++;
			pthread_cond_signal(&LOAD_cond);
			pthread_mutex_unlock(&STORAGE_mutex[id]);
		}else if(ACarton>0 && AStorage+BStorage+CStorage<storage_max_capacity){
			ACarton--;
			AStorage++;
			pthread_cond_signal(&LOAD_cond);
			pthread_mutex_unlock(&STORAGE_mutex[id]);
		}else if(AStorage+BStorage+CStorage>=storage_max_capacity){
			//pthread_mutex_lock(&STORAGE_mutex[id]);
			while(AStorage+BStorage+CStorage>=storage_min_capacity){
				usleep(700000);
				pthread_cond_signal(&LOAD_cond);
			}
			sleep(storage_time);
			pthread_mutex_unlock(&STORAGE_mutex[id]);
		}else{
			sleep(storage_time);
			pthread_mutex_unlock(&STORAGE_mutex[id]);
		}
		
		//printf(YELLOW"\t\t\n\nFC EMP %d UNLOCKED MUTEX"RESET,id);
		printf(YELLOW"\t\t\n\nA Storage:%d, B Storage:%d, C Storage:%d"RESET,AStorage,BStorage,CStorage);
		fflush(stdout);
	
	}
	
}

void LOADtypeEmployee(struct emp_info* data){
	int line=data->line_id;
	int id=data->emp_id;
	char chocolate_type[4];
	strcpy(chocolate_type, data->chocolate_type);
	
	while(1){
	
		printf(YELLOW"\t\t\n\nLOAD EMPLOYEE %d READY"RESET,id);
		fflush(stdout);
		
		pthread_mutex_lock(&LOAD_mutex[id]);
		//printf(YELLOW"\t\t\n\nFC EMP %d WAITING FOR PATCHES OF ANY TYPE"RESET,id);
		//fflush(stdout);
		pthread_cond_wait(&LOAD_cond,&LOAD_mutex[id]);
		//printf(YELLOW"\t\t\n\nFC EMP %d CONDITION SATISFIED"RESET,id);
		//fflush(stdout);
		
		if(AStorage>0 && CurrentTruck[0]<A_truck_max){
		sleep(3);
			AStorage--;
			CurrentTruck[0]++;//[0] number of A chocolates, 1 B, 2 C
			printf(YELLOW"\t\t\n\nLOAD EMP %d MOVING TO TRUCK (WILL TAKE %d MINS)"RESET,id,1);
			fflush(stdout);
			
		}else if(BStorage>0 && CurrentTruck[1]<B_truck_max){
		sleep(3);
			printf(YELLOW"\t\t\n\nLOAD EMP %d MOVING TO TRUCK (WILL TAKE %d MINS)"RESET,id,1);
			fflush(stdout);
			BStorage--;
			CurrentTruck[1]++;
			//sleep(3);
		}else if(CStorage>0 && CurrentTruck[2]<C_truck_max){
		sleep(3);
			printf(YELLOW"\t\t\n\nLOAD EMP %d MOVING TO TRUCK (WILL TAKE %d MINS)"RESET,id,1);
			fflush(stdout);
			CStorage--;
			CurrentTruck[2]++;
			//sleep(3);
		}
		
		if(CurrentTruck[0]!=A_truck_max && CurrentTruck[1]==B_truck_max && CurrentTruck[2]==C_truck_max){
			MISSING='A';
		}else if(CurrentTruck[0]==A_truck_max && CurrentTruck[1]!=B_truck_max && CurrentTruck[2]==C_truck_max){
			MISSING='B';
		}else if(CurrentTruck[0]==A_truck_max && CurrentTruck[1]==B_truck_max && CurrentTruck[2]!=C_truck_max){
			MISSING='C';
		}
		
		if(CurrentTruck[0]==A_truck_max && CurrentTruck[1]==B_truck_max && CurrentTruck[2]==C_truck_max){
			printf("\t\t\n\nEMP SAYS TRUCK %d IS FULL NOW",TruckOrder);
			fflush(stdout);
			pthread_cond_signal(&TRUCK_cond);
		}
		printf("\t\t\n\nEMP SAYS TRUCK %d now has A:%d, B:%d, C:%d",TruckOrder,CurrentTruck[0],CurrentTruck[1],CurrentTruck[2]);
		fflush(stdout);
		pthread_mutex_unlock(&LOAD_mutex[id]);
		//printf(YELLOW"\t\t\n\nFC EMP %d UNLOCKED MUTEX"RESET,id);
	
	}
	
}

void TRUCKtypeEmployee(struct emp_info* data){
	int line=data->line_id;
	int id=data->emp_id;
	char chocolate_type[4];
	strcpy(chocolate_type, data->chocolate_type);
	
	while(1){
		
		//usleep(300);
		if(id==0 && id==TruckOrder){
		
		pthread_mutex_lock(&TRUCK_mutex[id]);
		printf("\t\t\n\nTRUCK %d READY",id);
		fflush(stdout);
		//printf(YELLOW"\t\t\n\nFC EMP %d WAITING FOR PATCHES OF ANY TYPE"RESET,id);
		//fflush(stdout);
		pthread_cond_wait(&TRUCK_cond,&TRUCK_mutex[id]);
		//printf(YELLOW"\t\t\n\nFC EMP %d CONDITION SATISFIED"RESET,id);
		//fflush(stdout);
		
		printf("\t\t\n\n\n\n\nTRUCK %d HAS DISPATCHED (WILL TAKE %d MINS)\n\n\n",id,truck_time);
		fflush(stdout);
		
		AShipped+=A_truck_max;
		BShipped+=B_truck_max;
		CShipped+=C_truck_max;
		
		TruckOrder++;
		if(TruckOrder>=3)
			TruckOrder=0;
		
		if(AShipped>=A_max_shipped && BShipped>=B_max_shipped && CShipped>=C_max_shipped){
			printf(MAGENTA"\n\n\t\t SIMULATION GOAL REACHED!\nTOTAL A BOXES SHIPPED:%d, B BOXES SHIPPED:%d, C BOXES SHIPPED:%d"RESET,AShipped,BShipped,CShipped);
			terminate=1;
		}
		
		//printf(YELLOW"\t\t\n\nFC EMP %d UNLOCKED MUTEX"RESET,id);
		printf("\t\t\n\nA INSIDE TRUCK:%d, B INSIDE TRUCK:%d, C INSIDE TRUCK:%d",CurrentTruck[0],CurrentTruck[1],CurrentTruck[2]);
		fflush(stdout);
		
		CurrentTruck[0]=0;
		CurrentTruck[1]=0;
		CurrentTruck[2]=0;
		
		sleep(truck_time);
		
		pthread_mutex_unlock(&TRUCK_mutex[id]);
		
		}
		else if(id==1 && id==TruckOrder){
		
		pthread_mutex_lock(&TRUCK_mutex[id]);
		printf("\t\t\n\nTRUCK %d READY",id);
		fflush(stdout);
		//printf(YELLOW"\t\t\n\nFC EMP %d WAITING FOR PATCHES OF ANY TYPE"RESET,id);
		//fflush(stdout);
		pthread_cond_wait(&TRUCK_cond,&TRUCK_mutex[id]);
		//printf(YELLOW"\t\t\n\nFC EMP %d CONDITION SATISFIED"RESET,id);
		//fflush(stdout);
		
		printf("\t\t\n\nTRUCK %d HAS DISPATCHED (WILL TAKE %d MINS)",id,truck_time);
		fflush(stdout);
		
		AShipped+=A_truck_max;
		BShipped+=B_truck_max;
		CShipped+=C_truck_max;
		
		TruckOrder++;
		if(TruckOrder>=3)
			TruckOrder=0;
		
		if(AShipped>=A_max_shipped && BShipped>=B_max_shipped && CShipped>=C_max_shipped){
			printf(MAGENTA"\n\n\t\t SIMULATION GOAL REACHED!\nTOTAL A BOXES SHIPPED:%d, B BOXES SHIPPED:%d, C BOXES SHIPPED:%d"RESET,AShipped,BShipped,CShipped);
			terminate=1;
		}
		
		//printf(YELLOW"\t\t\n\nFC EMP %d UNLOCKED MUTEX"RESET,id);
		printf("\t\t\n\nA INSIDE TRUCK:%d, B INSIDE TRUCK:%d, C INSIDE TRUCK:%d",CurrentTruck[0],CurrentTruck[1],CurrentTruck[2]);
		fflush(stdout);
		
		CurrentTruck[0]=0;
		CurrentTruck[1]=0;
		CurrentTruck[2]=0;
		
		sleep(truck_time);
		pthread_mutex_unlock(&TRUCK_mutex[id]);
		
		}else if(id==2 && id==TruckOrder){
		
		pthread_mutex_lock(&TRUCK_mutex[id]);
		printf("\t\t\n\nTRUCK %d READY",id);
		fflush(stdout);
		//printf(YELLOW"\t\t\n\nFC EMP %d WAITING FOR PATCHES OF ANY TYPE"RESET,id);
		//fflush(stdout);
		pthread_cond_wait(&TRUCK_cond,&TRUCK_mutex[id]);
		//printf(YELLOW"\t\t\n\nFC EMP %d CONDITION SATISFIED"RESET,id);
		//fflush(stdout);
		
		printf(YELLOW"\t\t\n\nTRUCK %d HAS DISPATCHED (WILL TAKE %d MINS)"RESET,id,truck_time);
		fflush(stdout);
		
		AShipped+=A_truck_max;
		BShipped+=B_truck_max;
		CShipped+=C_truck_max;
		
		TruckOrder++;
		if(TruckOrder>=3)
			TruckOrder=0;
		
		if(AShipped>=A_max_shipped && BShipped>=B_max_shipped && CShipped>=C_max_shipped){
			printf(MAGENTA"\n\n\t\tSIMULATION GOAL REACHED!\nTOTAL BOXES SHIPPED:A %d, B BOXES:%d, C BOXES:%d"RESET,AShipped,BShipped,CShipped);
			terminate=1;
		}
		
		//printf(YELLOW"\t\t\n\nFC EMP %d UNLOCKED MUTEX"RESET,id);
		printf("\t\t\n\nA INSIDE TRUCK:%d, B INSIDE TRUCK:%d, C INSIDE TRUCK:%d",CurrentTruck[0],CurrentTruck[1],CurrentTruck[2]);
		fflush(stdout);
		
		CurrentTruck[0]=0;
		CurrentTruck[1]=0;
		CurrentTruck[2]=0;
		
		sleep(truck_time);
		pthread_mutex_unlock(&TRUCK_mutex[id]);
		
		}
	
	}
	
}

void time_exceeded(){
	printf(MAGENTA"\n\t\tTIME FOR SIMULATION EXCEEDED LIMIT (%dmin)"RESET,exec_time);
	fflush(stdout);
	terminate=1;
}








//_____===============================================================OPENGL=========================================================================


void setupScene(int clearColor[]) {
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    //glClearColor(250, 250, 250, 1.0);  //  Set the cleared screen colour to black.
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);  // This sets up the viewport so that the coordinates (0, 0) are at the top left of the window.
    
    // Set up the orthographic projection so that coordinates (0, 0) are in the top left.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -10, 10);
    
    // Back to the modelview so we can draw stuff.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the screen and depth buffer.
}

    
void initVariables(){
   // glPointSize(20);
}

void drawRaceGround(){
	//___________________________________________7 queues

	for(int i=0;i<3;i++){
    glBegin(GL_QUADS);
    glColor3ub(255, 0, 0);
    glVertex2d(100, 200+100*i);
    glVertex2d(100, 270+100*i);
    glVertex2d(400, 270+100*i);
    glVertex2d(400, 200+100*i);
    glEnd();
    }
     
    	for(int i=0;i<2;i++){
    glBegin(GL_QUADS);
    glColor3ub(0, 255, 0);
    glVertex2d(100, 530+100*i);
    glVertex2d(100, 600+100*i);
    glVertex2d(400, 600+100*i);
    glVertex2d(400, 530+100*i);
    glEnd();
    }
    
    	for(int i=0;i<2;i++){
    glBegin(GL_QUADS);
    glColor3ub(0, 0, 255);
    glVertex2d(100, 760+100*i);
    glVertex2d(100, 830+100*i);
    glVertex2d(400, 830+100*i);
    glVertex2d(400, 760+100*i);
    glEnd();
    }
    
    
    //_________________________________________________________ 3boxes
    
    glBegin(GL_QUADS);
    glColor3ub(180, 20, 180);
    glVertex2d(440, 270);
    glVertex2d(440, 400);
    glVertex2d(500, 400);
    glVertex2d(500, 270);
    glEnd();
    
        glBegin(GL_QUADS);
    glColor3ub(180, 220, 80);
    glVertex2d(440, 550);
    glVertex2d(440, 680);
    glVertex2d(500, 680);
    glVertex2d(500, 550);
    glEnd();
    
        glBegin(GL_QUADS);
    glColor3ub(80, 220, 180);
    glVertex2d(440, 780);
    glVertex2d(440, 910);
    glVertex2d(500, 910);
    glVertex2d(500, 780);
    glEnd();
    
    //_________________________________________________________ 8th queue
    
    glBegin(GL_QUADS);
    glColor3ub(20, 20, 110);
    glVertex2d(550, 500);
    glVertex2d(550, 600);
    glVertex2d(850, 600);
    glVertex2d(850, 500);
    glEnd();
    
    //______________________________________________________Huge containers
    
        glBegin(GL_QUADS);
    glColor3ub(180, 20, 180);
    glVertex2d(900, 270);
    glVertex2d(900, 400);
    glVertex2d(1050, 400);
    glVertex2d(1050, 270);
    glEnd();
    
        glBegin(GL_QUADS);
    glColor3ub(180, 220, 80);
    glVertex2d(900, 550);
    glVertex2d(900, 680);
    glVertex2d(1050, 680);
    glVertex2d(1050, 550);
    glEnd();
    
        glBegin(GL_QUADS);
    glColor3ub(80, 220, 180);
    glVertex2d(900, 780);
    glVertex2d(900, 910);
    glVertex2d(1050, 910);
    glVertex2d(1050, 780);
    glEnd();
    
    //_______________________________________________________ Cartons
    
            glBegin(GL_QUADS);
    glColor3ub(180, 20, 180);
    glVertex2d(1090, 270);
    glVertex2d(1090, 400);
    glVertex2d(1170, 400);
    glVertex2d(1170, 270);
    glEnd();
    
        glBegin(GL_QUADS);
    glColor3ub(180, 220, 80);
    glVertex2d(1090, 550);
    glVertex2d(1090, 680);
    glVertex2d(1170, 680);
    glVertex2d(1170, 550);
    glEnd();
    
        glBegin(GL_QUADS);
    glColor3ub(80, 220, 180);
    glVertex2d(1090, 780);
    glVertex2d(1090, 910);
    glVertex2d(1170, 910);
    glVertex2d(1170, 780);
    glEnd();
    
    //___________________________________________________Storage
    
           glBegin(GL_QUADS);
    glColor3ub(150, 150, 150);
    glVertex2d(1220, 550);
    glVertex2d(1220, 910);
    glVertex2d(1620, 910);
    glVertex2d(1620, 550);
    glEnd();
    
    //___________________________________________Trucks
    
                glBegin(GL_QUADS);
    glColor3ub(200, 20, 110);
    glVertex2d(1250, 200);
    glVertex2d(1250, 500);
    glVertex2d(1360, 500);
    glVertex2d(1360, 200);
    glEnd();
    
        glBegin(GL_QUADS);
    glColor3ub(200, 20, 110);
    glVertex2d(1380, 200);
    glVertex2d(1380, 500);
    glVertex2d(1490, 500);
    glVertex2d(1490, 200);
    glEnd();
    
        glBegin(GL_QUADS);
    glColor3ub(200, 20, 110);
    glVertex2d(1510, 200);
    glVertex2d(1510, 500);
    glVertex2d(1620, 500);
    glVertex2d(1620, 200);
    glEnd();
    
    
    
    
    
    
    
    int x_axis;
    int y_axis;
    srand(time(0));
    
    
    //citizen.arrival_hour = shmptr_temp[HR_INDEX];
	//citizen.arrival_minute = shmptr_temp[MN_INDEX];
    
    //Random points in Gate area
	//C line 0
	glPointSize(20);
    for(int i=0; i<Anum_chocs_at_stage[0][0]; i++){
    x_axis = rand()%35 + 110; 
    y_axis = rand()%50 + 210; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }
	    for(int i=0; i<Anum_chocs_at_stage[0][1]; i++){
    x_axis = rand()%35 + 145; 
    y_axis = rand()%50 + 210; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }
	    for(int i=0; i<Anum_chocs_at_stage[0][2]; i++){
    x_axis = rand()%35 + 180; 
    y_axis = rand()%50 + 210; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[0][3]; i++){
    x_axis = rand()%35 + 215; 
    y_axis = rand()%50 + 210; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[0][4]; i++){
    x_axis = rand()%35 + 250; 
    y_axis = rand()%50 + 210; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[0][5]; i++){
    x_axis = rand()%35 + 285; 
    y_axis = rand()%50 + 210; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[0][6]; i++){
    x_axis = rand()%35 + 320; 
    y_axis = rand()%50 + 210; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[0][7]; i++){
    x_axis = rand()%35 + 355; 
    y_axis = rand()%50 + 210; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }
	//___________________________------------------------------------------
		//C line 1
    for(int i=0; i<Anum_chocs_at_stage[1][0]; i++){
    x_axis = rand()%35 + 110; 
    y_axis = rand()%50 + 310; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[1][1]; i++){
    x_axis = rand()%35 + 145; 
    y_axis = rand()%50 + 310; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[1][2]; i++){
    x_axis = rand()%35 + 180; 
    y_axis = rand()%50 + 310; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[1][3]; i++){
    x_axis = rand()%35 + 215; 
    y_axis = rand()%50 + 310; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[1][4]; i++){
    x_axis = rand()%35 + 250; 
    y_axis = rand()%50 + 310; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[1][5]; i++){
    x_axis = rand()%35 + 285; 
    y_axis = rand()%50 + 310; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[1][6]; i++){
    x_axis = rand()%35 + 320; 
    y_axis = rand()%50 + 310; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[1][7]; i++){
    x_axis = rand()%35 + 355; 
    y_axis = rand()%50 + 310; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }
	//--------------------------------------------------------------
// A line 2
	    for(int i=0; i<Anum_chocs_at_stage[2][0]; i++){
    x_axis = rand()%35 + 110; 
    y_axis = rand()%50 + 410; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[2][1]; i++){
    x_axis = rand()%35 + 145; 
    y_axis = rand()%50 + 410; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[2][2]; i++){
    x_axis = rand()%35 + 180; 
    y_axis = rand()%50 + 410; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[2][3]; i++){
    x_axis = rand()%35 + 215; 
    y_axis = rand()%50 + 410; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[2][4]; i++){
    x_axis = rand()%35 + 250; 
    y_axis = rand()%50 + 410; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[2][5]; i++){
    x_axis = rand()%35 + 285; 
    y_axis = rand()%50 + 410; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[2][6]; i++){
    x_axis = rand()%35 + 320; 
    y_axis = rand()%50 + 410; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	    for(int i=0; i<Anum_chocs_at_stage[2][7]; i++){
    x_axis = rand()%35 + 355; 
    y_axis = rand()%50 + 410; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }
	//--------------------------------------------------------------
    
    // B line 0
	for(int i=0; i<Bnum_chocs_at_stage[0][0]; i++){
    x_axis = rand()%35 + 110; 
    y_axis = rand()%50 + 540; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<Bnum_chocs_at_stage[0][1]; i++){
    x_axis = rand()%35 + 155; 
    y_axis = rand()%50 + 540; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<Bnum_chocs_at_stage[0][2]; i++){
    x_axis = rand()%35 + 200; 
    y_axis = rand()%50 + 540; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<Bnum_chocs_at_stage[0][3]; i++){
    x_axis = rand()%35 + 245; 
    y_axis = rand()%50 + 540; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<Bnum_chocs_at_stage[0][4]; i++){
    x_axis = rand()%35 + 290; 
    y_axis = rand()%50 + 540; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<Bnum_chocs_at_stage[0][5]; i++){
    x_axis = rand()%35 + 335; 
    y_axis = rand()%50 + 540; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }
	//-----------------------------------------------------------



    // B line 0
	for(int i=0; i<Bnum_chocs_at_stage[0][0]; i++){
    x_axis = rand()%35 + 110; 
    y_axis = rand()%50 + 540; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<Bnum_chocs_at_stage[1][1]; i++){
    x_axis = rand()%35 + 155; 
    y_axis = rand()%50 + 640; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<Bnum_chocs_at_stage[1][2]; i++){
    x_axis = rand()%35 + 200; 
    y_axis = rand()%50 + 640; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<Bnum_chocs_at_stage[1][3]; i++){
    x_axis = rand()%35 + 245; 
    y_axis = rand()%50 + 640; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<Bnum_chocs_at_stage[1][4]; i++){
    x_axis = rand()%35 + 290; 
    y_axis = rand()%50 + 640; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<Bnum_chocs_at_stage[1][5]; i++){
    x_axis = rand()%35 + 335; 
    y_axis = rand()%50 + 640; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }
	//-----------------------------------------------------------

    // C line 0
	for(int i=0; i<Cnum_chocs_at_stage[0][0]; i++){
    x_axis = rand()%35 + 110; 
    y_axis = rand()%50 + 770; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<Cnum_chocs_at_stage[0][1]; i++){
    x_axis = rand()%35 + 165; 
    y_axis = rand()%50 + 770; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<Cnum_chocs_at_stage[0][2]; i++){
    x_axis = rand()%35 + 220; 
    y_axis = rand()%50 + 770; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<Cnum_chocs_at_stage[0][3]; i++){
    x_axis = rand()%35 + 275; 
    y_axis = rand()%50 + 770; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<Cnum_chocs_at_stage[0][4]; i++){
    x_axis = rand()%35 + 330; 
    y_axis = rand()%50 + 770; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }
//------------------------------------------------------------
    

    // C line 1
	for(int i=0; i<Cnum_chocs_at_stage[1][0]; i++){
    x_axis = rand()%35 + 110; 
    y_axis = rand()%50 + 870; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<Cnum_chocs_at_stage[1][1]; i++){
    x_axis = rand()%35 + 165; 
    y_axis = rand()%50 + 870; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<Cnum_chocs_at_stage[1][2]; i++){
    x_axis = rand()%35 + 220; 
    y_axis = rand()%50 + 870; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<Cnum_chocs_at_stage[1][3]; i++){
    x_axis = rand()%35 + 275; 
    y_axis = rand()%50 + 870; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<Cnum_chocs_at_stage[1][4]; i++){
    x_axis = rand()%35 + 330; 
    y_axis = rand()%50 + 870; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }
//------------------------------------------------------------

//Basket A
	for(int i=0; i<AChoco; i++){
    x_axis = rand()%40 + 450; 
    y_axis = rand()%110 + 280; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	//Basket B
	for(int i=0; i<BChoco; i++){
    x_axis = rand()%40 + 450; 
    y_axis = rand()%110 + 560; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	//Basket C
	for(int i=0; i<CChoco; i++){
    x_axis = rand()%40 + 450; 
    y_axis = rand()%110 + 790; 
    glBegin(GL_POINTS);
    glColor3ub(210, 105, 30);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

//------------------------------------------------------------

//EXP Queue

	if(patch_in_EXP != 'N'){
		x_axis = 700;
		y_axis = 550;
		glPointSize(50);
		glBegin(GL_POINTS);
		if(patch_in_EXP =='A')glColor3ub(255, 0, 0);
		else if(patch_in_EXP == 'B')glColor3ub(0, 255, 0);
		else glColor3ub(0, 0, 255);
		glVertex2d(x_axis, y_axis);
		glEnd();
		
	}


//Container -------------------------------------
	for(int i=0; i<AContainer; i++){
    x_axis = rand()%130 + 910; 
    y_axis = rand()%110 + 280; 
    glBegin(GL_POINTS);
    glColor3ub(255, 0, 0);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<BContainer; i++){
    x_axis = rand()%130 + 910; 
    y_axis = rand()%110 + 560; 
    glBegin(GL_POINTS);
    glColor3ub(0, 255, 0);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<BContainer; i++){
    x_axis = rand()%130 + 910; 
    y_axis = rand()%110 + 790; 
    glBegin(GL_POINTS);
    glColor3ub(0, 0, 255);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }


//Cartons -------------------------------------
	for(int i=0; i<ACarton; i++){
    x_axis = rand()%60 + 1100; 
    y_axis = rand()%110 + 280; 
    glBegin(GL_POINTS);
    glColor3ub(255, 0, 0);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<BCarton; i++){
    x_axis = rand()%60 + 1100; 
    y_axis = rand()%110 + 560; 
    glBegin(GL_POINTS);
    glColor3ub(0, 255, 0);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<CCarton; i++){
    x_axis = rand()%60 + 1100; 
    y_axis = rand()%110 + 790; 
    glBegin(GL_POINTS);
    glColor3ub(0, 0, 255);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }


	//Storage -------------------------------------
	for(int i=0; i<AStorage; i++){
    x_axis = rand()%380 + 1230; 
    y_axis = rand()%340 + 560; 
    glBegin(GL_POINTS);
    glColor3ub(255, 0, 0);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<BStorage; i++){
    x_axis = rand()%380 + 1230; 
    y_axis = rand()%340 + 560; 
    glBegin(GL_POINTS);
    glColor3ub(0, 255, 0);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<CStorage; i++){
    x_axis = rand()%380 + 1230; 
    y_axis = rand()%340 + 560; 
    glBegin(GL_POINTS);
    glColor3ub(0, 0, 255);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }


		//Storage -------------------------------------
	for(int i=0; i<CurrentTruck[0]; i++){
    x_axis = rand()%90 + 1260 + TruckOrder*130; 
    y_axis = rand()%280 + 190; 
    glBegin(GL_POINTS);
    glColor3ub(255, 0, 0);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

	for(int i=0; i<CurrentTruck[1]; i++){
    x_axis = rand()%90 + 1260 + TruckOrder*130; 
    y_axis = rand()%280 + 190; 
    glBegin(GL_POINTS);
    glColor3ub(0, 255, 0);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

		for(int i=0; i<CurrentTruck[3]; i++){
    x_axis = rand()%90 + 1260 + TruckOrder*130; 
    y_axis = rand()%280 + 190; 
    glBegin(GL_POINTS);
    glColor3ub(0, 0, 255);
    glVertex2d(x_axis, y_axis);
    glEnd();
    }

}

void drawScene() {
    setupScene((int[]){250, 250, 250, 1});    
    drawRaceGround();  
    glutSwapBuffers();  // Send the scene to the screen.
    glFlush();
}

void update(int value) {
	
    glutPostRedisplay();  // Tell GLUT that the display has changed.
    glutTimerFunc(updateTime, update, 0);  // Tell GLUT to call update again in 25 milliseconds.
}



void opengl(){
	//glutInit(&argc, argv);
	int c =1;
	glutInit(&c, NULL);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB); 
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("proj3");
	initVariables();
	//initRendering();
	glutDisplayFunc(drawScene);
	//glutReshapeFunc(resizeWindow);
	//glutKeyboardFunc(myKeyboardFunc);
	//glutMouseFunc(myMouseFunc);
	glutTimerFunc(updateTime, update, 0);
	glutMainLoop();
	//close(myfifo);

}
