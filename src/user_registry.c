#include "user_registry.h"
#include <stddef.h>
#include <string.h>
#include <semaphore.h>
#ifndef USER_H
#define USER_H
#include "user.h"
#include <stdlib.h>
#include <stdio.h>
#include "user.c"

//extern typedef struct user USER;

// define the USER_REGISTRY struct: 
typedef struct user_registry{
    USER *first;  //head user to point to the first user and to the last(initially points to itself)
    sem_t mutex; // mutext for the list of users
}USER_REGISTRY;

// Figure out the struc to hold the user entries----> doubly llinked list:

// Method to initialize a new user registry: 
// (returns the newly initialized USER_REGISTRY, or NULL if initialization fails)
USER_REGISTRY *ureg_init(void){
    // STEP 1: initialize the new users list:     
    USER_REGISTRY *new_user_registry = (void *)malloc(sizeof(USER_REGISTRY));
    if (new_user_registry == NULL){
        fprintf(stderr, "%s\n", "Memory allocation failed"); // TODO: What should we use, perror or frpintf for error handling? 
        return NULL;
    } 
    // STEP 2: Set the list: 
    new_user_registry -> first = (void *)malloc(sizeof(USER));
    new_user_registry -> first -> handle = NULL; // no handler; 
    new_user_registry -> first -> prev = new_user_registry -> first; 
    new_user_registry -> first -> next = new_user_registry -> first;  
    
    // STEP 3: Init the semaphore:   
    int semaphore = sem_init(&new_user_registry -> mutex, 0, 1);
    if (semaphore == -1) // error check
        return NULL;
    return new_user_registry;
}

// Method to finilize the users registry and free all associated resources
// Must not be referenced again: 
void ureg_fini(USER_REGISTRY *ureg){
    // STEP 1: Free the user registry and all the users in it: 
    // iterate through all the users and free each one of them: 
    
    // user_registry_list pointer to point to the first user in the list: 
    USER *ureg_p = ureg -> first -> next; 
    
    while (ureg_p != ureg -> first){
        USER *user_to_free = ureg_p; 
        // go to the next user in the list: 
        ureg_p = user_to_free -> next; 
        // decrease the ref_count of each user which will also take care of freeing it if the ref_count is 0
        user_unref(user_to_free, "ureg_fini() call.");
        free(user_to_free);     
               
    }
    // STEP 2: Descroy user_registry's mutex: 
    sem_destroy(&ureg -> mutex);  
    
    // STEP 3: finally free the user_registry itself: 
    free(ureg);
}


USER *ureg_register(USER_REGISTRY *ureg, char *handle){
     P(&ureg -> mutex); // lock the ureg mutex

    // STEP 1: loop through ureg and check if the user exists, if yes, return it: 
    USER *ureg_p = ureg -> first -> next; 
    while( ureg_p != ureg -> first){
        if (strcmp(ureg_p -> handle, handle) == 0){
            // Increment the ref_count: 
            user_ref(ureg_p, "user already registered");     
            // Unlock the ureg mutex before returning: 
           V(&ureg -> mutex);
            return ureg_p;
        }
        ureg_p = ureg_p -> next; 
    }
    // STEP 2: create a new user: 
    USER *new_user = user_create(handle);
    if (new_user == NULL ){
        //V(&ureg -> mutex);
        return NULL; 
    }
    // Increment the new_user's ref_count: // TODO: should I do it here? 
    user_ref(new_user, "ureg_register a new user"); // should be two 
    
    // STEP 3: Add new user to the user_list list between the head and first user: 
    new_user -> next = ureg -> first -> next;
    new_user -> prev = ureg -> first; 
    ureg -> first -> next = new_user; 
    new_user -> next -> prev = new_user;
    
    // Unlock the ureg mutex before returning:
    V(&ureg -> mutex);
    return new_user; 
}

void ureg_unregister(USER_REGISTRY *ureg, char *handle){
    P(&ureg -> mutex);// Lock the ureg's mutex
    USER *ureg_p = ureg -> first -> next; // get pointer to the first user in the list
    
    // STEP 1: Search the list and get pointer to the user's object to remove: 
    while (ureg_p != ureg -> first){
        if (strcmp(ureg_p -> handle, handle) == 0)
            break; 
        
        ureg_p = ureg_p -> next; 
    }
    if (ureg_p != ureg -> first){ // means is found
    // STEP 2: if found, decrease the reference count of the USER; 
    user_unref(ureg_p, "ureg_unregister");       
    // STEP 3: Remove the user from the linked list:
    ureg_p -> next = NULL; 
    ureg_p -> prev = NULL;   
    ureg_p -> prev -> next = ureg_p -> next; 
    ureg_p -> next -> prev = ureg_p -> prev; 
    }
    V(&ureg -> mutex); // unlock the ureg's mutex
}
#endif
