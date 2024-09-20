#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define wordLimit 20 //char limit of all words
#define definitionLimit 100 //char limit of all definitions
#define numWords 60 //total num of words in file

#define alarmTime 60

#define numHW 3 //total number of horizontal words
#define numVW 3 //total number of vertical words
int totalWords = numHW + numVW; 

int blockW[6] = {0}; //index of words bloqued by the user
int correctWords = 0;

int currentH_I = 0; //index used to move across the 3 saved horizontal words
int currentV_I = 0; //index used to move across the 3 saved vertical words

char matrix[numHW*10][numVW*10]; //Matrix

sem_t semaphore_id;

int usedWords[numWords]; //array used = 1, !used = 0;

char words[numWords][wordLimit]; //matrix with all words from file
char definitions[numWords][definitionLimit]; //matrix with all definitions from file

int savedWords_IH[numHW]; //array with random word indexes
int savedWords_IV[numVW]; //array with random word indexes

int intersectionsH[15] = {0}; //array of the intersections found
int intersectionsV[15] = {0}; //array of the intersections found

int intersectionsSetH[5] = {0};
int intersectionsSetV[5] = {0};

int intersectionsMatrixSetH[6] = {0};
int intersectionsMatrixSetV[6] = {0};

int intersectionsMatrixSetH_H[3] = {0};
int intersectionsMatrixSetV_H[3] = {0};

int intersectionsMatrixSetH_V[3] = {0};
int intersectionsMatrixSetV_V[3] = {0};

int savedWords[6];

//prototipes of functions
void saveWords(); //saves words from file to local
void saveDefinitions(); //saves definitions from file to local
void* selectHorizontalWords(); //thread handler to selesct random horizontal words
void* selectVerticalWords(); //thread handler to selesct random vertical words
void initializeWords(); 
void inicializeIntersections();
void findIntersections(int string1, int string2, int index1, int index2);
void setIntersections();
void wordChanger();
void initializeMatrix();
void printMatrix();
void deff();

void user();
void handler_alarm() 
{
/*
    int flag = 0;
	do{
        printf("alarma recibida, cambiando una palabra");
		int randNum = rand() % numWords+1; //is going to select a random word in the wordlist
		int randWord = rand() % totalWords; //is going to select 1 random word to change in the cross word

		if(blockW[randWord] != 1){ //if the word is not blocked (user is using it)
			randNum = rand() % numWords+1;
			if(usedWords[randNum] == 0){//The index word is not used
				savedWords[randWord] = randNum; //saves the random word to the saved words
				usedWords[randNum] = 1;
				flag = 1;
				(printf("\npalabra encontrada y cambiada\n"));
			}
		}
	}while(flag!=1);

	initializeMatrix();
	printf("\nPalabra Cambiada\n");
 	alarm(alarmTime);
*/
}
int main(){
	srand(time(NULL));
	
	saveWords();
	saveDefinitions();
	initializeWords();
	inicializeIntersections();
	
	user();	
	
	return 0;
}

void saveWords(){
	int cont = 0;
	FILE *fd = fopen ("palabras_casa_de_hojas", "r");
	
	while(fscanf(fd, "%s", words[cont]) != EOF){
		cont++;
	}
	fclose(fd);
}

void saveDefinitions(){
	int cont = 0;
	FILE *fd = fopen ("descripciones_casa_de_hojas", "r");
	
	char line[definitionLimit];
	
	while(fgets(line, definitionLimit, fd) != NULL){
		line[strcspn(line, "\n")] = '\0';
		strcpy(definitions[cont], line);
		cont++;
	}
	fclose(fd);
}

void* selectHorizontalWords(){
	sem_wait(&semaphore_id);
	int randNum;
	int flag = 0;
	do{
		randNum = rand() % numWords+1;
		if(usedWords[randNum] == 0){//The index word is not used
			savedWords_IH[currentH_I] = randNum;
			currentH_I ++;
			usedWords[randNum] = 1;
			flag = 1; 
		}
	}while(flag!=1);
	sem_post(&semaphore_id);
	pthread_exit(NULL);
}

void* selectVerticalWords(){

	sem_wait(&semaphore_id);
	int randNum;
	int flag = 0;
	do{
		randNum = rand() % numWords+1;
		if(usedWords[randNum] == 0){//The index word is not used
			savedWords_IV[currentV_I] = randNum;
			currentV_I ++;
			usedWords[randNum] = 1;
			flag = 1; 
		}
	}while(flag!=1);
	sem_post(&semaphore_id);
	pthread_exit(NULL);
}

