#include "globals.h"
#include "csapp.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <semaphore.h> 
#include <debug.h>


#include "client.h"
#include "helper.h"

#include <pthread.h>
//#include "client_registry.h"
//

// Client object consist of 
// -- File Descriptor       ---> set when the client object is created and doesn't change
// -- Ref to USER (if logged in)    ----> Can change asynchronysly if user log in or out    ===> need to avoid races !!!!!
// -- Ref to MAILBOX ( if logged in)     ----> Can change asynchronysly if user log in or out  ===> need to avoid races !!!!!

//typedef struct client_registry CLIENT_REGISTRY;

typedef struct client {
    sem_t mutex;
    int fd; // file dscriptor of a socket in which to communticate with the client
    MAILBOX *mailbox; // mailbox  ---> can change
    USER *user; // user ---> can change    
    int ref_count; // reference count
    int logged_in; // state --> 0 - logged out, 1 - logged in
}CLIENT;

// helper method prototype
int send_ack_or_nack(CLIENT *client, uint32_t msgid, void *data, size_t datalen, CHLA_PACKET_TYPE type);

// Method to create a new client: 
CLIENT *client_create(CLIENT_REGISTRY *creg, int fd){ // TODO: HELP WHAT I AM DOING WITH THE CREG HERE!!? ???
    
    // STEP 1: Check if the client already exists in the creg: 
    // not sure we can do that for some reason, but i guess one same user can have many client sessions
    // STEP 2: create a new CLIENT object: 
    CLIENT *new_client = (void *)malloc(sizeof(CLIENT));
    if(new_client == NULL){
        fprintf(stderr, "%s\n", "Memory allocation failed"); // TODO: can we do that?  
        return NULL;
    }
           
    // intialize the new client object: 
    new_client -> fd = fd; 
    new_client -> logged_in = 0; // not loggged in  
    if(sem_init(&new_client -> mutex, 0, 1) == -1)
    {
        free(new_client);
        return NULL; 
    }
    new_client -> user = NULL;  // TODO: Are we supposed to create a new user here? 
    new_client -> mailbox = NULL; // TODO: And a new mailbox here? 
    new_client -> ref_count = 0; 
    client_ref(new_client, "cilent_create"); // increase a ref count, should be 1
           
    return new_client;
}

// Method to increase a ref_count of a client: 
CLIENT *client_ref(CLIENT *client, char *why){
    REF_COUNTED_OBJECT_REF(client, why);
    return client; 
}
 
// Method to decrease a ref_count of a client: 
// If a ref_count is 0, then free the client
void client_unref(CLIENT *client, char *why){
    debug("decrease reference count for a client: %s", why); // TODO: check if it what we need to do
    // decrease the ref_count: 
   P(&client -> mutex); //TODO: is it okay to user P & V? 
    client -> ref_count --; 
    //sem_post(&client -> mutex);
    
    // Check if the ref_count now is 0, if so, thewn free the client: 
    if (client -> ref_count == 0 ){
        // free the mailbox:
        if (client -> user != NULL) 
            user_unref(client->user, "client_unref");
       
        // free the user: 
        if (client -> mailbox != NULL)
            mb_unref(client->mailbox, "client_unref");
        
        // free the client: 
        // Destroy the client's mutex:
        sem_destroy(&client->mutex);  
        free(client);   
    }
    else{
        // Unblock mutex:    
        V(&client -> mutex);
    }    
}
// TODO: FINISH THE LOGGING IN AND LOGGIN OUT !!!!!!!!!!!!!


// Static mutex for loggining in: 
static sem_t mutex;

// Thread routine for logging in: 
void thread() {
    //*mutex = (sem_t *)malloc(sizeof(sem_t));
    //if ( 
    sem_init(&mutex, 0, 1); 
    //!= 0) { // *mutex == NULL ||
        // Malloc error: 
   //    return -1;
    //}
}

