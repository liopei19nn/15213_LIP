// Name: Li Pei
// ID: lip


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
    REQUIRES(M > 0);
    REQUIRES(N > 0);
    
    if (M == 32) {
        for (int right_move = 0; right_move < 32; right_move += 8) {
            
            for (int j = 0; j < 32; j++) {
                int temp = 0;
                int temp_x = 0;
                int temp_y = 0;
                int flag = 0; //if diagnal,flag = 1 
                
                for (int i = right_move; i < right_move + 8; i++) {
                    if (i == j) {
                        temp = A[j][i];
                        temp_x = i;
                        temp_y = j;
                        flag = 1;
                    }
                    else{
                        B[i][j] = A[j][i];
                    }
                }
                if (flag) {
                    B[temp_x][temp_y] = temp;
                    flag = 0;
                    
                }
            }
        }
    }
    
    if (M == 61) {
        int block_height = 16;
        int block_width = 8;
        for (int right_move = 0; right_move < 61; right_move += block_height) {
            for (int down_move = 0; down_move < 67; down_move += block_width) {
                for(int i = down_move; i< down_move+block_width && i < 67;i++){
                    for (int j = right_move; j < right_move+block_height && j < 61; j++) {
                        B[j][i] = A[i][j];
                    }
                }
            }
        }
        
        
    }
    
    if (M == 64) {
        int a0 = 0;
        int a1 = 0;
        int a2 = 0;
        int a3 = 0;
        int a4 = 0;
        int a5 = 0;
        int a6 = 0;
        int a7 = 0;
        int down_move =0;
        int right_move = 0;
        int i = 0;
        int j = 0;
        
        for (down_move = 0; down_move < 64; down_move = down_move + 8) {
            for (right_move = 0; right_move < 64; right_move = right_move + 8) {
                for (int i = down_move; i < down_move + 4; i++) {
                    int temp = 0;
                    int temp_row = 0;
                    int temp_col = 0;
                    int flag = 0; //if diagnal,flag = 1 
                    
                    for (int j = right_move; j < right_move + 4; j++) {
                        if (i == j) {
                            temp = A[i][j];
                            temp_row = i;
                            temp_col = j;
                            flag = 1; 
                        }
                        else{
                            B[j][i] = A[i][j];
                        }
                    }
                    if (flag) {
                        B[temp_col][temp_row] = temp;
                        flag = 0;
                        
                    }
                }
                a0 = A[down_move][right_move + 4];
                a1 = A[down_move][right_move + 5];
                a2 = A[down_move][right_move + 6];
                a3 = A[down_move][right_move + 7];
                a4 = A[down_move + 1][right_move + 4];
                a5 = A[down_move + 1][right_move + 5];
                a6 = A[down_move + 1][right_move + 6];
                a7 = A[down_move + 1][right_move + 7];
                for (i = down_move + 4; i < down_move + 8; i++) {
                    for (j = right_move; j < right_move + 4; j++) {
                        B[j][i] = A[i][j];
                    }
                }
                for (int i = down_move + 4; i < down_move + 8; i++) {
                    int temp = 0;
                    int temp_row = 0;
                    int temp_col = 0;
                    int flag = 0;//if diagnal,flag = 1 
                    
                    for (int j = right_move + 4; j < right_move + 8; j++) {
                        if (i == j) {
                            temp = A[i][j];
                            temp_row = i;
                            temp_col = j;
                            flag = 1;
                        }
                        else{
                            B[j][i] = A[i][j];
                        }
                    }
                    if (flag) {
                        B[temp_col][temp_row] = temp;
                        flag = 0;
                        
                    }
                }
                B[right_move + 4][down_move + 0] = a0;
                B[right_move + 5][down_move + 0] = a1;
                B[right_move + 6][down_move + 0] = a2;
                B[right_move + 7][down_move + 0] = a3;
                B[right_move + 4][down_move + 1] = a4;
                B[right_move + 5][down_move + 1] = a5;
                B[right_move + 6][down_move + 1] = a6;
                B[right_move + 7][down_move + 1] = a7;
                for (i = down_move + 2; i < down_move + 4; i++) {
                    for (j = right_move + 4; j < right_move + 8; j++) {
                        B[j][i] = A[i][j];
                    }
                }
                
            }
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

