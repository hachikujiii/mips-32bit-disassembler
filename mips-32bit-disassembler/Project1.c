#include <stdio.h>

unsigned int instructions[] = {0x032BA020, 0x8CE90014, 0x12A90003, 0x022DA822, 0xADB30020, 
0x02697824, 0xAE8FFFF4, 0x018C6020, 0x02A4A825, 0x158FFFF7, 0x8ECDFFF0};

unsigned int opcodemask = 0xFC000000;                          //11111100000000000000000000000000
unsigned int source1mask = 0x03E00000;                         //00000011111000000000000000000000
unsigned int source2mask = 0x001F0000, idestmask = 0x001F0000; //00000000000111110000000000000000
unsigned int rdestmask = 0x0000F800;                           //00000000000000001111100000000000
unsigned int functionmask = 0x0000003F;                        //00000000000000000000000000111111
short offsetmask = 0xFFFF;                                                     //1111111111111111

typedef struct {
    unsigned int number;
    char* name;
} Operation;

Operation operations[] = {
    {0x20, "add"}, 
    {0x22, "sub"},
    {0x24, "and"}, 
    {0x25, "or"},
    {0x2A, "slt"},

    {0x23, "lw"}, 
    {0x2B, "sw"}, 
    {0x04, "beq"},
    {0x05, "bne"}
};

unsigned int programcounter = 0x9A040; //starting address for instructions
int instruction_count = sizeof(instructions) / sizeof(unsigned int); 
int operation_count = sizeof(operations) / sizeof(Operation);
char* find_opname(unsigned int function_number);

int main(void) {

    for(int i = 0; i < instruction_count; i++) {

        unsigned int opcode = (instructions[i] & opcodemask) >> 26; //mask then shift to least significant bit

        if (opcode == 0) { //handle R-format cases

            unsigned int src1 = (instructions[i] & source1mask) >> 21; 
            unsigned int src2 = (instructions[i] & source2mask) >> 16; 
            unsigned int dest = (instructions[i] & rdestmask) >> 11; 
            unsigned int function = (instructions[i] & functionmask);
            char* fname = find_opname(function);

            printf("%x  %-3s  $%d, $%d, $%d\n", programcounter, fname, dest, src1, src2);

            programcounter += 4; //move to next instruction's address

        } else { //handle I-format cases
            
            if (opcode == 0x23 || opcode == 0x2B) { //handle load/store word

                unsigned int src = (instructions[i] & source1mask) >> 21; 
                unsigned int dest = (instructions[i] & idestmask) >> 16;
                short offset = (instructions[i] & offsetmask);
                char* opcodename = find_opname(opcode);

                printf("%x  %-3s  $%d, %d($%d)\n", programcounter, opcodename, dest, offset, src);

                programcounter += 4;

            } else if (opcode == 0x04 || opcode == 0x05) { //handle branches

                unsigned int src1 = (instructions[i] & source1mask) >> 21;
                unsigned int src2 = (instructions[i] & source2mask) >> 16;
                short offset = (instructions[i] & offsetmask);

                unsigned int branchaddress = (offset << 2) + (programcounter + 4); //left shift 2 then add to next instruction's address

                char* opcodename = find_opname(opcode);

                printf("%x  %s  $%d, $%d, address %x\n", programcounter, opcodename, src1, src2, branchaddress);

                programcounter += 4;
            }
        }
    }
}

char* find_opname(unsigned int number) { //takes an opcode/function number and returns its name

    for(int i = 0; i < operation_count; i++) {
        
        if(operations[i].number == number) {
            return operations[i].name;
        }
    }
}