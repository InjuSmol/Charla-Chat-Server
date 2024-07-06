#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <unistd.h> // getopt


#include "debug.h"
#include "server.h"
#include "globals.h"

#include "csapp.h" // --> P/V


volatile sig_atomic_t quit = 0;

void sighup_handler(int sig) {
    quit = 1;
}

static void terminate(int);

/*
 * "Charla" chat server.
 *
 * Usage: charla <port>
 */
int main(int argc, char* argv[]){ 
   //printf("herere %s\n", "herererer");

    int port = -1;
    
// STEP 1: Obtain the port number to be used by the server from the command line arguments: -p <port> 

    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                if (port < 1024) {
                    fprintf(stderr, "Invalid port number (must be1024 or above)\n"); // TODO: Are we allowed to have fprints? 
                    
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Usage: %s -p <port>\n", argv[0]); // TODO: Are we allowed to have fprints? 
                exit(EXIT_FAILURE);
        }
    } 
    if (argc != 3) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]); // TODO: Are we allowed to have fprints? 
        exit(EXIT_FAILURE);
    }
    if (port == -1) {
        fprintf(stderr, "Port number not specified.\n"); // TODO: Are we allowed to have fprints? 
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]); // TODO: Are we allowed to have fprints? 
        exit(EXIT_FAILURE);
    }
    
    //printf("port number: %d\n", port);
      
// STEP 2: Initiate the user_registry and client_registery models (store references to them in global variables): 
    
    // Perform required initializations of the client_registry and
    // player_registry.
    user_registry = ureg_init(); // in global.h !!!!!
    client_registry = creg_init();
    
// STEP 3: Set up a socket to listen for incoming connection requests on the <port> 

    int listenfd, *connfdp; // listening descriptor and connected descriptor (after the connection is established -- returned from accept )
    socklen_t clientlen; // 
    struct sockaddr_storage clientaddr; // storage for the client 
    pthread_t tid; // storing thread TID
    
    
    // SIGHUP signal handling: 
    
    struct sigaction sa;
    sa.sa_handler = sighup_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGHUP, &sa, NULL);
    
    char port_str[6]; // Assuming port numbers are within 0-65535 range
    snprintf(port_str, sizeof(port_str), "%d", port);
    // Open a listening socket on the specified port from command line argument:
    listenfd = Open_listenfd(port_str); 
   
    //sbuf_init(&sbuf, SBUFSIZE);  // initialize a shared buffer of connected descriptors
    
    //for (i = 0; i < NTHREADS; i++)  // create worker threads to handle connections
    //    Pthread_create(&tid, NULL, thread, NULL);
        
    // Continuously accept incoming connections     
    while (!quit) { // the loop to listen to listen for connection requests: 
        clientlen = sizeof(struct sockaddr_storage); // get the size of the client address
       if ((connfdp = malloc(sizeof(int))) == NULL) { // malloc of connected descriptior is necessary to avoid deadly race
            perror("malloc error");
            Close(listenfd);
            terminate(EXIT_FAILURE);
        } 
        // Accept a connection and obtain a new file descriptor for the connection
        *connfdp = accept(listenfd, (SA *)&clientaddr, &clientlen);
        if (quit) 
            break;
        // Insert the connection file descriptor into the shared buffer
        Pthread_create(&tid, NULL, chla_client_service,  (void*)connfdp); /// what is thread here
     } 
    
// STEP 4: Install a SIGHUP handler (so that clean termination of the server can be achieved by sending it a SIGHUP ----> use sigaction(), not  signal()):
    
// STEP 5: Enter a loop to accept connection requests for each connection a thread should be started to run function chla_client_service(): 
 

    //fprintf(stderr, "You have to finish implementing main() "
	//    "before the server will function.\n");
	
	
	// close the listening descriptor: 
    Close(listenfd);
     
    terminate(EXIT_SUCCESS);

}


/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shut down all existing client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    // Finalize modules.
    creg_fini(client_registry);
    ureg_fini(user_registry);

    debug("%ld: Server terminating", pthread_self());
    exit(status);
}