void initializeWords(){
	currentH_I = 0;
	currentV_I = 0; 
	
	pthread_t threads_horizontal[numHW];
	pthread_t threads_vertical[numVW];
    
    sem_init(&semaphore_id, 0, 1);
    for( int i=0; i<numHW; i++)// horizontal threads
        pthread_create( &threads_horizontal[i], NULL, selectHorizontalWords, NULL );
    // Wait for the threads to finish.
    for(int i=0; i<numHW; i++)
        pthread_join(threads_horizontal[i], NULL);
    
	for( int i=0; i<numVW; i++)// vertical threads
        pthread_create( &threads_vertical[i], NULL, selectVerticalWords, NULL);
	for(int i=0; i<numVW; i++)
        pthread_join(threads_vertical[i], NULL);
    
	sem_destroy(&semaphore_id);// Destroy the semaphore.
	
}

int counterH, counterV = 0; //from 0 to 5 to fill the array of intersections
int totalIntersections = 0;

void findIntersections(int string1, int string2, int index1, int index2){
	int flag=0; //if flag=0, didn't find any intersection
	int counter=0;
	totalIntersections = 0;
	for(int i=0; i<(strlen(words[string1])) ; i++){
		for(int j=0; j<(strlen (words[string2])) ; j++){
			if(words[string1][i] == words[string2][j]){
				flag=1;
				intersectionsH[counter] = i;
				intersectionsV[counter] = j;
				counter ++;
				totalIntersections ++;
			}
		}
	}
	if(flag == 0){ //didn't found any intersections
		if(strlen(words[string1]) < strlen(words[string2])){
			currentH_I = index1;
			pthread_t threads_horizontal;
			pthread_create( &threads_horizontal, NULL, selectHorizontalWords, NULL );
			pthread_join(threads_horizontal, NULL);
			findIntersections(savedWords_IH[currentH_I], savedWords_IV[currentV_I], currentH_I, currentV_I );
			setIntersections(currentH_I);
			
		}else{
			currentV_I = index2;
			pthread_t threads_vertical;
			pthread_create( &threads_vertical, NULL, selectVerticalWords, NULL);
		    pthread_join(threads_vertical, NULL);
			findIntersections(savedWords_IH[currentH_I], savedWords_IV[currentV_I], currentH_I, currentV_I );
			setIntersections(currentV_I);
        }
	}
}

int counterH2, counterV2 = 0; //counter for functions inicialize intersections
void setIntersections(int index){ //selects randomly one of all the intersections
	int interIndex = rand() % totalIntersections;
	//int usedRand[totalIntersections] = {0};
	
	//if (intersectionsH[interIndex] == intersectionsH[interIndex+1] || intersectionsV[interIndex] == intersectionsV[interIndex+1]){
	//	setIntersections(index);
	//}else{
		intersectionsSetH[index] = intersectionsH[interIndex];
		intersectionsSetV[index] = intersectionsV[interIndex];
	//}
}

void inicializeIntersections(){
	currentH_I = 0;
	currentV_I = 0;
	
	for(int i=0; i<5 ; i++){
		findIntersections(savedWords_IH[currentH_I], savedWords_IV[currentV_I], currentH_I, currentV_I );
		setIntersections(i);
		if(i % 2 == 0)
			currentH_I++;
		else
			currentV_I++;
	}
}

void initializeMatrix(){
	int indexJ = 15; //moves between columns horizontal
	int indexI = 15; //moves between lines   vertical
	int wordNumberH = 1;
	int wordNumberV = 4;
	
	int counterH3, counterV3 = 0; //inicialice again the counters
	
	for(int i=0; i<3;i++){
		savedWords[i] = savedWords_IH[i] ;
		savedWords[i+3] = savedWords_IV[i];
	}
	
	for(int k=0; k<numHW*10 ; k++){
		for(int l=0; l<numVW*10 ; l++){
			matrix[k][l]= ' ';			
		}
	}
	int indexCounter = 0;
	
	for(int i=0 ; i<6; i++){
		
		//horizontal
		if(i % 2 == 0){
			for(int j=0; j<(strlen (words[savedWords_IH[currentH_I]])) ; j++){
				matrix[indexI][indexJ+j] = wordNumberH + '0';
			}
			intersectionsMatrixSetH_H[counterH] = indexJ;
			intersectionsMatrixSetV_H[counterH] = indexI;
			counterH++;
			indexJ += intersectionsSetH[i];
			indexI -= intersectionsSetV[i];
			
			currentH_I++;
			wordNumberH++;
		}else{
			for(int j=0; j<(strlen (words[savedWords_IV[currentV_I]])) ; j++){
				matrix[indexI+j][indexJ] = wordNumberV + '0';
			}
			intersectionsMatrixSetH_V[counterV] = indexJ;
			intersectionsMatrixSetV_V[counterV] = indexI;
			indexJ -= intersectionsSetH[i];
			indexI += intersectionsSetV[i];
			
			counterV++;
			currentV_I++;
			wordNumberV++;
		}
	}
}

