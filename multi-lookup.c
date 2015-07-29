/******************************************************************************
 * FILE: multi-lookup.c
 * DESCRIPTION:
 *   A program which provides dns lookup for address names in input files
 * AUTHOR: Garrett Long
 ******************************************************************************/
#include "multi-lookup.h"

/* Global Variables */
// Queue Mutex
pthread_mutex_t queueMutex;
// AddressQueue
queue addressQueue;
// Output File Mutex
pthread_mutex_t outputFileMutex;
// Output File
FILE *ofp;


void* inputRequest(void *fn){
	char *filename = fn;

	FILE *ifp;
	ifp = fopen(filename, "r");

	if(ifp != NULL){
		char* tmp = malloc(sizeof(char) * MAX_NAME_LENGTH);
	
		while(fscanf(ifp, "%s", tmp) == 1){
			char* hostname = malloc(sizeof(char) * MAX_NAME_LENGTH);
			strcpy(hostname, tmp);

			pthread_mutex_lock(&queueMutex);
			while(queue_is_full(&addressQueue)){
				pthread_mutex_unlock(&queueMutex);
				usleep((rand()%100));
				pthread_mutex_lock(&queueMutex);
			}
			queue_push(&addressQueue, hostname);
			pthread_mutex_unlock(&queueMutex);
			
		}
		free(tmp);
		fclose(ifp);
		
	}else{
		fprintf(stderr, "Can't open input file %s!\n", filename);
	}
	
	
	
	return 0;
}

void* resolveRequests(){
	pthread_mutex_lock(&queueMutex);
	while(!queue_is_empty(&addressQueue)){
		pthread_mutex_unlock(&queueMutex);
		char* ipAddresses = malloc(sizeof(char) * MAX_IP_LENGTH * 25);
		strcpy(ipAddresses, "");
		
		pthread_mutex_lock(&queueMutex);
		char* hostname = queue_pop(&addressQueue);
		pthread_mutex_unlock(&queueMutex);
	
		if(dnslookup2(hostname, ipAddresses) == 0){
			pthread_mutex_lock(&outputFileMutex);
			fprintf(ofp, "%s%s\n", hostname, ipAddresses);
			pthread_mutex_unlock(&outputFileMutex);
			fprintf(stderr, "%s%s\n", hostname, ipAddresses);
		}else{
			pthread_mutex_lock(&outputFileMutex);
			fprintf(ofp, "%s,\n", hostname);
			pthread_mutex_unlock(&outputFileMutex);
			fprintf(stderr, "Bogus Hostname: %s!\n", hostname);
		}
		free(hostname);
		free(ipAddresses);
	pthread_mutex_lock(&queueMutex);
	}
	pthread_mutex_unlock(&queueMutex);
	return 0;
}

int main(int argc, char *argv[])
{
	clock_t begin, end;
	double time_spent;
	begin = clock();
    
	/* If No Input Files Exit */
	if(argc == 1) {
		exit(0);
	}
	
	/* Open Output File */
	char outputFilename[MAX_FILE_NAME_LENGTH];
	strcpy(outputFilename, argv[argc - 1]);
	ofp = fopen(outputFilename, "w");

	if (ofp == NULL) {
	  fprintf(stderr, "Can't open output file %s!\n", outputFilename);
	  exit(1);
	}
	
	/* Initialize Queue and Queue Mutex */
	queue_init(&addressQueue, QUEUE_SIZE);
	pthread_mutex_init(&queueMutex, NULL);
	
///////////////////////////////////////////////////////////////////////////////////////////////////
	/* Requester Thread Generation*/
///////////////////////////////////////////////////////////////////////////////////////////////////
	
	/* Setup Local Variables */
	int file_num, rc;
	pthread_t requestorThreads[MAX_INPUT_FILES];
	int numReqThreads = 0;
	
	for(file_num = 2; file_num < argc && file_num <= MAX_INPUT_FILES + 1; file_num++){

		/* Spawn Requester Thread */
		printf("Spawning Requester thread %i\n", numReqThreads);
		rc = pthread_create(&(requestorThreads[numReqThreads]), NULL, inputRequest, (void *)argv[file_num - 1]);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
		numReqThreads++;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////
	/* Resolver Thread Generation */
///////////////////////////////////////////////////////////////////////////////////////////////////
	
	/* Initialize Output File Mutex */
	pthread_mutex_init(&outputFileMutex, NULL);
	
	/* Determine Number of Processes to Spawn */
	int numCPU = sysconf( _SC_NPROCESSORS_ONLN );
	int numResThreads;
	
	if(numCPU * T_PER_CPU > MAX_RESOLVER_THREADS){
		numResThreads = MAX_RESOLVER_THREADS;
	}else{
		numResThreads = numCPU * T_PER_CPU;
	}	
	
	/* Setup Local Vars */
    pthread_t resolverThreads[numResThreads];
    long t;
    
    for(t = 0; t < numResThreads; t++){
		printf("Spawning Resolver Thread %ld\n", t);
		/* Spawn Resolver Thread */
		rc = pthread_create(&(resolverThreads[t]), NULL, resolveRequests, NULL);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
    }

///////////////////////////////////////////////////////////////////////////////////////////////////
	/* Wait for Threads to Finish */
///////////////////////////////////////////////////////////////////////////////////////////////////
	
	/* Wait for Requester Theads to Finish */
    for(t = 0; t < numReqThreads; t++){
		pthread_join(requestorThreads[t],NULL);
    }
    /* Wait for Resolver Theads to Finish */
    for(t = 0; t < numResThreads; t++){
		pthread_join(resolverThreads[t],NULL);
    }

    printf("All of the threads were completed!\n");

///////////////////////////////////////////////////////////////////////////////////////////////////
	/* Memory Cleanup */
///////////////////////////////////////////////////////////////////////////////////////////////////    
	queue_cleanup(&addressQueue);
	pthread_mutex_destroy(&queueMutex);
	pthread_mutex_destroy(&outputFileMutex);
	fclose(ofp);

///////////////////////////////////////////////////////////////////////////////////////////////////
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("Num Resolver Threads: %i\n", numResThreads);
	printf("Runtime: %f\n", time_spent);
    return 0;
}
