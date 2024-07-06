/**
 * === DO NOT MODIFY THIS FILE ===
 * If you need some other prototypes or constants in a header, please put them
 * in another header file.
 *
 * When we grade, we will be replacing this file with our own copy.
 * You have been warned.
 * === DO NOT MODIFY THIS FILE ===
 */
#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdio.h>
#include <stdlib.h>

/*
 * A mailbox is a queue that contains two types of entries:
 * "messages" and "notices".
 */
typedef struct mailbox MAILBOX;

/*
 * A message is a user-generated transmission from one client to another.
 * it contains a message ID that uniquely identifies the message,
 * the mailbox of the sender of the message, a body that may consist of
 * arbitary data, and a length field that specifies the number of bytes
 * of data in the body.
 */
typedef struct message {
    int msgid;
    MAILBOX *from;
    void *body;
    int length;
} MESSAGE;

/*
 * A notice is a server-generated transmission that informs a client
 * about the disposition of a previously sent message.  A notice contains
 * a type field and the message ID of the message to which it pertains.
 *
 * A notice can be an ACK notice, which positively acknowledges a
 * previous client request, a NACK notice, which negatively
 * acknowledges a previous client request, a bounce notice, which
 * informs a client that a previously sent message was not delivered,
 * or a delivery notice (return receipt), which informs a client that
 * a previously sent message has been delivered to its intended
 * recipient.
 *
 * The reason a message contains information about the sender is so that
 * a notice concerning the disposition of the message can be sent back to
 * the sender when the message is either delivered or bounced.
 * A message contains the mailbox of the sender, rather than just the
 * sender's handle, to avoid the need to make multiple copies of the handle,
 * each of which would require allocation, and, ultimately, freeing.
 * In order to be sure that the mailbox referred to by a message is
 * guaranteed not to have been deallocated as long as the message still
 * exists, when a message is constructed, the reference count of the
 * sender's mailbox is incremented to account for the pointer stored in
 * the message.  When the message is ultimately discarded, it is the
 * responsibility of whomever discards it to decrease the reference count
 * of the mailbox to account for the pointer to it that is being destroyed.
 */
typedef enum {
    NO_NOTICE_TYPE,
    BOUNCE_NOTICE_TYPE,
    RRCPT_NOTICE_TYPE
} NOTICE_TYPE;

typedef struct notice {
    NOTICE_TYPE type;
    int msgid;
} NOTICE;

/*
 * An entry in a mailbox can be either a message entry or a notice entry.
 */
typedef enum { MESSAGE_ENTRY_TYPE, NOTICE_ENTRY_TYPE } MAILBOX_ENTRY_TYPE;

typedef struct mailbox_entry {
    MAILBOX_ENTRY_TYPE type;
    union {
	MESSAGE message;
	NOTICE notice;
    } content;
} MAILBOX_ENTRY;

/*
 * A mailbox provides the ability for its client to set a hook to be
 * called when an undelivered entry in the mailbox is discarded.
 * Discarding entries occurs when the mailbox has become defunct as a
 * result of a call to mb_shutdown(), but the mailbox still contains
 * undelivered entries.  This would typically occur when the client
 * associated with the mailbox has logged out and closed its network
 * connection before the messages could be delivered.
 * Once this has happened, attempts to add further entries to the
 * defunct mailbox are rejected.  In addition, a call to mb_next_entry()
 * will no longer block the calling thread, but will instead dispose
 * of the remaining messages in the mailbox in succession.  As each entry
 * is removed, it will be passed to the mailbox discard_hook function,
 * if any.  For example, the discard hook might arrange for the sender of
 * an undelivered message to receive a bounce notification.
 * The discard hook is not responsible for actually deallocating the
 * mailbox entry; that is handled within mb_next_entry() after the
 * discard hook has been called.  Once all the remaining entries have
 * been drained from the defunct mailbox, mb_next_entry() will return
 * NULL to alert the caller that the mailbox is no longer useful.
 * 
 * There is one special case that the discard hook must handle, and that
 * is the case that the "from" field of a message is NULL.  This will only
 * occur in a mailbox entry passed to the discard hook, and it indicates
 * that the sender of the message was in fact the mailbox that is now defunct.
 * Since that mailbox can no longer be used to receive any notification
 * (and in fact attempting to do so will result in a deadlock trying to
 * lock the mailbox twice) it does not make sense to do anything further
 * with it, so it has been replaced by NULL.
 *
 * The following is the type of a discard hook function.
 */