// Method to log in a new client: 
int client_login(CLIENT *client, char *handle){
   
    // STEP 0.1: 
    // Static mutex for logging in: 
    // static sem_t *mutex = NULL;
       
    // Static flag for pthread_once: 
    static pthread_once_t once_control = PTHREAD_ONCE_INIT;
    // initialize mutex for logging in with pthread_one: 
    pthread_once(&once_control, thread);
    
     P(&mutex); // block the loggin in mutex

    // STEP 0.2: Check if the client is already logged in: 
    
     if(client -> logged_in){
        V(&mutex);
        return -1;
    }
    
    // STEP 0.3: Check if there is a client existed with this handle: 
    CLIENT **all_clients_list = creg_all_clients(client_registry);  
    CLIENT **list_p = all_clients_list; // pointer to the list
    //char *clients_handle = user_get_handle((*list_p) -> user);
    while (*list_p != NULL) {
    // If the client is not logged in and doesnt have the same handle: 
        if( (*list_p) -> logged_in == 1  && strcmp(handle, user_get_handle((*list_p) -> user)) == 0)
         {
            list_p = all_clients_list;
            // Need to decreament the ref_count of every client in the list: 
            while (*list_p != NULL) {     
                client_unref(*list_p, "creg_all_clients() call");
                list_p++;
            }
            free(all_clients_list);     
            V(&mutex);
            return -1;
        }
        list_p++;
    }
    
    // Need to decreament the ref_count of every client in the list: 
    list_p = all_clients_list;
    while (*list_p != NULL) {     
        client_unref(*list_p, "creg_all_clients() call");
        list_p++;
    }
    free(all_clients_list);  
    
    
    // Block the clients mutex: 
    P(&client -> mutex);
    
    // STEP 1: First, create a new user: 
    USER *new_user = ureg_register(user_registry, handle); // TODO: not sure if it is right
    
    if (new_user == NULL){
        fprintf(stderr, "%s\n", "error creating a new user");  // TODO: Do I need it here? 
        //fprintf(stdout, "%s\n", "error creating a new user");  //TODO: Remove this!!!!!!!!!
        return -1; // error creating a new user
    }
    client -> user = new_user; 
    // STEP 2: Then create a new mailbox: 
    MAILBOX * new_mailbox = mb_init(handle);   
    if (new_mailbox == NULL ){
       fprintf(stderr, "%s\n", "error creating a new mailbox");  // TODO: Do I need it here? 
       //fprintf(stdout, "%s\n", "error creating a new mailbox");  // TODO: Remove this!!!!!
       return -1; // error creating a new mailbox
    }   
    client -> mailbox = new_mailbox; 
    
    // STEP 3: no step 3, but 3 0 steps
    // STEP 4: mark the client as logged in: 
    client -> logged_in = 1; 
    
    // unblok loggin in and client mutex: 
    V(&mutex);
    V(&client -> mutex);
    return 0; // success!!!! Yay
       
}

// Mtehod to log out a user: 
int client_logout(CLIENT *client){
    // STEP 0: Block the clietns mutex: 
    P(&client -> mutex);

    // STEP 1: Chekc if the client is logged in: 
    if (client -> logged_in != 1){
        // Ublock client's mutex: 
        V(&client -> mutex);
        return -1;  // ERROR!!!!!
    } 
    // STEP 3; Unref the user: 
    user_unref(client -> user, "client_logout");
    // STEP 3.2: Unregister the user: 
    char *handle = user_get_handle(client -> user);
    ureg_unregister(user_registry, handle);     
     
    // STEP 2: Shutdown the mailbox; 
    mb_shutdown(client -> mailbox); 
    // Unref the mailbox !!!!!!!!!!!
    mb_unref(client -> mailbox, "client_loggout() call");      
    client -> mailbox = NULL; 
    
    client -> user = NULL;
    // STEP 4: logged out:    
    
    client -> logged_in = 0; // not logged in 
    
    // Unblock client's  mutex: 
    V(&client -> mutex);
    return 0; // success!!!!!
}


