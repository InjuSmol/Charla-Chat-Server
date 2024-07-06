
#include "mailbox.h"
#include "helper.h"
#include <stdlib.h>
#include "debug.h"
#include <semaphore.h>
#include <string.h>
#include "csapp.h"

// The Node struct for the mailbox queue: 
typedef struct mailbox_node{
    MAILBOX_ENTRY *entry; // message can be either NOTICE or MESSAGE
    struct mailbox_node *next; // next element
    struct mailbox_node *prev; // prev element
}MAILBOX_NODE;

// The mailbox struct: 
typedef struct mailbox{
    MAILBOX_NODE *front;  // first element in the queue
    MAILBOX_NODE *rear;   // last element in the queue
    //int entries; // counter for the number fo entries in the mailbox
    sem_t entries; // semaphore for entries
    MAILBOX_DISCARD_HOOK * hook; // discard function to be called
    char *handle;
    sem_t mutex; // mutex for the mailbox object
    unsigned int ref_count; // ref counter for the mailbox
    unsigned int is_defunct; // weither it is defunct or no
} MAILBOX;

// Helper method for mailbox: 
void mb_add_entry(MAILBOX *mb, MAILBOX_ENTRY_TYPE type, NOTICE_TYPE ntype, int msgid, MAILBOX *from, int length, void *body);


/* So, we have: 

// mailbox ----> queue

// mailbox_node ----> queue node

// mailbox_entry ----> encapsulates the type + contenet itself

// message + notec ----> content types
*/

// Method to set the discard hook for mailbox: 
void mb_set_discard_hook(MAILBOX *mb, MAILBOX_DISCARD_HOOK *hook){ // TODO: Why here is no parameter name huh
    // Block the mutex: 
    P(&mb -> mutex);      
    mb -> hook = hook;
    // Unblock the mutex: 
    V(&mb -> mutex);      
}

// Method to increase the ref_count of the mailbox: 
void mb_ref(MAILBOX *mb, char *why){
    REF_COUNTED_OBJECT_REF(mb, why);
}

// Method to decrease the ref_count for the mailbox
// Free it if the ref_count is 0: 
void mb_unref(MAILBOX *mb, char *why){
    debug("decreasing the ref count: %s", why);
    // Block the mutex: 
    P(&mb -> mutex);
    // Decrease the ref count: 
    mb -> ref_count --;
    
    // If the ref_count reached 0: 
    if(mb -> ref_count == 0){
        // Need to free the mailbox: 
        // STEP 1: Make sure it is defunct: 
        if (mb -> is_defunct != 1){ // TODO: Is it for sure right huh? 
            return; 
        }
        // STEP 2: Free the handle: 
        free(mb -> handle);
        
        // STEP 3: Free all the entries: 
        MAILBOX_NODE *mailbox_p = mb -> front; 
        MAILBOX_NODE *temp = NULL; 
       
        while (mailbox_p != mb -> rear){
            temp = mailbox_p; 
            mailbox_p = mailbox_p -> next; 
            // STEP 3.2 Check if the entry is a message: 
            if(temp -> entry -> type == MESSAGE_ENTRY_TYPE){
                // Check if there is a hook: 
                if(mb -> hook != NULL)
                    mb -> hook(temp -> entry);
                // STEP 3.3 Need to free the message if the etnry is a message:
                free(temp -> entry -> content.message.body); // TODO: What am i freeing exactly here? the whole message or only message.body? 
            }
            // else: if the notice: 
            //else { free(temp -> entry -> content.notice);}
            free(temp -> entry); // free the entry, omg, why it is so complicated
            free(temp); // free the node
        }
        free(mailbox_p); // free the last (rear) in the queue
        
        // STEP 4: free the mailbox itself: 
        free(mb);    
    }
    
    // Unblock the mutex:
    V(&mb -> mutex);
}

// Method to shut downt the mailbox: 
void mb_shutdown(MAILBOX *mb){
    // Block the mutex: 
    P(&mb -> mutex);
    
    // Set the defunct flag: 
    mb -> is_defunct = 1; // is defunct
    // Unblock the entries_count: 
    
    V(&mb -> entries);
    
    // Unblock the mutex:
    V(&mb -> mutex);
}

// Getter method for the handle of mailbox: 
char *mb_get_handle(MAILBOX *mb){
    return mb -> handle; 
}