typedef void (MAILBOX_DISCARD_HOOK)(MAILBOX_ENTRY *);

/*
 * Create a new mailbox for a given handle.  A private copy of the
 * handle is made.  The mailbox is returned with a reference count of 1.
 */
MAILBOX *mb_init(char *handle);

/*
 * Set the discard hook for a mailbox.  The hook may be NULL,
 * in which case any existing hook is removed.
 */
void mb_set_discard_hook(MAILBOX *mb, MAILBOX_DISCARD_HOOK *);

/*
 * Increase the reference count on a mailbox.
 * This must be called whenever a pointer to a mailbox is copied,
 * so that the reference count always matches the number of pointers
 * that exist to the mailbox.
 */
void mb_ref(MAILBOX *mb, char *why);

/*
 * Decrease the reference count on a mailbox.
 * This must be called whenever a pointer to a mailbox is discarded,
 * so that the reference count always matches the number of pointers
 * that exist to the mailbox.  When the reference count reaches zero,
 * the mailbox will be finalized.
 */
void mb_unref(MAILBOX *mb, char *why);

/*
 * Shut down this mailbox.
 * The mailbox is set to the "defunct" state.  A defunct mailbox cannot
 * be used to send any more messages or notices, but it continues
 * to exist until the last outstanding reference to it has been
 * discarded.  At that point, the mailbox will be finalized.
 */
void mb_shutdown(MAILBOX *mb);

/*
 * Get the handle of the user associated with a mailbox.
 * The handle is set when the mailbox is created and it does not change.
 */
char *mb_get_handle(MAILBOX *mb);

/*
 * Add a message to the end of the mailbox queue.
 *   msgid - the message ID
 *   from - the sender's mailbox
 *   body - the body of the message, which can be arbitrary data, or NULL
 *   length - number of bytes of data in the body
 *
 * The message body must have been allocated on the heap,
 * but the caller is relieved of the responsibility of ultimately
 * freeing this storage, as it will become the responsibility of
 * whomever removes this message from the mailbox.
 *
 * Unless the recipient's mailbox ("mb') is the same as the sender's
 * mailbox ("from"), this function must increment the reference count of the
 * sender's mailbox, to ensure that this mailbox persists so that it can receive
 * a notification in case the message bounces.
 *
 * An attempt to add a message to a defunct mailbox is ignored.
 */
void mb_add_message(MAILBOX *mb, int msgid, MAILBOX *from, void *body, int length);

/*
 * Add a notice to the end of the mailbox queue.
 *   ntype - the notice type
 *   msgid - the ID of the message to which the notice pertains
 *
 * An attempt to add a notice to a defunct mailbox is ignored.
 */
void mb_add_notice(MAILBOX *mb, NOTICE_TYPE ntype, int msgid);

/*
 * Remove the first entry from the mailbox, blocking until there is
 * one.  The caller assumes the responsibility of freeing the entry
 * and its body, if present.  In addition, if it is a message entry,
 * then the caller must decrease the reference count on the sender's
 * mailbox to account for the destruction of the pointer to it.
 *
 * Once the mailbox has become defunct, a thread calling this function
 * will no longer be subject to blocking.  Instead, any remaining
 * entries in the mailbox will be removed, passed to any discard
 * hook that has been set on the mailbox, and then freed, as has
 * already been discussed above.  Once there are no entries remaining
 * in the defunct mailbox, this function will return NULL to alert
 * the caller that this mailbox is no longer useful and that service
 * should be terminated.
 */
MAILBOX_ENTRY *mb_next_entry(MAILBOX *mb);

#endif
