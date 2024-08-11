#include <stdio.h>

#define RAM_SIZE 0x800000 //8MB RAM
#define ROM_SIZE 0x2000000 //32MB ROM

//$t1 = $9
//$t2 = $10

const char* register_names[] = {
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};

unsigned int instructions[] = {0x032BA020, 0x8CE90014, 0x12A90003, 0x022DA822, 0xADB30020, 
0x02697824, 0xAE8FFFF4, 0x018C6020, 0x02A4A825, 0x158FFFF7, 0x8ECDFFF0};

unsigned int opcodemask = 0xFC000000;                       //11111100000000000000000000000000
unsigned int rs_mask = 0x03E00000;                          //00000011111000000000000000000000
unsigned int rt_mask = 0x001F0000;                          //00000000000111110000000000000000
unsigned int rd_mask = 0x0000F800;                          //00000000000000001111100000000000
unsigned int shamtmask = 0x000007C0;                        //00000000000000000000011111000000
unsigned int functmask = 0x0000003F;                        //00000000000000000000000000111111
short offsetmask = 0xFFFF;                                                  //1111111111111111

typedef struct {
    unsigned int number;
    char* name;
} Operation;


//virtual memory map
unsigned int virtual_mem[RAM_SIZE];


//NEC VR4300 , nearly identical to MIPSIII

//need to handle with specific conditions - BCzFL, BCzT, BCzTL, BGEZ, BGEZAL, BGEZALL, BGEZL, BGTZ, BGTZL, BLEZ, BLEZL, BLTZAL, CTC


Operation operations[] = {
    //R format
    {0x02, "SRL"},
    {0x03, "SRA"},
    {0x04, "SSLV"},
    {0x06, "SRLV"},
    {0x07, "SRAV"},
    {0x0C, "SYSCALL"},
    {0x0D, "BREAK"},
    {0x14, "DSLLV"},
    {0x16, "DSRLV"},
    {0x17, "DSRAV"},
    {0x18, "MULT"},
    {0x19, "MULTU"},
    {0x1A, "DIV"},
    {0x1B, "DIVU"},
    {0x1C, "DMULT"},
    {0x1D, "DMULTU"},
    {0x1E, "DDIV"},
    {0x1F, "DDIVU"},
    {0x20, "ADD"}, 
    {0x21, "ADDU"},
    {0x22, "SUB"},
    {0x23, "SUBU"},
    {0x24, "AND"}, 
    {0x25, "OR"},
    {0x26, "XOR"},
    {0x27, "NOR"},
    {0x2A, "SLT"},
    {0x2B, "SLTU"},
    {0x2C, "DADD"}, 
    {0x2D, "DADDU"},
    {0x2E, "DSUB"},
    {0x2F, "DSUBU"},
    {0x30, "TGE"},
    {0x31, "TGEU"},
    {0x32, "TLT"},
    {0x33, "TLTU"},
    {0x34, "TEQ"},
    {0x36, "TNE"},
    {0x38, "DSLL"},
    {0x3A, "DSRL"},
    {0x3B, "DSRA"},
    {0x3C, "DSLL32"},
    {0x3E, "DSRL32"},
    {0x3F, "DSRA32"},

    //I format
    {0x01, "BGEZ"}, //will need additional conditions //TEQI uses this and other bits , also TGEI, TGEIU, TLTI, TLTIU, TNEI
    {0x04, "BEQ"},
    {0x05, "BNE"},
    {0x06, "BLEZ"},
    {0x07, "BGTZ"},
    {0x08, "ADDI"},
    {0x09, "ADDIU"},
    {0x0A, "SLTI"},
    {0x0B, "SLTIU"},
    {0x0C, "ANDI"},
    {0x0D, "ORI"},
    {0x0E, "XORI"},
    {0x0F, "LUI"}, //additional bits need to be checked
    {0x10, "DMCF0"}, //DMTC0 uses same code need to filter other bits .. could also be ERET     {0x10, "TLBP"}, //also tlbr, tlbwi, tlbwr other bits too
    {0x14, "BEQL"},
    {0x15, "BNEL"},
    {0x16, "BLEZL"},
    {0x17, "BGTZL"},
    {0x18, "DADDI"},
    {0x19, "DADDIU"},
    {0x1A, "LDL"},
    {0x1B, "LDR"},
    {0x20, "LB"}, //op code is same for LL    // look up MFC - MTLO confusing
    {0x21, "LH"},
    {0x22, "LWL"},
    {0x23, "LW"}, //need to handle LWCz
    {0x24, "LBU"}, //op code is same for LLD
    {0x25, "LHU"},
    {0x26, "LWR"},
    {0x27, "LWU"},
    {0x28, "SB"},
    {0x29, "SH"},
    {0x2A, "SWL"},
    {0x2B, "SW"}, //look up SWCz
    {0x2C, "SDL"},
    {0x2D, "SDR"},
    {0x2E, "SWR"},
    {0x2F, "CACHE"},
    {0x37, "LD"}, //look up how to handle LDCz,
    {0x38, "SC"},
    {0x3C, "SCD"},
    {0x3F, "SD"}, //look up SDCz
    {0x41, "BCzF1"},
    {0x45, "BCzF2"},
    {0x49, "BCzF3"},
    {0x4D, "BCzF4"}, //need to look into bczfl and bczft, needs additional conditions

    //J format
    {0x02, "J"},
    {0x03, "JAL"},
    {0x09, "JALR"} //op code is 0 for this and other bits need checking same for JALR which returns r31 and JR


    //look up shifts they require lots of bits
    //SYNC = NOP look it up
};

