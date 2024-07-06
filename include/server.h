/**
 * === DO NOT MODIFY THIS FILE ===
 * If you need some other prototypes or constants in a header, please put them
 * in another header file.
 *
 * When we grade, we will be replacing this file with our own copy.
 * You have been warned.
 * === DO NOT MODIFY THIS FILE ===
 */
#ifndef SERVER_H
#define SERVER_H

/*
 * Thread function for the thread that handles client requests.
 *
 * The arg pointer points to the file descriptor of client connection.
 * This pointer must be freed after the file descriptor has been retrieved.
 */
void *chla_client_service(void *arg);

/*
 * Function run by the thread servicing the mailbox of a logged-in client.
 * It repeatedly uses `mb_next_entry()` to wait for and retrieve the
 * next entry arriving in the mailbox, and then it sends the corresponding
 * message or notice to the client.  Note that this function is not
 * referenced outside of the server module, so it need not have been
 * mentioned in this header file (and it should have been declared as
 * "static"), but it is mentioned anyway so that you know this is how
 * the implementation is supposed to be structured.
 */
void *chla_mailbox_service(void *arg);

#endif
