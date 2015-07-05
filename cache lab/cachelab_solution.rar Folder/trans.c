/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i,j,k,m,tmp0,tmp1,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7;
    REQUIRES(M > 0);
    REQUIRES(N > 0);
/*
 * Idea for M=32 and N=32:
 * Since each block holds 8 numbers and the cache can hold 8 lines
 * (ex.B[0] to B[7]), we can fetch the whole line(A[j][i] to A[j][i+7]) at
 * a time and write it to the right place of B to decrease misses. 
 */
    if(M==32)		//B[i][j]=A[j][i]	
    {	
    	for(i=0;i<32;i+=8)	
		for(j=0;j<32;++j)
		{
			tmp0=A[j][i];
			tmp1=A[j][i+1];
			tmp2=A[j][i+2];
			tmp3=A[j][i+3];
			tmp4=A[j][i+4];
			tmp5=A[j][i+5];
			tmp6=A[j][i+6];
			tmp7=A[j][i+7];
			B[i][j]=tmp0;
			B[i+1][j]=tmp1;
			B[i+2][j]=tmp2;
			B[i+3][j]=tmp3;
			B[i+4][j]=tmp4;
			B[i+5][j]=tmp5;
			B[i+6][j]=tmp6;
			B[i+7][j]=tmp7;
		}
    }			
/*
 * Idea for M=64 and N=64:
 * This is similar to the idea when dealing with M=32 and N=32,and we can still 
 * fetch a line of A[0] to A[3] at a time, but since the cache is not big 
 * enough, it can hold only 4 lines at a time. So firstly I put the first 
 * 4 elements of  a line in A to its right place in B and put last 4 elements 
 * somewhere else(of course they have already been fetched!). Next, move the 
 * value in the "wrong positions" to its right position and get the right 
 * value for these "wrong positions". Last, repeat the same process for 
 * A[4] to A[7]. Pitifully, there are no more positions to store the last four 
 * elements like before, so I just do simple transformation(I believe here can 
 * be optimized...)
 */
    else if(M==64)	//B[i][j]=A[j][i]
    {
	for(i=0;i<64;i+=8)
		for(j=0;j<64;j+=8)
		{
			for(k=j;k<j+4;++k)
			{
				tmp0=A[k][i];
				tmp1=A[k][i+1];
				tmp2=A[k][i+2];
				tmp3=A[k][i+3];
				tmp4=A[k][i+4];
				tmp5=A[k][i+5];
				tmp6=A[k][i+6];
				tmp7=A[k][i+7];
				B[i][k]=tmp0;
				B[i][k+4]=tmp4;
				B[i+1][k]=tmp1;
				B[i+1][k+4]=tmp5;
				B[i+2][k]=tmp2;
				B[i+2][k+4]=tmp6;
				B[i+3][k]=tmp3;
				B[i+3][k+4]=tmp7;								
			}
			for(k=i;k<i+4;++k)
			{
				tmp0=B[k][j+4];
				tmp1=B[k][j+5];
				tmp2=B[k][j+6];
				tmp3=B[k][j+7];
				tmp4=A[j+4][k];
				tmp5=A[j+5][k];
				tmp6=A[j+6][k];
				tmp7=A[j+7][k];
				B[k][j+4]=tmp4;
				B[k][j+5]=tmp5;
				B[k][j+6]=tmp6;
				B[k][j+7]=tmp7;
				B[k+4][j]=tmp0;
				B[k+4][j+1]=tmp1;
				B[k+4][j+2]=tmp2;
				B[k+4][j+3]=tmp3;
			}
			for(k=i+4;k<i+8;++k)
			{
				tmp0=A[j+4][k];
				tmp1=A[j+5][k];
				tmp2=A[j+6][k];
				tmp3=A[j+7][k];
				B[k][j+4]=tmp0;
				B[k][j+5]=tmp1;
				B[k][j+6]=tmp2;
				B[k][j+7]=tmp3;
			}
		}
    }
/*
 * Idea for M=61 and N=67:
 * Use blocking! And I tried many block sizes, and I find 17 is a good one:)
 */
     else if(M==61)	
    {	
	    for(i=0;i<61;i+=17) 		//B[i][j]=A[j][i]
        	for(j=0;j<67;j+=17) 
		    for(k=j;k<j+17 && k<67;++k)
			for(m=i;m<i+17 && m<61;++m)
            		{
				tmp0=A[k][m];
           			B[m][k]=tmp0;
			}
        
    }
    
    ENSURES(is_transpose(M, N, A, B));
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

