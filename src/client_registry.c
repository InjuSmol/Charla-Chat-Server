#include "globals.h"
#include "client_registry.h"

#ifndef CLIENT_H
#define CLIENT_H
#include "client.c"
#include "client.h"

#include <semaphore.h>

//typedef struct client CLIENT;

/*
 * The maximum number of simultaneous clients supported by the registry.
 */
//#define MAX_CLIENTS 64

// Structure to hold the clients of the client registry: 
typedef struct client_registry{
    CLIENT **clients_array;  // a regular array to store the clients
    sem_t mutex;  // mutex
} CLIENT_REGISTRY;


// Method to initialize a new clients registry: 
CLIENT_REGISTRY *creg_init(){
    // step 1: Allocate memory for the new registry: 
    CLIENT_REGISTRY *new_client_registry = (void *)malloc(sizeof(CLIENT_REGISTRY));
    
    new_client_registry -> clients_array = (void *)malloc(sizeof(CLIENT) *(MAX_CLIENTS));
    if (new_client_registry == NULL || new_client_registry -> clients_array == NULL) // TODO: Is it right? 
        return NULL; // malloc error
    int i;
    // STEP 2: Iterate throigh array and initialize all the clients to NULL:
    CLIENT **clients_array_p = new_client_registry -> clients_array;
    for(i = 0; i < MAX_CLIENTS ; i++){
        // Set all the clients to NULL:   
        clients_array_p = NULL;
        clients_array_p++;
    } 
    // STEP 3: Initialize the mutex:
    sem_init(&new_client_registry -> mutex , 0, 1);  
    // Return new creg: 
    return new_client_registry;
}

// Method to finalize a client registry:
void creg_fini(CLIENT_REGISTRY *cr){
    // Iterate through all the MAX_CLIENTS clients: 
    // STEP 1: unref all the clietns in the array: 
    CLIENT **clients_array_p = cr -> clients_array;
    for(int i=0; i < MAX_CLIENTS; i++)
        if (*(clients_array_p + i)  != NULL)
            client_unref(*(clients_array_p + i), "creg_fini() call.");
        
    // STEP 2: destroy the mutex and free the array:      
    free(cr -> clients_array);
    
    sem_destroy(&cr -> mutex);
    // STEP 3: Free the registry: 
    free(cr);
}
 
// Method to register an client:
CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd){
    // STEP 1: Block the mutex:
    P(&cr -> mutex);  
    
    // STEP 2: We need to find the first empty spot in the array:
    int i = 0;
    CLIENT **clients_array_p = cr -> clients_array;
    while((clients_array_p + i) != NULL){
        // If reached rhe end of the array without an empty spot: 
        if (i == MAX_CLIENTS){
            fprintf(stderr, "The clients array is full!\n");
            V(&cr -> mutex);
            return NULL;
        }
        i++;     
    }
    // If reached here ==> the clients_array_p is our empty spot!!
    
    // STEP 3: Create a new client: 
    CLIENT *new_client = client_create(cr, fd);
    if (new_client == NULL){
            fprintf(stderr, "Couldn't create a new client\n");
            V(&cr -> mutex);
            return NULL;
        }
    
    // STEP 4: Insert the new_client:
    cr -> clients_array[i] = new_client;
    
    // STEP 5: Increase the ref_count of the new_client: 
    client_ref(new_client, "creg_register() call.");
    
    V(&cr -> mutex);
    return new_client; 
     
}

// semaphore for the client session: 
static sem_t global_sem; 

// Method to unregister a client: 
int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client){
    // Block the mutex:
    P(&cr -> mutex);
    
    // Need to first find the client in the clients_array: 
    int i = 0;
    CLIENT **clients_array_p = cr -> clients_array;
    
    while(*(clients_array_p + i) != client){
        // If reached the end of the array without an empty spot: 
        if (i == MAX_CLIENTS){
            fprintf(stderr, "Didn't find the client!\n");
            V(&cr -> mutex);
            return -1; // fail
        }
        i++;     
    }
    // here the array pointer points to the client we need to unregister: 
    client_unref(client, "creg_unregistry() call.");
    cr -> clients_array[i] = NULL;
    
    
    // Unblock the mutex:
    V(&cr -> mutex);
    //V(&global_sem);
    V(&global_sem);
    return 0;
}

// Method to get the array of clients:
CLIENT **creg_all_clients(CLIENT_REGISTRY *cr){
    P(&cr -> mutex);
    CLIENT **clients_array_p = cr -> clients_array;
    int i = 0, count = 0;
    // STEP 1: Count the clients:      
    while (i < MAX_CLIENTS){
        if (*(clients_array_p + i) != NULL)
            count ++;
        i++;
    }
    // Pointer to the first client in the array: 
    clients_array_p = cr -> clients_array;
    // STEP 2: Alocate an array to return: 
    CLIENT **array_to_return = (void *)malloc(sizeof(CLIENT)*(count+1));
    // STEP 3: Iterate throguh the clients again: 
    CLIENT **new_array_p = array_to_return;
    i = 0; 
    while (i < MAX_CLIENTS){
        // If the client is not NULL:
        if ((clients_array_p + i) != NULL){
            //insert the client:
            *new_array_p = *(clients_array_p + i);
            new_array_p++;
        }        
        i++;
    }
    // Add NULL terminator: 
    *new_array_p = NULL;
    // Unblock the mutex:
    V(&cr -> mutex);
    
    return array_to_return;
}

// Static mutex and thread routine for shuting down al the clients in cr:
static sem_t mutex_shut_down; 
// Thread routine: 
void thread_2(){
    sem_init(&mutex_shut_down, 0, 1);
}
void creg_shutdown_all(CLIENT_REGISTRY *cr){
    // STEP 1: Initialize the mutex for shut down once: 
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    // Initialize the mutex:  
    pthread_once(&once, thread_2);  
    // Block the mutex: 
    //P(&cr -> mutex);
    P(&mutex_shut_down);
    
    // STEP 2: Count the clients ccurrently in: 
    // Pointer to the clients_array:
    CLIENT **clients_array_p = cr-> clients_array;
    int count = 0; 
    int i = 0; 
     while (i < MAX_CLIENTS){
        // If the client is not NULL:
        if ((clients_array_p + i) != NULL)
            count++;           
        i++;
    }
    // Semaphore for waiting for online clients: 
    sem_init(&global_sem, 0, count);
    i = 0;
    clients_array_p = cr-> clients_array;
    // STEP 3: Iterate throguth the array again and shutdown the socket for each client: 
     while (i < MAX_CLIENTS){
        // If the client is not NULL:
        if ((clients_array_p + i) != NULL){
            P(&global_sem);   
            shutdown(client_get_fd(*clients_array_p), SHUT_RDWR);  // SHUT_RDWR
        }              
        i++;
    }
    // 
    for (int i = 0; i < count; i++)
        P(&global_sem);
        
    
    V(&mutex_shut_down);
    
}

#endif

