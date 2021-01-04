#include <sys/mman.h>//for mmap()
#include <stddef.h>//for NULL
#include <pthread.h>//for pthread_mutex_t data type
#include <string.h>//for memset

//16 bytes of memory
typedef char BLOCK[16];

//Memory header
typedef union headerunion{
  struct{
    size_t size;//size of memory
    unsigned status;//status of if the memory is free or not
    union headerunion *next;//link to the next block in the list
  }headerstruct;

  BLOCK stub;//align memory to 16 bytes
}header_t;

//Function Prototypes
void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);

//Helper Function Prototypes
header_t *getFreeBlock(size_t);

//Keep track of head and tail of header linked list
header_t *head, *tail;

//Prevents multiple threads from concurrently accessing memory
pthread_mutex_t global_lock;

/*This function will traverse our linked list of memory blocks and find if there
 is a memory block with the status of free that is large enough to fit the size 
 of the memory chunk we are trying to allocate. If it finds a suitable chunk that 
 already exists it will return this chunk to the calling program. Otherwise it 
 will return NULL.*/
header_t *getFreeBlock(size_t s){
  header_t *current = head;

  while(current){
    if(current->headerstruct.status && current->headerstruct.size >= s) return current;
    
    current = current->headerstruct.next;
  }
  return NULL;
}

void *malloc(size_t objectSize){
  void *memBlock; 
  size_t totalSize; 
  header_t *myHeader;
  
  //if the size wasn't passed exit the function
  if(!objectSize) return NULL;

  pthread_mutex_lock(&global_lock);

  myHeader = getFreeBlock(objectSize);

  //if getFreeBlock found a block flagged as free
  if(myHeader){
    //use this block for "memory allocation"
    myHeader->headerstruct.status = 0;
    pthread_mutex_unlock(&global_lock);
    return (void *)(myHeader+1);
  }

  totalSize = objectSize + sizeof(header_t);
  memBlock = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  //if mmap error code is thrown exit the function
  if(memBlock == (void *)-1){
    pthread_mutex_unlock(&global_lock);
    return NULL;
  }
  
  //instantiate our block
  myHeader = memBlock;
  myHeader->headerstruct.size = objectSize;
  myHeader->headerstruct.status = 0;
  myHeader->headerstruct.next = NULL;

  //deal with linked list
  if(!head) head = myHeader;
  if(tail) tail->headerstruct.next = myHeader;
  tail = myHeader;
  pthread_mutex_unlock(&global_lock);

  return (void *)(myHeader+1);
}


void *calloc(size_t totalElements, size_t elementSize){
  size_t totalSize; 
  void *myMemory;

  //if the total elements or the element size doesn't exist exit function
  if(!totalElements || !elementSize) return NULL;

  totalSize = elementSize * totalElements;

  //if mul overflow exit the function
  if(elementSize != (totalSize/totalElements)) return NULL;

  myMemory = malloc(totalSize);

  //if malloc failed exit the function
  if(!myMemory) return NULL;

  memset(myMemory, 0, totalSize);

  return myMemory;
}

/*
void *realloc(void *myMemory, size_t targetSize){
}*/

void free(void *myMemory){
  header_t *myHeader, *currentHeaderNode;
  void *breakCondition;

  if(!myMemory){
    return;
  }

  pthread_mutex_lock(&global_lock);

  myHeader = ((header_t *)myMemory - 1);

  //gives current value of the break condition (second parameter is 0)
  breakCondition = mmap(NULL, 0, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  //is the block we want to free at the end of the heap?
  if(((char *)myMemory + myHeader->headerstruct.size) == breakCondition){
    //if only one node
    if(head == tail){
      //free the node
      head = tail = NULL;
    }
    else{
      //otherwise traverse the list until we are the tail
      currentHeaderNode = head;

      while(currentHeaderNode){
        if(currentHeaderNode->headerstruct.next == tail){
          //free tail
          currentHeaderNode->headerstruct.next = NULL;
          tail = currentHeaderNode;
        }

        currentHeaderNode = currentHeaderNode->headerstruct.next;
      }
    }

    //releases the amount of memory taken up by the target block
    mmap(NULL, (0 - sizeof(header_t) - myHeader->headerstruct.size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    pthread_mutex_unlock(&global_lock);
    return;
  }

  //if the block is not the last in the linked list, just flag the block as freed
  myHeader->headerstruct.status = 1;
  pthread_mutex_unlock(&global_lock);
}
