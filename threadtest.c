
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#define NUM_THREADS	1

extern void _mercury6_ ();
typedef struct {double time;} shared;
extern shared shared_;

void *callout(void *threadid)
{
	printf("Starting mercury6 in thread %ul\n", threadid);
	mercury6_ ();
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	pthread_t threads[NUM_THREADS];
	int rc, t=0;
	printf("Creating compute thread\n");
	rc = pthread_create(&threads[t], NULL, callout, (void *)t);
	if (rc){
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
	while (1)
	{
		printf ("time = %f\n", shared_.time);
		sleep (1);
	}

	pthread_exit(NULL);
}
