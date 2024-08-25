#include "../include/cache.h"

#define MAX_ELEMENT_SIZE (1<<13)
#define MAX_CACHE_SIZE (1<<30)

struct cacheNode* head = NULL;
int cacheSize = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct cacheNode* findCacheEle(char* url){
    struct cacheNode* site = NULL;
    int mutexValue = pthread_mutex_lock(&mutex);   
    printf("find cache element: lock acquired: %d\n", mutexValue);

    if(head != NULL){
        site = head;
        while(site != NULL){
            if(strcmp(site->url, url) == 0){
                printf("LRU time track before: %ld\n", site->LRUtimeTrack);
                printf("URL found\n");
                site->LRUtimeTrack = time(NULL);
                printf("LRU time track after: %ld\n", site->LRUtimeTrack);
                break;
            }
            site = site->next;
        }
    } 
    else{
        printf("head is NULL: URL not found\n");
    }

    mutexValue = pthread_mutex_unlock(&mutex);
    printf("find cache element: lock unlocked: %d\n", mutexValue);

    return site;
}

int addCacheEle(char* data, int size, char* url){
    int mutexValue = pthread_mutex_lock(&mutex);   
    printf("add cache element lock acquired: %d\n", mutexValue);
    int totalEleSize = size + 1 + strlen(url) + sizeof(struct cacheNode);
    
    if(totalEleSize>MAX_ELEMENT_SIZE){
        mutexValue = pthread_mutex_unlock(&mutex);
        printf("add cache element: lock unlocked\n");
        printf("total elemet size exceeded limit\n");
        return 0;
    }
    else{
        while(cacheSize+totalEleSize > MAX_ELEMENT_SIZE){
            removeCacheEle();
        }
        struct cacheNode * element = (struct cacheNode*) malloc(sizeof(struct cacheNode));
        element->data = (char*) malloc(size+1);
        strcpy(element->data, data);
        element->url = (char*) malloc( (strlen(url)*sizeof(char)) + 1 );
        strcpy(element->url, url);
        element->LRUtimeTrack = time(NULL);
        element->dataLength = size;
        element->next = head;
        head = element;

        cacheSize += totalEleSize;

        mutexValue = pthread_mutex_unlock(&mutex);
		printf("element added to cache with URL: %s\n", element->url);
        printf("add element: lock unlocked: %d\n", mutexValue);
        return 1;
    }

    return 0; 
}

void removeCacheEle(){
    struct cacheNode* prev;   
    struct cacheNode* next;    
    struct cacheNode* temp;
    int mutexValue = pthread_mutex_lock(&mutex);   
    printf("remove cache element: lock acquired: %d\n", mutexValue);

    if(head!=NULL){
       prev=head, next=head, temp=head;
       while(next->next != NULL){
            if( ((next->next)->LRUtimeTrack) <  (temp->LRUtimeTrack) ){
                temp = prev->next;
                prev = next;
            }
            next=next->next;
       }

       if(temp == head){
            head = head->next;
       }
       else{
            next->next = temp->next;
       }

       cacheSize = cacheSize - (temp->dataLength) - sizeof(struct cacheNode) - strlen(temp->url) - 1;
       free(temp->url);
       free(temp->data);
       free(temp); 
    }

    mutexValue = pthread_mutex_unlock(&mutex);
    printf("remove cache element: lock unlocked: %d\n", mutexValue);
    return;
}