// Helper method to add an entry to the rear of the mailbox queue
void mb_add_entry(MAILBOX *mb, MAILBOX_ENTRY_TYPE type, NOTICE_TYPE ntype, int msgid, MAILBOX *from, int length, void *body) {
// Method to add a message to the rear of the mailbox queue:
//void mb_add_message(MAILBOX *mb, int msgid, MAILBOX *from, void *body, int length){
    // Block the mutex: 
    P(&mb -> mutex);
    
    // STEP 2: Allocate memory for new entry: 
    MAILBOX_ENTRY *new_entry =(void *)malloc(sizeof(MAILBOX_ENTRY));
    
    // IF MESSAGE: 
    if (type == MESSAGE_ENTRY_TYPE) {
        // STEP 1: Check if the sender and the recipient are the same, if not, then increment the count:   
        if(mb != from){
            // We need to increase the ref_count so that there is no bounce: 
            mb_ref(from, "mb_add_message() call.");
        }
        
        // Set the type: 
        new_entry -> type = MESSAGE_ENTRY_TYPE;
        // STEP 3: Set the new_message: 
        //MESSAGE *new_message = (void *)malloc(sizeof(MESSAGE)); // TODO: Where are we freeeing the message? 
        
        
        MESSAGE new_message; 
        new_message.msgid = msgid; 
        new_message.from = from; 
        new_message.body = body; 
        new_message.length = length; 
        new_entry -> content.message = new_message; // TODO: is it right? 
    
    } else if (type == NOTICE_ENTRY_TYPE) {
        new_entry -> type = MESSAGE_ENTRY_TYPE;
        
        NOTICE new_message; 
        new_message.msgid = msgid; 
        new_message.type = ntype; 
        new_entry -> content.notice = new_message; // TODO: is it right? 
    }
    // STEP 4: Set the messsage field of new_entry: 
     
    // STEP 5: Initialize a new mailbox_node: // TODO: Is not working chekc this later: 
    struct mailbox_node *new_node = (void *)malloc(sizeof(MAILBOX_NODE));
    new_node -> entry = new_entry; 
    // check if the queue is empy: 
    if (mb -> front == NULL && mb -> rear == NULL ){
        // create the first entry to be a dummy node: 
        //MAILBOX_NODE *dummy_node = (void *)malloc(sizeof(MAILBOX_NODE)); // TODO: Need to free it later no? 
        //dummy_node -> entry = NULL;
        //dummy_node -> next = new_node;
        //dummy_node -> prev = NULL;  // dummy node doesn't have prev
        // then make it to be the first node: 
       // mb -> front = dummy_node; // create a dummy node to be a front that will point to the first entry !!!!!!!!!!!!!!!!!!!!
        //mb -> rear = new_node;  
        //mb -> rear -> prev = mb -> front; 
        //mb -> rear -> next = NULL; // rear node doesn't have next
        
        // way too many problems with a dummy node: 
        
        mb -> front = new_node; 
        mb -> front ->  prev = NULL; 
        mb -> rear = new_node;
        
    }
    else{
        MAILBOX_NODE * prev_rear = mb -> rear; 
        prev_rear -> next = new_node; 
        new_node -> prev = prev_rear; 
        mb -> rear = new_node;  
    }
    // STEP 6: Now insert the message to the mailbox queue in the back: 
    struct mailbox_node *temp = mb -> rear; 
    mb -> rear = new_node;
    temp -> next = new_node; 
    new_node -> prev = temp; // TODO: I need to make sure I didn't allocate memory for the rear and front nodes as they are just pointers
    new_node -> next = NULL;    
    // STEP 7: Unlock the mutexes:         
    V(&mb -> entries);       
    V(&mb -> mutex);               
}

/*
// Method to add a message to the rear of the mailbox queue:
void mb_add_message(MAILBOX *mb, int msgid, MAILBOX *from, void *body, int length){
    
    mb_add_entry(mb, MESSAGE_ENTRY_TYPE, 0, msgid, from, length, body);             
}
// Method to add a notice to the rear of the mailbox queue:
void mb_add_notice(MAILBOX *mb, NOTICE_TYPE ntype, int msgid){
   
    mb_add_entry(mb, NOTICE_ENTRY_TYPE, ntype, msgid, NULL, 0, NULL);  
}
*/

