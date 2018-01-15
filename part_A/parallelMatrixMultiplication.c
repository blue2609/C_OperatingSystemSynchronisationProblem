#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

//declare 3 global matrices
float ** A, ** B, ** C, ** D;

//declare 3 global variables to specify matrix dimensions
int m, k, n;

//declare a global variable to contain number of threads
int num_thrds;

/* THREAD FUNCTION 
--- multiply each row of matrix A with 
--- each column of matrix B and sum 
--- the products of multiplications
--- to get an element of matrix C
*/
void * multiThreadedMatrixMultiplication(void * user_defined_id){

	int startIndex;
	int endIndex;

	//convert the user defined id into task id
	int task_id = *((int *)user_defined_id);

	//calculate how many remainder columns to be processed
	int remainderColumns = n % num_thrds;
	int columnsToBeProcessed = n / num_thrds;

	//check if remainder Columns exist
	if(remainderColumns > 0){
		int leftoverRemainder = remainderColumns - task_id;

		//check if there's still any leftover remainder
		//columns to be processed
		if(leftoverRemainder > 0){
			startIndex = task_id * columnsToBeProcessed + task_id;
			endIndex = startIndex + columnsToBeProcessed + 1;
		}else if(leftoverRemainder == 0){
			startIndex = task_id * columnsToBeProcessed + task_id;
			endIndex = startIndex + columnsToBeProcessed;
		}else{
			startIndex = task_id * columnsToBeProcessed + remainderColumns;
			endIndex = startIndex + columnsToBeProcessed;
		}

	//if remainder is 0
	}else{
		startIndex = task_id * columnsToBeProcessed;
		endIndex = startIndex + columnsToBeProcessed;
	}

	//print which columns are going to be processed by this thread
	printf("thread %d: will process column [%d - %d]\n", task_id, startIndex, endIndex-1); 

	//get the sum of the products of each Kth column element of matrix A
	//and each Kth row element of matrix B
	for(int z = startIndex; z < endIndex; z++){

		//loop through each row of matrix A
		for(int x = 0; x < m; x++){

			//initialise the particular element of C to 0 
			C[x][z] = 0;

			//loop through every column of matrix A row
			for(int y = 0; y < k; y++){
				C[x][z] = C[x][z] + (A[x][y] * B[y][z]);
			}

		}

	}
	pthread_exit(0);
}

//the matrix multiplication function
void sequentialMatrixMultiplication(float ** A, float ** B, float ** C, int m, int k, int n){
	//M indicates the row for the first matrix
	//K indicates the column for the first matrix AND the row for the second matrix
	//N indicates the column for the second matrix
	//the result matrix C will be in the form of M X N

	printf("====================================================\n");
	printf("==== Result of sequential matrix multiplication ====\n");
	printf("====================================================\n");

	//loop through the row of the first matrix
	for(int x = 0; x < m; x++){



		printf("[");
		for(int z = 0; z < n; z++){

			//initialise C[x][z] to zero 
			C[x][z] = 0;

			//loop through each column of the first matrix 
			//AND at the same time
			//loop through each row for the second matrix
			for(int y = 0; y < k; y++){
				C[x][z] = C[x][z] + (A[x][y] * B[y][z]);
			}	
			printf(" %f ",C[x][z]);
		}
		//go to the next line
		printf("]\n");
	}


}

//compare the sequentially calculated matrix with the parallel calculated matrix
void compareMatrices(float ** sequentialMatrix, float ** parallelMatrix, int row, int column){
	
	bool matricesAreSame = true;

	//make sure each element of 2 matrices are the same
	for(int i = 0; i < row; i++){
		for(int j = 0; j < column; j++){
			if(!(sequentialMatrix[i][j] == parallelMatrix[i][j])){

				//if the matrices element are not the same, change 
				//boolean value to false
				matricesAreSame = false;
			}
		}	
	}	

	//print out apporpriate messages according to the boolean value
	if(matricesAreSame){ 
		printf("\n========The two matrices are the same!===========\n\n");
	}else{
		printf("\n========The two matrices are different, there's an error!========\n\n");
	}


}

void printMatrix(float ** matrix, int row, int column){
	for(int i = 0; i < row; i++){
		printf("[");
		for(int j = 0; j < column; j++){
			printf(" %f ", matrix[i][j]);
		}
		printf("]\n");
	}

	printf("\n");
}

