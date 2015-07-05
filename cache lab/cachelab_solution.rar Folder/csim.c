#include <stdio.h>
int main(){
	int x = 0xffffffff;
	//float f  = 123;
	if(x == (float)x){
		printf("!");
	}
	return 0;
}