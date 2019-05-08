#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

/*** Skeleton for Lab 2 ***/

// #define TIME_COUNT
// #define DEBUG_1
// #define DEBUG_2

/***** Globals ******/
// float **a; /* The coefficients */
float *a;
float *x;  /* The unknowns */
float *b;  /* The constants */
float err; /* The absolute relative error */
int num = 0;  /* number of unknowns */


/****** Function declarations */
void check_matrix(); /* Check whether the matrix will converge */
void get_input();  /* Read input from file */

/********************************/



/* Function definitions: functions are ordered alphabetically ****/
/*****************************************************************/

/* 
   Conditions for convergence (diagonal dominance):
   1. diagonal element >= sum of all other elements of the row
   2. At least one diagonal element > sum of all other elements of the row
 */
// this function is not used, thus not modified
void check_matrix()
{
  int bigger = 0; /* Set to 1 if at least one diag element > sum  */
  int i, j;
  float sum = 0;
  float aii = 0;
  
  for(i = 0; i < num; i++)
  {
    sum = 0;
    // aii = fabs(a[i][i]);
    aii = fabs(a[i*num+i]);
    
    for(j = 0; j < num; j++)
       if( j != i)
	 // sum += fabs(a[i][j]);
   sum += fabs(a[i*num+j]);
       
    if( aii < sum)
    {
      printf("The matrix will not converge.\n");
      exit(1);
    }
    
    if(aii > sum)
      bigger++;
    
  }
  
  if( !bigger )
  {
     printf("The matrix will not converge\n");
     exit(1);
  }
}


/******************************************************/
/* Read input from file */
/* After this function returns:
 * a[][] will be filled with coefficients and you can access them using a[i][j] for element (i,j)
 * x[] will contain the initial values of x
 * b[] will contain the constants (i.e. the right-hand-side of the equations
 * num will have number of variables
 * err will have the absolute error that you need to reach
 */
void get_input(char filename[])
{
  FILE * fp;
  int i,j;  
 
  fp = fopen(filename, "r");
  if(!fp)
  {
    printf("Cannot open file %s\n", filename);
    exit(1);
  }

 fscanf(fp,"%d ",&num);
 fscanf(fp,"%f ",&err);

 /* Now, time to allocate the matrices and vectors */
 // a = (float**)malloc(num * sizeof(float*));
 a = (float*)malloc(num * num * sizeof(float));
 if( !a)
  {
	printf("Cannot allocate a!\n");
	exit(1);
  }

/*
 for(i = 0; i < num; i++) 
  {
    a[i] = (float *)malloc(num * sizeof(float)); 
    if( !a[i])
  	{
		printf("Cannot allocate a[%d]!\n",i);
		exit(1);
  	}
  }
*/
 
 x = (float *) malloc(num * sizeof(float));
 if( !x)
  {
	printf("Cannot allocate x!\n");
	exit(1);
  }


 b = (float *) malloc(num * sizeof(float));
 if( !b)
  {
	printf("Cannot allocate b!\n");
	exit(1);
  }

 /* Now .. Filling the blanks */ 

 /* The initial values of Xs */
 for(i = 0; i < num; i++)
	fscanf(fp,"%f ", &x[i]);
 
 for(i = 0; i < num; i++)
 {
   for(j = 0; j < num; j++)
     // fscanf(fp,"%f ",&a[i][j]);
     fscanf(fp,"%f ",&a[i*num+j]);
   
   /* reading the b element */
   fscanf(fp,"%f ",&b[i]);
 }
 
 fclose(fp); 

}


/************************************************************/