void printMatrix(){
	//printf title and instructions
	printf("\t██████████████████████████████████████\n\t┃  ╱╱╱╱╱╭╮╱╱╱╱╱╱╭━━━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱┃┃╱╱╱╱╱╱┃╭━╮┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱┃┃╱╱╭━━╮┃┃╱╰╋━━┳━━┳━━╮╱╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱┃┃╱╭┫╭╮┃┃┃╱╭┫╭╮┃━━┫╭╮┃╱╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱┃╰━╯┃╭╮┃┃╰━╯┃╭╮┣━━┃╭╮┃╱╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱╰━━━┻╯╰╯╰━━━┻╯╰┻━━┻╯╰╯╱╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱╭━━━╮╱╱╱╭╮╱╭╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱╰╮╭╮┃╱╱╱┃┃╱┃┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱╱┃┃┃┣━━╮┃╰━╯┣━━┳┳━━┳━━╮╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱╱┃┃┃┃┃━┫┃╭━╮┃╭╮┣┫╭╮┃━━┫╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱╭╯╰╯┃┃━┫┃┃╱┃┃╰╯┃┃╭╮┣━━┃╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱╰━━━┻━━╯╰╯╱╰┻━━┫┣╯╰┻━━╯╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯┃╱╱╱╱╱╱╱╱╱╱  ┃\n\t┃  ╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱  ┃\n\t██████████████████████████████████████\n");
	//print the matrix

	for(int j=0; j<numVW*10 ; j++){
		printf("██");			
	}
	for(int i=0; i<numHW*10 ; i++){
		printf("┃");
		for(int j=0; j<numVW*10 ; j++){
			printf("%c ", matrix[i][j]);			
		}
		printf("┃");
		printf("\n");
	}
	for(int j=0; j<numVW*10 ; j++){
		printf("██");			
	}
	int counter = 0;
	printf("\n\t\t  faltan %d palabras\n", totalWords - correctWords);//prueba
	printf("\n\t\t\tHORIZONTAL\n");
	for(int i=0 ; i<6 ; i++){
		printf("%d %s\n", i+1, definitions[savedWords[i]]);
		counter ++;
		if(counter == 3)
		printf("\n\t\t\tVERTICAL\n");
	}
}

void user(){
	int selection = 0;
	char selectionWord[wordLimit];

	alarm(alarmTime);

	signal(SIGALRM,handler_alarm);
	int flag = 0;
	int buffer=0;;
	initializeMatrix();

	for(int i = 0; i<6; i++){
		printf("i: %d of saved words %s\n", i , words[savedWords[i]]);
	}

	do{
		printMatrix();
		printf("Qué palabra desea adivinar: ");
		scanf("%d", &selection);

		if(selection <= totalWords && selection >0){
			selection--;
			blockW[selection] = 1;
			printf("Ingrese su respuesta: ");
			scanf("%s", selectionWord);
			if(strcmp(selectionWord, words[savedWords[selection]]) == 0){ //if the word is blocked means its correct
				printf("\n\n adivinaste %s!, te faltan %d palabras\n", selectionWord, totalWords - correctWords);//prueba
				int indexJ;
				int indexI;

				if(selection <= 3 ){
					indexJ = intersectionsMatrixSetH_H[selection];
					indexI = intersectionsMatrixSetV_H[selection];
					for(int j=0; j<(strlen (words[savedWords[selection]])) ; j++)
						matrix[indexI][indexJ+j] = words[savedWords[selection]][j];
				}else{					
					indexJ = intersectionsMatrixSetH_V[selection/3];
					indexI = intersectionsMatrixSetV_V[selection/3];
					for(int j=0; j<(strlen (words[savedWords[selection]])) ; j++)
						matrix[indexI+j][indexJ] = words[savedWords[selection]][j];
				}					
				blockW[selection] = 1;
				correctWords++;
			}
			printf("\nIncorrecto!\n");
			blockW[selection] = 0; //liberates the block using word.
		}else{
			printf("número ingresado no es válido\n");
		}
	}while(correctWords != 6);
	printMatrix();
	printf("╔═══╗───────────╔╗───╔╦╦╗\n║╔═╗║──────────╔╝╚╗──║║║║\n║║─╚╬══╦═╗╔══╦═╩╗╔╬══╣║║║\n║║╔═╣╔╗║╔╗╣╔╗║══╣║║║═╬╩╩╝\n║╚╩═║╔╗║║║║╔╗╠══║╚╣║═╬╦╦╗\n╚═══╩╝╚╩╝╚╩╝╚╩══╩═╩══╩╩╩╝\n");
}