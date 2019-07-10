#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(){
	FILE *fp = fopen("index.html", "r");
	int cnt = 0;
	while(1){
		char c = fgetc(fp);
		printf("%c", c);
		cnt++;
		if(c == EOF) break;
	}
	printf("\n cnt : %d\n", cnt);
			
}