// Method to add a message to the rear of the mailbox queue:
void mb_add_message(MAILBOX *mb, int msgid, MAILBOX *from, void *body, int length){
    // Block the mutex: 
    P(&mb -> mutex);
    // STEP 1: Check if the sender and the recipient are the same, if not, then increment the count: 
    if(mb != from){
        // We need to increase the ref_count so that there is no bounce: 
        mb_ref(from, "mb_add_message() call.");
    }
    // STEP 2: Allocate memory for new entry: 
    MAILBOX_ENTRY *new_entry =(void *)malloc(sizeof(MAILBOX_ENTRY));
    // Set the type: 
    new_entry -> type = MESSAGE_ENTRY_TYPE;
    // STEP 3: Set the new_message: 
    //MESSAGE *new_message = (void *)malloc(sizeof(MESSAGE)); // TODO: Where are we freeeing the message? 
    MESSAGE new_message; 
    new_message.msgid = msgid; 
    new_message.from = from; 
    new_message.body = body; 
    new_message.length = length; 
    // STEP 4: Set the messsage field of new_entry: 
    new_entry -> content.message = new_message; // TODO: is it right? 
    
    // STEP 5: Initialize a new mailbox_node: // TODO: Is not working chekc this later: 
    struct mailbox_node *new_node = (void *)malloc(sizeof(MAILBOX_NODE));
    new_node -> entry = new_entry; 
    // check if the queue is empy: 
    if (mb -> front == NULL && mb -> rear == NULL ){
        // create the first entry to be a dummy node: 
        //MAILBOX_NODE *dummy_node = (void *)malloc(sizeof(MAILBOX_NODE)); // TODO: Need to free it later no? 
        //dummy_node -> entry = NULL;
        //dummy_node -> next = new_node;
        //dummy_node -> prev = NULL;  // dummy node doesn't have prev
        // then make it to be the first node: 
       // mb -> front = dummy_node; // create a dummy node to be a front that will point to the first entry !!!!!!!!!!!!!!!!!!!!
        //mb -> rear = new_node;  
        //mb -> rear -> prev = mb -> front; 
        //mb -> rear -> next = NULL; // rear node doesn't have next
        
        // way too many problems with a dummy node: 
        
        mb -> front = new_node; 
        mb -> front ->  prev = NULL; 
        mb -> rear = new_node;
        
    }
    else{
        MAILBOX_NODE * prev_rear = mb -> rear; 
        prev_rear -> next = new_node; 
        new_node -> prev = prev_rear; 
        mb -> rear = new_node;  
    }
    // STEP 6: Now insert the message to the mailbox queue in the back: 
    struct mailbox_node *temp = mb -> rear; 
    mb -> rear = new_node;
    temp -> next = new_node; 
    new_node -> prev = temp; // TODO: I need to make sure I didn't allocate memory for the rear and front nodes as they are just pointers
    new_node -> next = NULL;    
    // STEP 7: Unlock the mutexes:         
    V(&mb -> entries);       
    V(&mb -> mutex);               
}
// Method to add a notice to the rear of the mailbox queue:
void mb_add_notice(MAILBOX *mb, NOTICE_TYPE ntype, int msgid){
    // Block the mutex: 
    P(&mb -> mutex);
    
    // STEP 2: Allocate memory for new entry: 
    MAILBOX_ENTRY *new_entry =(void *)malloc(sizeof(MAILBOX_ENTRY));
    // Set the type: 
    new_entry -> type = NOTICE_ENTRY_TYPE;
    // STEP 3: Set the new_message: 
    //MESSAGE *new_message = (void *)malloc(sizeof(MESSAGE)); // TODO: Where are we freeeing the message? 
    NOTICE new_message; 
    new_message.msgid = msgid; 
    new_message.type = ntype;  
    // STEP 4: Set the messsage field of new_entry: 
    new_entry -> content.notice = new_message; // TODO: is it right? 
    
    // STEP 5: Initialize a new mailbox_node: // TODO: Is not working chekc this later: 
    struct mailbox_node *new_node = (void *)malloc(sizeof(MAILBOX_NODE));
    new_node -> entry = new_entry; 
    // check if the queue is empy: 
    if (mb -> front == NULL && mb -> rear == NULL ){
        // create the first entry to be a dummy node: 
        //MAILBOX_NODE *dummy_node = (void *)malloc(sizeof(MAILBOX_NODE)); // TODO: Need to free it later no? 
        //dummy_node -> entry = NULL;
        //dummy_node -> next = new_node;
        //dummy_node -> prev = NULL;  // dummy node doesn't have prev
        // then make it to be the first node: 
       // mb -> front = dummy_node; // create a dummy node to be a front that will point to the first entry !!!!!!!!!!!!!!!!!!!!
        //mb -> rear = new_node;  
        //mb -> rear -> prev = mb -> front; 
        //mb -> rear -> next = NULL; // rear node doesn't have next
        
        // way too many problems with a dummy node: 
        
        mb -> front = new_node; 
        mb -> front ->  prev = NULL; 
        mb -> rear = new_node;
        
    }
    else{
        MAILBOX_NODE * prev_rear = mb -> rear; 
        prev_rear -> next = new_node; 
        new_node -> prev = prev_rear; 
        mb -> rear = new_node;  
    }
    // STEP 6: Now insert the message to the mailbox queue in the back: 
    struct mailbox_node *temp = mb -> rear; 
    mb -> rear = new_node;
    temp -> next = new_node; 
    new_node -> prev = temp; // TODO: I need to make sure I didn't allocate memory for the rear and front nodes as they are just pointers
    new_node -> next = NULL;    
    // STEP 7: Unlock the mutexes:         
    V(&mb -> entries);       
    V(&mb -> mutex);               
}