// Method to return USER object of the CLIENT client: 
USER *client_get_user(CLIENT *client, int no_ref){
    // First, block client's mutex: 
    P(&client -> mutex);
    // STEP 1: Check if logged out: 
    if (client -> logged_in == 0){
    // Unblock the client's mutex:
        V(&client -> mutex);
        return NULL; // TODO: Should I return a NULL here? 
    }
    USER *client_to_return = client -> user;
    // Not sure we need to do that, but check just in case if the user is NULL: // TODO: Do we need to doo that: 
    if (client_to_return == NULL){
        return NULL; 
    }
    
    // STEP 2: if no_ref is != 0, then don't increment the ref_count of the user to return: 
    // if 0 (caller takes the resposibility for decrementing the ref_count after it is done with the user object) => increment the ref_count: 
    if (no_ref){
        // Need to increment the ref_count: 
         user_ref(client_to_return, "client_get_user() call"); 
    }     
    // unblock the client's mutex:
    V(&client -> mutex);
    return client_to_return;    
}
 
// Method to  get a MAILBOX object of the CLIENT client: 
MAILBOX *client_get_mailbox(CLIENT *client, int no_ref){
    P(&client -> mutex);
    
    MAILBOX *mailbox_to_return = client -> mailbox;
    // First, check if logged out and if the client -> mailbox is zero: 
    if ( client -> mailbox == NULL || client -> logged_in == 0){
        P(&client -> mutex);
        return NULL;
    }
    if (no_ref == 0)
        mb_ref(mailbox_to_return, "client_get_mailbox() call.");           
    V(&client -> mutex);
    return mailbox_to_return; 
}

// method to get a file desacriptor of the client: 
int client_get_fd(CLIENT *client){
    return client -> fd; // TODO: we just return the fd bc it doesnt change, right 
}
 
// Method to send a packet: 
int client_send_packet(CLIENT *user, CHLA_PACKET_HEADER *pkt, void *data){
    P(&user -> mutex);
    // Need to obtain an exclusive access to the network connection for the duration of this operation --> How? 
    // STEP 1: Should check if the client is logged in:  ---> why is it called a user, a trap ? 
    if (user -> logged_in == 0 || user == NULL){
        return -1; // TODO: Verify this moment, not sure about that
    }   
    // STEP 2: Create a new header: and send it and the packet:    
    int to_return = proto_send_packet(user -> fd, pkt, data);
    V(&user -> mutex);
    return to_return; // depends on the proto_send_packet
}

// Method to send ACK packet: 
int send_ack_or_nack(CLIENT *client, uint32_t msgid, void *data, size_t datalen, CHLA_PACKET_TYPE type){
    // STEP 1: first we need to create the packet
    CHLA_PACKET_HEADER *new_packet = (void *)malloc(sizeof(CHLA_PACKET_HEADER));
    
    // STEP 2: msgid: 
    new_packet -> msgid = htonl(msgid);
    
    // STEP 3: payload_lenght: 
    if (type == CHLA_ACK_PKT)
        new_packet -> payload_length = htonl(datalen);
    
    // STEP 4: type: 
    new_packet -> type = CHLA_ACK_PKT; // For ACK   
    
    // STEP 5: timestamps:
    // Get a system time: 
    struct timespec system_time;
    clock_gettime(CLOCK_REALTIME, &system_time);
    // Seconds field of time packet was sent:
    uint32_t seconds = system_time.tv_sec;  // sec for seconds
    new_packet -> timestamp_sec = htonl(seconds);
    // Nanoseconds field of time packet was sent:
    uint32_t nanoseconds = system_time.tv_nsec;   // nsec for nanoseconds
    new_packet -> timestamp_nsec = htonl(nanoseconds); 
    int to_return = 0; 
    // STEP 6: Now send the packet: 
    if (type == CHLA_ACK_PKT)
    
        to_return = client_send_packet(client, new_packet, data);
    else if (type == CHLA_NACK_PKT)
        to_return = client_send_packet(client, new_packet, NULL);
    // STEP 7: Free the packet: 
    free(new_packet);  
    return to_return; 
}

// Helper m ethod to send a packet: 
int client_send_ack(CLIENT *client, uint32_t msgid, void *data, size_t datalen){
    return send_ack_or_nack(client, msgid,data, datalen, CHLA_ACK_PKT); 
}

// Method to send NACK packet: 
int client_send_nack(CLIENT *client, uint32_t msgid){
   return send_ack_or_nack(client, msgid,NULL, 0, CHLA_NACK_PKT);
}

