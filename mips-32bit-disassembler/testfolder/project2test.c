#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>

#define BLOCK_SIZE 16
#define NUM_OF_SLOTS 16
#define MEM_SIZE 2048
#define BUFFER_SIZE 1000

typedef struct {
    unsigned short slot_num;
    bool valid;
    bool dirty;
    unsigned char tag;
    unsigned char data[BLOCK_SIZE];
} Slot;

Slot cache[NUM_OF_SLOTS];
unsigned short main_memory[MEM_SIZE];
char request_buffer[BUFFER_SIZE];

void file_to_buffer(FILE *ptr);
char* process_address(char *cursor, unsigned short *address, unsigned short *block_offset,
                unsigned short *block_index, unsigned short *tag, unsigned short *slot_num);
void handle_miss(Slot *cache, unsigned short *main_memory, unsigned short address, unsigned short slot_num, 
                unsigned short block_index, unsigned short tag, unsigned short block_offset, unsigned short write_value, bool write);                
void displaycache();


int main(int argc, char *argv[]) {

    //check for command line arguments
    if(argc != 2) {
        printf("Usage: ./test inputfile\n");
        return 1;
    }

    //open operations txt file
    FILE *inputfile = fopen(argv[1], "r"); 
    if (inputfile == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    file_to_buffer(inputfile);
    fclose(inputfile);

    //initialize memory
    unsigned short value = 0;
    for (int i = 0; i < MEM_SIZE; i++) {
 
        if(value > 0xFF) { //check for value reset back to 0

            value = 0;
        }

        main_memory[i] = value;
        value++;
    }

    //initialize cache
    for (int i = 0; i < NUM_OF_SLOTS; i++) {

        cache[i].slot_num = i;
        cache[i].valid = false;
        cache[i].dirty = false;
        cache[i].tag = 0;
        
        for (int j = 0; j < BLOCK_SIZE; j++) { //fill data blocks with 0

            cache[i].data[j] = 0;
        }
            
    }


    unsigned short address, block_offset, block_index, tag, slot_num, write_value;

    char *cursor = request_buffer;
    while (*cursor != '\0') { //iterate through entire buffer

        if(*cursor == '\n' || *cursor == ' ') {

            cursor++;
            continue;
        }

        if(*cursor == 'R') {
            
            cursor = process_address(cursor, &address, &block_offset, &block_index, &tag, &slot_num); //get cache fields

            if(cache[slot_num].valid && cache[slot_num].tag == tag) { //cache hit

                printf("At address %X there is the value %X (Cache hit)\n", address, cache[slot_num].data[block_offset]);
                continue;

            } else { //cache miss

                handle_miss(cache, main_memory, address, slot_num, block_index, tag, block_offset, 0, false);
            }
        }

        if(*cursor == 'W') {
            
            cursor = process_address(cursor, &address, &block_offset, &block_index, &tag, &slot_num); // get cache fields
            
            write_value = (unsigned short)strtol(cursor, &cursor, 16); //get additional value from string buffer
            
            if(cache[slot_num].valid && cache[slot_num].tag == tag) { //cache hit

                cache[slot_num].data[block_offset] = write_value;
                cache[slot_num].dirty = true;

                printf("The value %X has been written to address %X (Cache hit)\n", write_value, address);
                continue;
                
            } else { //cache miss
                
                handle_miss(cache, main_memory, address, slot_num, block_index, tag, block_offset, write_value, true);
            }
        }

        if(*cursor == 'D') {

            displaycache();
            cursor++;
        }
    }
}

void file_to_buffer(FILE *ptr) {

    char c;
    int buffer_index = 0;
    while (fread(&c, sizeof(char), 1, ptr)) { //read entire file into buffer array

    request_buffer[buffer_index] = c;
    buffer_index++;
    }
}

char* process_address(char *cursor, unsigned short *address, unsigned short *block_offset,
                unsigned short *block_index, unsigned short *tag, unsigned short *slot_num) {

    cursor++;
    *address = (unsigned short)strtol(cursor, &cursor, 16); //returns hex value from string buffer

    *block_offset = *address & 0x000F;
    *block_index = *address & 0xFFF0;
    *tag = *address >> 8;
    *slot_num = (*address & 0x00F0) >> 4;

    return cursor;
}

void handle_miss(Slot *cache, unsigned short *main_memory, unsigned short address, unsigned short slot_num, 
                unsigned short block_index, unsigned short tag, unsigned short block_offset, unsigned short write_value, bool write) {
    
    int write_address;

    if (cache[slot_num].valid && cache[slot_num].dirty) { 

        //get block index from dirty block in cache
        write_address = (((cache[slot_num].tag << 4) + slot_num) << 4); 

        //update MM with dirty cache block
        for(int i = 0; i < BLOCK_SIZE; i++) {

            main_memory[write_address + i] = cache[slot_num].data[i]; 
        }
        cache[slot_num].dirty = false;
    }

    //bring in requested block to cache
    for(int i = 0; i < BLOCK_SIZE; i++) {
                    
        cache[slot_num].data[i] = main_memory[block_index + i]; 
    }

    cache[slot_num].tag = tag;
    cache[slot_num].valid = true;

    if(write) {

        cache[slot_num].data[block_offset] = write_value;
        cache[slot_num].dirty = true;
        printf("The value %X has been written to address %X (Cache miss)\n", write_value, address);
    } else {

        printf("At address %X there is the value %X (Cache miss)\n", address, cache[slot_num].data[block_offset]);
    }
     
}


void displaycache() {

printf("Slot Valid Dirty Tag    Data\n");

    for (int i = 0; i < NUM_OF_SLOTS; i++) {

        printf("%2X %5d %5d %4X     ", cache[i].slot_num, cache[i].valid, cache[i].dirty, cache[i].tag);
        
        for (int j = 0; j < BLOCK_SIZE; j++) {

            printf("%02X ", cache[i].data[j]);
        }
        
        printf("\n");
    }

}