int main(int argc, char * argv[]){
	
	//ask user to specify matrix dimension values
	printf("Please enter the dimension value M: ");
	scanf("%d",&m);
	printf("\nPlease enter the dimension value K: ");
	scanf("%d",&k);
	printf("\nPlease enter the dimension value N: ");
	scanf("%d",&n);

	//ask user to also specify the num_thrds to be created
	printf("Please enter the number of threads to be created: ");
	scanf("%d", &num_thrds);

	//check if num_thrds is <= N and not 0 or negative number
	//and ask user for num_thrds again if that's the case 
	while(num_thrds < 1 || num_thrds > n){
		printf("num_thrds has to be more than 0 and <= to N\n");
		scanf("%d", &num_thrds);
	}

	//declare the normal thread ids
	pthread_t thread_id[num_thrds];	

	//create a variable to store the return value of pthread_create()
	int rc;

	//declare the user defined thread_id
	int * user_defined_id = (int *) malloc(num_thrds * sizeof(int));

	//print out the values for testing
	printf("The value of M is: [%d]\n",m);
	printf("The value of K is: [%d]\n",k);
	printf("The value of N is: [%d]\n",n);

	//dynamically create 2 matrices using malloc 

	//first matrix has dimension of M X K
	if((A = (float **) malloc(m * sizeof(float *))) != NULL){
		for(int i = 0; i < m; i++){
			A[i] = malloc(k * sizeof(float));
		}
	}else{
		fprintf(stderr, "A runs out of memory\n");
		exit(1);
	}

	//second matrix has dimension of K X N
	if((B = (float**)malloc(k * sizeof(float*))) != NULL){
		for(int i = 0; i < k; i++){
			B[i] = malloc(n * sizeof(float));
		}
	}else{
		fprintf(stderr, "B runs out of memory\n");
	}

	//third matrix has dimension of M X N
	if((C = (float**)malloc(m * sizeof(float*))) != NULL){
		for(int i = 0; i < m; i++){
			C[i] = malloc(n * sizeof(float));
		}
	}else{
		fprintf(stderr, "C runs out of memory\n");
	}

	//make another matrix to store the sequential matrix multiplication
	//result (also has M X N dimension)
	if((D = (float**)malloc(m * sizeof(float*))) != NULL){
		for(int i = 0; i < m; i++){
			D[i] = malloc(n * sizeof(float));
		}
	}else{
		fprintf(stderr, "D runs out of memory\n");
	}

	//give the random number generator a seed
	srand((unsigned int)time(NULL));

	//populate the A matrix with random float numbers
	for(int i = 0; i < m; i++){
		for(int j = 0; j < k; j++){
			A[i][j] = ((float)rand()/(float)(RAND_MAX)) * 0.5;
		}
	}

	//populate the B matrix with random float numbers
	for(int i = 0; i < k; i++){
		for(int j = 0; j < n; j++){
			B[i][j] = ((float)rand()/(float)(RAND_MAX)) * 0.5;
		}
	}

	//print out A and B!
	printf("Matrix A is: \n");
	printMatrix(A,m,k);
	printf("Matrix B is: \n");
	printMatrix(B,k,n);

	//create num_thrds number of threads and execute the function
	// multiThreadedMatrixMultiplication on each thread
	for(int i = 0; i < num_thrds; i++){

		//assign the index number to user defined id
		//and pass it to each thread created
		user_defined_id[i] = i;
		rc = pthread_create(&thread_id[i],NULL,multiThreadedMatrixMultiplication,&user_defined_id[i]);
		if(rc){
			printf("ERROR; return code from pthread_create() is %d\n",rc);
			exit(-1);
		}
	}

	//join all the threads
	for(int i = 0; i < num_thrds; i++){
		pthread_join(thread_id[i],NULL);
	}

	printf("=======================================================\n");
	printf("==== Result of multithreaded matrix multiplication ====\n");
	printf("=======================================================\n");
	printMatrix(C,m,n);

	//multiply the 2 matrices sequentially 
	sequentialMatrixMultiplication(A,B,D,m,k,n);

	//make sure that the matrices are the same
	compareMatrices(D,C,m,n);



	//FREE THE ALLOCATED MEMORY!

	//free the allocated user defined threads
	free(user_defined_id);

	//first matrix has dimension of M X K
	for(int i = 0; i < m; i++){
		free(A[i]);
	}
	free(A);

	//second matrix has dimension of K X M
	for(int i = 0; i < k; i++){
		free(B[i]);
	}
	free(B);

	//third matrix has dimension of M X N
	for(int i = 0; i < m; i++){
		free(C[i]);
	}
	free(C);

	//matrix D also has to be de-allocated
	for(int i = 0; i < m; i++){
		free(D[i]);
	}
	free(D);

	return 0;
}
