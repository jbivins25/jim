#include <stdio.h>
#include <stdlib.h>

int main() {
	char * ptr = malloc(10*sizeof(char));
	for ( int i = 0; i < 9; i++ ) ptr[i] = 'a';
	printf("%s\n",ptr);
	ptr[2] = 0;
	free(ptr);
	return 0;
}