unsigned int programcounter = 0x80000000; //starting address for instructions
int instruction_count = sizeof(instructions) / sizeof(unsigned int); 
int operation_count = sizeof(operations) / sizeof(Operation);
char* find_opname(unsigned int function_number);
char* get_register_name(int reg);

int main(void) {

        unsigned int opcode = (instructions[i] & opcodemask) >> 26; //mask then shift to least significant bit
        unsigned int rs = (instructions[i] & rs_mask) >> 21; 
        unsigned int rt = (instructions[i] & rt_mask) >> 16; 
        unsigned int rd = (instructions[i] & rd_mask) >> 11;
        unsigned int shamt = (instructions[i] & shamtmask) >> 6;
        unsigned int function = (instructions[i] & functmask);
        char* fname = find_opname(function); //this might not work because of dups

        if (opcode == 0) { //handle R-format cases

            switch (function) {

                case 0x02: //SRL
                    char *reg1 = register_names[rt];
                    char *reg2 = register_names[rd];
                    printf("%x  %-3s  %s, %s, %d\n", programcounter, fname, reg2, reg1, shamt);
                    break;

                case 0x03: //SRA
                    char *reg1 = register_names[rt];
                    char *reg2 = register_names[rd];
                    printf("%x  %-3s  %s, %s, %d\n", programcounter, fname, reg2, reg1, shamt);
                    break;

                case 0x04: //SLLV
                    char *reg1 = register_names[rs];
                    char *reg2 = register_names[rt];
                    char *reg3 = register_names[rd];
                    printf("%x  %-3s  %s, %s, %s\n", programcounter, fname, reg3, reg2, reg1);
                    break;

                case 0x06: //SRLV
                    char *reg1 = register_names[rs];
                    char *reg2 = register_names[rt];
                    char *reg3 = register_names[rd];
                    printf("%x  %-3s  %s, %s, %s\n", programcounter, fname, reg3, reg2, reg1);
                    break;
            default:
                break;
            }
           

            printf("%x  %-3s  $%d, $%d, $%d\n", programcounter, fname, rd, rs, rt);

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

char* find_opname(unsigned int number) { //takes an opcode/function number and returns its name

    for(int i = 0; i < operation_count; i++) {
        
        if(operations[i].number == number) {
            return operations[i].name;
        }
    }
}

char* get_register_name(int reg) {
    if (reg >= 0 && reg < 32) {
        return register_names[reg];
    } else {
        return "unknown";
    }
}