int main(int argc, char *argv[])
{

 /* Lab 2 - MPI */

 // MPI starts here
 int comm_sz, my_rank;
 MPI_Init(NULL, NULL);
 MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
 MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

 // process 0 reads the input file
 if (my_rank == 0) {
  
 if( argc != 2)
 {
   printf("Usage: ./gs <filename>\n");
   exit(1);
 }

 #ifdef TIME_COUNT
 double read_time_start = MPI_Wtime();
 #endif
  
 /* Read the input file and fill the global data structure above */ 
 get_input(argv[1]);
 
 /* Check for convergence condition */
 /* This function will exit the program if the coffeicient will never converge to 
  * the needed absolute error. 
  * This is not expected to happen for this programming assignment.
  */
 // check_matrix();

 #ifdef DEBUG_2
 printf("hello from %d\n", my_rank);
 printf("num = %d\n", num);
 #endif

 if (comm_sz > num) {
   printf("Number of processes must be equal to or smaller than number of unknowns!\n");
   exit(1);
 }

 #ifdef TIME_COUNT
 double read_time_end = MPI_Wtime();
 printf("Reading input file time: %f\n", read_time_end - read_time_start); 
 #endif

 } 
 else {

   #ifdef DEBUG_2
   printf("hello from %d\n", my_rank);
   printf("num = %d\n", num);
   #endif

 }

 #ifdef DEBUG_2
 for (int i = 0; i < num; i++) {
   printf("%f ", b[i]);
 }
 printf("\n");
 #endif

 #ifdef TIME_COUNT
 double cal_time_start;
 if (my_rank == 0) {
   cal_time_start = MPI_Wtime();
 }
 #endif

 // distribute variables
 // broadcast num and err
 MPI_Bcast(&num, 1, MPI_INT, 0, MPI_COMM_WORLD);
 MPI_Bcast(&err, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

 // prepare to broadcast the old x
 if (my_rank != 0) {
   x = (float *) malloc(num * sizeof(float));
 if( !x)
  {
	printf("Cannot allocate x in process %d!\n", my_rank);
	exit(1);
  }
 }
 // broadcast the old x
 MPI_Bcast(x, num, MPI_FLOAT, 0, MPI_COMM_WORLD);

 #ifdef DEBUG_2
 if (my_rank == 0) {
   printf("num = %d\n", num);
   printf("err = %f\n", err);
   for (int i = 0; i < num; i++) {
    printf("%f ", x[i]);
   }
   printf("\n");
 }
 #endif

 // scatter a and b
 // first, distribute bucket size evenly
 int basic_bucket_size = num / comm_sz;
 int extra_bucket_num = num - comm_sz * basic_bucket_size;

 #ifdef DEBUG_2
 printf("basic bucket size = %d\n", basic_bucket_size);
 printf("extra bucket num = %d\n", extra_bucket_num);
 #endif

 // prepare scatterv

 // int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
 //                MPI_Datatype sendtype, void *recvbuf, int recvcount,
 //                MPI_Datatype recvtype,
 //                int root, MPI_Comm comm)

 // sendbuf is a or b

 // sendcounts are different for a and b
 int *sendcounts_a = (int *) malloc(comm_sz * sizeof(int));
 int *sendcounts_b = (int *) malloc(comm_sz * sizeof(int));
 for (int i = 0; i < extra_bucket_num; i++) {
   sendcounts_a[i] = (basic_bucket_size + 1) * num;
   sendcounts_b[i] = basic_bucket_size + 1;
 }
 for (int i = extra_bucket_num; i < comm_sz; i++) {
   sendcounts_a[i] = basic_bucket_size * num;
   sendcounts_b[i] = basic_bucket_size;
 }

 // displs is different for a and b
 int *displs_a = (int *) malloc(comm_sz * sizeof(int));
 int *displs_b = (int *) malloc(comm_sz * sizeof(int));
 for (int i = 0; i < extra_bucket_num; i++) {
   displs_a[i] = (basic_bucket_size + 1) * num * i;
   displs_b[i] = (basic_bucket_size + 1) * i;
 }
 for (int i = extra_bucket_num; i < comm_sz; i++) {
   displs_a[i] = (basic_bucket_size + 1) * num * extra_bucket_num + basic_bucket_size * num * (i - extra_bucket_num);
   displs_b[i] = (basic_bucket_size + 1) * extra_bucket_num + basic_bucket_size * (i - extra_bucket_num);
 }

 // sendtype is MPI_FLOAT

 // recvbuf 
 float *recvbuf_a = (float *) malloc(sendcounts_a[my_rank] * sizeof(float));
 float *recvbuf_b = (float *) malloc(sendcounts_b[my_rank] * sizeof(float));

 // recvcount stored in sendcounts

 // recvtype is MPI_FLOAT

 // root is 0

 // comm is MPI_COMM_WORLD

 // now begin scatterv

 MPI_Scatterv
 (
   a,
   sendcounts_a,
   displs_a,
   MPI_FLOAT,
   recvbuf_a,
   sendcounts_a[my_rank],
   MPI_FLOAT,
   0,
   MPI_COMM_WORLD
 );

 MPI_Scatterv
 (
   b,
   sendcounts_b,
   displs_b,
   MPI_FLOAT,
   recvbuf_b,
   sendcounts_b[my_rank],
   MPI_FLOAT,
   0,
   MPI_COMM_WORLD
 );

 #ifdef DEBUG_2
 if (my_rank == 0) {
   for (int i = 0; i < sendcounts_b[my_rank]; i++) {
     printf("%f ", recvbuf_b[i]);
   }
   printf("\n");
 }
 #endif

 #ifdef DEBUG_2
 if (my_rank == 0) {
   for (int i = 0; i < sendcounts_a[my_rank]; i++) {
     printf("%f ", recvbuf_a[i]);
   }
   printf("\n");
 }
 #endif

 // calculations
 int nit = 0; /* number of iterations */

 int err_satisfied = 0; /* 1 if the error condition satisfied, 0 otherwise */

 // holder for updated x
 float* next_x = (float *) malloc(num * sizeof(float));
 if( !next_x)
  {
	printf("Cannot allocate next_x in process %d!\n", my_rank);
	exit(1);
  }

 // begin loop
 while (!err_satisfied) {

   // 1 if the error condition for current process satisfied
   int current_satisfied = 1;

   // update x for current process
   for (int i = 0; i < sendcounts_b[my_rank]; i++) {

     // calculate new x
     int row_x = displs_b[my_rank] + i;
     float new_x = recvbuf_b[i];
     for (int j = 0; j < num; j++) {
       if (j == row_x){
         continue;
       }
       new_x -= recvbuf_a[i*num+j] * x[j];
     }
     new_x /= recvbuf_a[i*num+row_x];

     // evaluate error
     float current_err = fabsf((new_x - x[row_x]) / new_x);
     if (current_err <= err) {
       current_satisfied &= 1;
     } else {
       current_satisfied &= 0;
     }

     next_x[row_x] = new_x;

   }

   // use allreduce to let all processes know if next round is needed
   err_satisfied = 1;

   MPI_Allreduce
   (
     &current_satisfied,
     &err_satisfied,
     1,
     MPI_INT,
     MPI_BAND,
     MPI_COMM_WORLD
   );

   // use allgatherv to push the updated value to original x

   // int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
   //                void *recvbuf, const int *recvcounts, const int *displs,
   //                MPI_Datatype recvtype, MPI_Comm comm)

   // sendbuf is part of next_x

   // sendcount is related to sendcounts_b

   // sendtype is MPI_FLOAT

   // recvbuf is x

   // recvcounts is sendcounts_b

   // displs is displs_b

   // recvtype is MPI_FLOAT

   // comm is MPI_COMM_WORLD

   // now begin alltogetherv

   MPI_Allgatherv
   (
     &next_x[displs_b[my_rank]],
     sendcounts_b[my_rank],
     MPI_FLOAT,
     x,
     sendcounts_b,
     displs_b,
     MPI_FLOAT,
     MPI_COMM_WORLD
   );

   nit++;

 }

 #ifdef TIME_COUNT
 double cal_time_end;
 if (my_rank == 0) {
   cal_time_end = MPI_Wtime();
   printf("Calculation time: %f\n", cal_time_end - cal_time_start); 
 }
 #endif

 // process 0 writes the output file
 if (my_rank == 0) {

 #ifdef TIME_COUNT
 double write_time_start = MPI_Wtime();
 #endif

 /* Writing results to file */
 FILE* fp;

 char output[100] ="";
 sprintf(output,"%d.sol",num);
 fp = fopen(output,"w");
 if(!fp)
 {
   printf("Cannot create the file %s\n", output);
   exit(1);
 }
    
 for(int i = 0; i < num; i++)
   fprintf(fp,"%f\n",x[i]);
 
 fclose(fp);

 #ifdef DEBUG_2
 printf("bye-bye from %d\n", my_rank);
 #endif

 #ifdef TIME_COUNT
 double write_time_end = MPI_Wtime();
 printf("Writing output file time: %f\n", write_time_end - write_time_start); 
 #endif

 printf("total number of iterations: %d\n", nit);

 }

 // MPI ends here
 MPI_Finalize();
 
 exit(0);

}
