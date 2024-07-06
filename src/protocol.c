#include "protocol.h"
#include <unistd.h>
#include <stdio.h>
#include "debug.h"
#include <stdlib.h>

// Function to send a packet over a given file descriptor
int send_packet(int fd, CHLA_PACKET_HEADER *hdr, void *payload) {

    // STEP 1: Check if the hdr and fd are okay:
    if(hdr == NULL || fd < 0)  // no hdr or invalid fd
        return -1;
    
    // STEP 2: Write the header first: 
    // Total bytes written:
    int total_written = 0;
    // the size of the hdr we need to write: 
    int hdr_bytes = sizeof(CHLA_PACKET_HEADER);

    // STEP 2.2: Loop until the entire header is written:
    while(total_written != hdr_bytes) {
        int bytes_written = write(fd, (void *)hdr + total_written, hdr_bytes - total_written);
        // Check for errors in writing:
        if(bytes_written == -1)  // error
            return -1;
        total_written = total_written +  bytes_written;  
    }
    
    // STEP 3: Then write the payload: 
    // If payload exists, send it:
    if(payload) {
        // Get the payload length from the header:
        int payload_length = ntohs(hdr -> payload_length); // TODO: Do we use the ntohs????
        // Total payload bytes written:
        int total_payload_written = 0;

        // STEP 3.2 Loop until the entire payload is written:
        while(total_payload_written != payload_length) {
            int bytes_written = write(fd, (char *)payload + total_payload_written, payload_length - total_payload_written);
            if(bytes_written == -1)  // error
                return -1;
            
            total_payload_written += bytes_written;
        }
    }

    // Return success:
    return 0;
}

// Function to receive a packet from a given file descriptor
int receive_packet(int fd, CHLA_PACKET_HEADER *hdr, void **payload) {
    // STEP 1: Check if file descriptor, header, and payload are valid:
    if(fd < 0 || hdr == NULL || payload == NULL)  // fd is okay, paylload and hrd exist
        return -1;
    
    // STEP 2: Read the header:
    // Total bytes read:
    int total_read = 0;
    // Header size:
    int hdr_bytes = sizeof(CHLA_PACKET_HEADER);
    // Temporary header to store received header:
    CHLA_PACKET_HEADER tmp_hdr;

    // Loop until the entire header is read:
    while(total_read != hdr_bytes) {
        int bytes_read = read(fd, (void *)&tmp_hdr + total_read, hdr_bytes - total_read);
        if(bytes_read <= 0)  // EOF or error
            return -1;       
        total_read += bytes_read;        
    }
    // Copy the received header to the provided header pointer:
    *hdr = tmp_hdr;   
    // STEP 3: Read the payloud:
    int payload_length = ntohs(tmp_hdr.payload_length); // TODO: ntohs?

    if(payload_length > 0) {
        // Payload buffer:
        char *buffer = malloc(payload_length + 1);
        if(buffer == NULL)  // malloc error
            return -1;
        // nULL terminate the buffer:
        buffer[payload_length] = '\0';
        int total_payload_read = 0;

        // Loop until the entire payload is read:
        while(total_payload_read != payload_length) {
            
            int bytes_read = read(fd, buffer + total_payload_read, payload_length - total_payload_read);
            
            if(bytes_read <= 0) {
                free(buffer);
                return -1;
            }            
            total_payload_read = total_payload_read +  bytes_read;
        }
        *payload = buffer;
    }

    // Return success:
    return 0;
}

