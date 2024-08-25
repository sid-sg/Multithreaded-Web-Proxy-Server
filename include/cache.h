#ifndef CACHE_H
#define CACHE_H


#include <time.h> 
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct cacheNode{
    char* data;
    int dataLength;
    char* url;
    time_t LRUtimeTrack;
    struct cacheNode* next; 
};

extern struct cacheNode* head;
extern int cacheSize;
extern pthread_mutex_t mutex;


struct cacheNode* findCacheEle(char* url);
int addCacheEle(char* data, int size, char* url);
void removeCacheEle(); 


#endif