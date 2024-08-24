#ifndef CACHE_H
#define CACHE_H


#include <time.h> 


struct cacheNode{
    char* data;
    int dataLength;
    char* url;
    time_t LRUtimeTrack;
    struct cacheNode* next; 
};

struct cacheNode* findCacheEle(char* url);
int addCacheEle(char* data, int size, char* url);
int removeCacheEle(); //return 1 on success & 0 on failure


#endif