// Method to initiate a mailbox: 
MAILBOX *mb_init(char *handle){

    // STEP 1: Allocate memory for new mailbox object:
    
    MAILBOX *new_mailbox = (void *)malloc(sizeof(MAILBOX));
     if (new_mailbox == NULL) {
        return NULL; // TODO: How to handle the memory allocation error? 
    }
    
    // STEP 2: initialize its fields:
    new_mailbox -> ref_count = 0; 
    new_mailbox -> is_defunct = 0; // not defunct; 
    new_mailbox -> front = NULL;
    new_mailbox -> rear = NULL;   
    new_mailbox -> hook = NULL; 
   
    
    // The hanlde: 
    new_mailbox -> handle = strdup(handle); 
    if (new_mailbox -> handle == NULL) {
        free(new_mailbox); // TODO: How to handle the allocation errors? 
        return NULL;
    } 
    
    
    // STEP 3.2: Initialize the mutex: 
    if (sem_init(&(new_mailbox->mutex), 0, 1) != 0) {
        free(new_mailbox->handle); 
        free(new_mailbox); 
        return NULL;
    }
    
    Sem_init(&new_mailbox -> entries, 0, 0); // could just do that with the mutex too
    
    // STEP 3: mb_ref()
    // Increase the ref_count of the new mailbox:
    mb_ref(new_mailbox, "mb_init() call.");
    
    // STEP 4: return new_mailbox:    
    return new_mailbox; 
}

// Method to get the front entry of mailbox:
MAILBOX_ENTRY *mb_next_entry(MAILBOX *mb) {

    // Block the mailbox's mutex: 
    P(&mb -> entries); // TODO: Should I unblock it right here in this method? 
    P(&mb -> mutex);
    
    // STEP 1: check f the mailbox is defunct:    
    if (mb -> is_defunct){
        // Unblock the mutex: 
        V(&mb -> mutex);
        return NULL; 
    }

    // If the queue is empty return NULL:
    if (mb -> front == NULL) {
        // Unlock the mutex: 
        //V(&mb -> entries); // TODO: Why it doesnt work if I unblock it here? Where should i unblock it then? 
        V(&mb -> mutex); 
        return NULL;
    }
    // STEP 2: Get the pointer to the front entry in the mailbox
    // MAILBOX_NODE *new_front = mb -> front -> next; 
    MAILBOX_NODE *front_node = mb -> front;
    MAILBOX_ENTRY *entry_to_return = front_node -> entry;
    
    // Should I actually free this node? 
    //free(new_front -> prev);

     // STEP 3: Make the mb -> front point now to the mb -> front -> next:
    // TODO: Make sure I need this check here: 
    // Check if there is no nodes left: 
    if (mb -> front == mb -> rear) {    
        mb -> front = NULL;
        mb -> rear = NULL;
    
    } 
    // if there are still items i the queue: // TODO: DOn't I take care of  it in the add message and add notice? 
    if (mb -> front != mb -> rear){
        mb -> front = mb -> front -> next;
        mb -> front -> prev = NULL; 
    }
    // Decrement the reference count of the sender if it's a message entry
    // STEP 4: decrement the from if the message: 
     if(entry_to_return -> type == MESSAGE_ENTRY_TYPE){
        MAILBOX *from = entry_to_return -> content.message.from;
        if (mb != from)
            mb_unref(from, "mb_next_entry() call."); 
     } 
    // Free the front node:
    free(front_node);
    // Unolck the mutex:
    V(&mb -> mutex); // Unlock the mutex 
    //V(&mb -> entries); // TODO: Why it doesnt work if I unblock it here? Where should i unblock it then? 
    // STEP 5: return the entry: 
    return entry_to_return;
}



