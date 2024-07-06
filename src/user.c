#include "user.h"
#include "csapp.h"
#include <debug.h>
#include "helper.h"

// hadnle for user object --> fixed at the time user is created 

// reference counted

// managed by user registry 

// list of users to store users objects:  // TODO: Do we need to store the users anywhere? 

 typedef struct user{
    char *handle; // user's handle 
    sem_t mutex; // mutex
    int ref_count; //counter for references
    // For the list of users: 
    USER *next; // link to thenext user in the list of all users
    USER *prev; // link to the previous user in the list of all users   
} USER;

// Method to create a new user: 
USER *user_create(char *handle){
    // STEP 1: Allocate memory on the heap for a new object: 
    USER *new_user = (void *)malloc(sizeof(USER)); // allocate memory for new user
    new_user -> handle = malloc(strlen(handle)+ 1); // allocate memory for a handle of the new user ----> + 1 for the null terminator !!!!!!!!
    // intialize the links to the prev and next users to NULL: 
    new_user -> next = NULL; 
    new_user -> prev = NULL;
    
    // STEP 2: define new handler for the new user: 
    if (new_user -> handle == NULL) { // handle malloc errors
        fprintf(stderr, "%s\n", "Memory allocation failed"); // TODO: What should we use, perror or frpintf for error handling? 
        exit(EXIT_FAILURE); // TODO: Should I exit here or return NULL? 
    }  
    // Copy the new handle for the user: 
    //strcpy(new_user -> handle, handle);
    
    new_user -> handle = strdup(handle);
    if (new_user -> handle == NULL) { 
        free(new_user); // free the new user struct 
        fprintf(stderr, "%s\n","Failed to allocate memory for handle");
        return NULL;
    }
    
    // STEP 3: mutex and reference counter: 
    new_user -> ref_count = 1; // initialize reference counter // TODO: should it be initializeto 1 or to 0 at first? 
    
    // mutex: 
    
    int semaphore = sem_init(&(new_user -> mutex), 0, 1);   // Initialize it to 1 for semaphore
    
    if(semaphore == -1){
        fprintf(stderr, "%s\n", "Couldn't initialize semaphore ");
        free(new_user -> handle); // free the handle 
        free(new_user); // free the new user struct 
        return NULL;
    }
    //user_ref(new_user, "user _create() call.");
    
    return new_user; 
}

// Method to increase the reference counter of a user: 
USER *user_ref(USER *user, char *why){ // why this why? 
     REF_COUNTED_OBJECT_REF(user, why);    
    return user;
}

// Method to decrease the ref_counter of a user by one and if it reached 0, free it:
void user_unref(USER *user, char *why){
    debug("decrease reference count for a user: %s", why); // TODO: check if it what we need to do
    // STEP 1: lock the mutex:
    P(&user -> mutex);
    // STEP 2: Decrease the ref_count: 
    user -> ref_count --;     
    // STEP 4: check if the ref_count has reached 0: 
    if (user -> ref_count == 0){
        // If yes, free the user: 
        if (user -> handle != NULL)
            free(user -> handle);
        // fix the linking between the prev and next of the user to free: 
        if (user -> prev != NULL && user -> next != NULL){
            // new prev for the next: 
            user -> prev -> next = user -> next;
            // new next for the prev: 
            user -> next -> prev = user -> prev; 
        }
        user -> next = NULL; // TODO: Do I need to do that? 
        user -> prev = NULL;
        //V(&user->mutex); // Do I need to do wait here agian? 
        // Destroy the mutex: 
        sem_destroy(&user -> mutex); 
        free(user);    
    }
    else {
        // STEP 3: Unclock the mutex: 
        V(&user -> mutex);
    }
}

// Getter method for an user's handler: 
char *user_get_handle(USER *user){
    return user -> handle; 
}

