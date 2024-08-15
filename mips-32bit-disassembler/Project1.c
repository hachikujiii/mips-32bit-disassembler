#include <stdio.h>
#include <stdint.h>

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


Operation R_function_codes[] = {

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

    //J format
    {0x02, "J"},
    {0x03, "JAL"},
    {0x09, "JALR"} //op code is 0 for this and other bits need checking same for JALR which returns r31 and JR


    //look up shifts they require lots of bits
    //SYNC = NOP look it up
};
Operation I_opcodes[] = {

    {0x01, "BGEZ"},
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
    {0x0F, "LUI"}, 
    {0x10, "DMFC0"}, //DMTC0 uses same code need to filter other bits .. could also be ERET     {0x10, "TLBP"}, //also tlbr, tlbwi, tlbwr other bits too
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
    {0x4D, "BCzF4"} //need to look into bczfl and bczft, needs additional conditions
};


unsigned int programcounter = 0x80000000; //starting address for instructions
char* find_opname(unsigned int number);
char* find_functname(unsigned int number);
char* get_register_name(int reg); 
int function_count = sizeof(R_function_codes) / sizeof(Operation);
int operation_count = sizeof(I_opcodes) / sizeof(Operation);


int main(void) {

        unsigned int opcode = (instructions[i] & opcodemask) >> 26; //mask then shift to least significant bit
        unsigned int rs = (instructions[i] & rs_mask) >> 21; 
        unsigned int rt = (instructions[i] & rt_mask) >> 16; 
        unsigned int rd = (instructions[i] & rd_mask) >> 11;
        unsigned int shamt = (instructions[i] & shamtmask) >> 6;
        unsigned int function = (instructions[i] & functmask);
        int16_t offset = (instructions[i] & offsetmask);
        int16_t immediate;
        unsigned int targetaddress;

        //special cases
        // ERET  0100 0010 0000 0000 0000 0000 0001 1000

        if (opcode == 0) { //handle R-format cases

            char* fname = find_functname(function);

            switch (function) {

                case 0x02: //SRL   shift right logical
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %d\n", programcounter, fname, rd_reg, rt_reg, shamt);
                    programcounter += 4;
                    break;

                case 0x03: //SRA   shift right arithmetic 
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %d\n", programcounter, fname, rd_reg, rt_reg, shamt);
                    programcounter += 4;
                    break;

                case 0x04: //SLLV  shift left logical variable
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rt_reg, rs_reg);
                    programcounter += 4;
                    break;

                case 0x06: //SRLV  shift right logical variable
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rt_reg, rs_reg);
                    programcounter += 4;
                    break;

                case 0x07: //SRAV shift right arithmetic variable
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rt_reg, rs_reg);
                    programcounter += 4;
                    break;
                
                case 0x08: //JR   jump to address stored in rs
                    char *rs_reg = register_names[rs];
                    printf("%X  JR  %s\n", programcounter, rs_reg);
                    programcounter += 4;
                    break;

                case 0x09: //double check this but should work

                    switch(shamt) {

                        case 0x1F: //JALR   jump to address stored in rs, store return in r31
                        char *rs_reg = register_names[rs];
                        printf("%X  JALR  %s\n", programcounter, rs_reg);
                        programcounter += 4;
                        break;

                    default: //JALR when shamt is something else
                        char *rs_reg = register_names[rs];
                        char *rd_reg = register_names[rd];
                        printf("%X  JALR  %s, %s\n", programcounter, rd_reg, rs_reg);
                        programcounter += 4;
                        break;

                    }

                case 0x0C: //SYSCALL
                    printf("SYSCALL\n");
                    programcounter += 4;
                    break;
                case 0x0D: //BREAK
                    printf("BREAK\n");
                    programcounter += 4;
                    break;

                case 0x14: //DSLLV  doubleword shift left logical variable
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rt_reg, rs_reg);
                    programcounter += 4;
                    break;

                case 0x16: //DSRLV  doubleword shift right logical variable
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rt_reg, rs_reg);
                    programcounter += 4;
                    break;

                case 0x17: //DSRAV  doubleword shift right arithmetic variable
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rt_reg, rs_reg);
                    programcounter += 4;
                    break;

                case 0x18: //MULT  multiply
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x19: //MULTU   multiply unsigned by unsigned
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x1A: //DIV    division
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x1B: //DIVU    division unsigned by unsigned 
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x1C: //DMULT    mult signed by signed store doubleword 
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x1D: //DMULTU   mult unsigned by unsigned store doubleword 
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x1E: //DDIV   divide signed by signed store quotient and remainder
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;
                
                case 0x1F: //DDIVU   divide unsigned by unsigned store quotient and remainder
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x20: //ADD
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x21: //ADDU   add unsigned and unsigned
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x22: //SUB
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x23: //SUBU   sub unsigned and unsigned
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;
                
                case 0x24: //AND 
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;
                
                case 0x25: //OR
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;
                
                case 0x26: //XOR
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x27: //NOR
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x2A: //SLT   if signed rs < signed rt, store 1 in rd otherwise store 0
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;  
                
                case 0x2B: //SLTU   if unsigned rs < unsigned rt, store 1 in rd otherwise store 0
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x2C: //DADD   mode restrictions?
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x2D: //DADDU   mode restrictions?
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;
                
                case 0x2E: //DSUB   mode restrictions?
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x2F: //DSUBU   mode restrictions?
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x30: //TGE    if rs >= rt, cause trap exception
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;
                
                case 0x31: //TGEU    if unsigned rs >= unsigned rt, cause trap exception
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;

                case 0x32: //TLT    if rs < rt, cause trap exception
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;  

                case 0x33: //TLTU    if unsigned rs < unsigned rt, cause trap exception
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;
                
                case 0X34: //TEQ    if rs == rt cause trap exception
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;
                
                case 0x36: //TNE   trap not equal
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  %-3s  %s, %s\n", programcounter, fname, rs_reg, rt_reg);
                    programcounter += 4;
                    break;
                
                case 0x38: //DSLL   shift rt left by shamt bits, store in rd
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %d\n", programcounter, fname, rd_reg, rt_reg, shamt);
                    programcounter += 4;
                    break;

                case 0x3A: //DSRL   shift rt right by shamt bits, store in rd
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %d\n", programcounter, fname, rd_reg, rt_reg, shamt);
                    programcounter += 4;
                    break;

                case 0x3B: //DSRA  shift rt right arithmetic by shamt bits, store in rd
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %d\n", programcounter, fname, rd_reg, rt_reg, shamt);
                    programcounter += 4;
                    break;
                
                case 0x3C: //DSLL32   shift rt left by (32 + shamt) store in rd
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %s\n", programcounter, fname, rd_reg, rt_reg, rs_reg);
                    programcounter += 4;
                    break;
                
                case 0x3E: //DSRL32   shift rt right by (32 + shamt) store in rd
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %d\n", programcounter, fname, rd_reg, rt_reg, shamt);
                    programcounter += 4;
                    break;

                case 0x3F: //DSRA32   same as above but arithmetic shift
                    char *rt_reg = register_names[rt];
                    char *rd_reg = register_names[rd];
                    printf("%X  %-3s  %s, %s, %d\n", programcounter, fname, rd_reg, rt_reg, shamt);
                    programcounter += 4;
                    break;
            default:
                printf("ERROR: UNKNOWN INSTRUCTION\n");
                programcounter += 4;
                break;
            }
        
        } else if (opcode != 0) { //handle I-format cases
            
            char* opname = find_opname(opcode);

            switch(opcode) {

                case 0x01:  

                    switch(rt) {
                        
                        case 0x00: //BLTZ    branch if rs is less than 0
                            char *rs_reg = register_names[rs];
                            printf("%X  BLTZ  %s, %X\n", programcounter, rs_reg, offset);
                            programcounter += 4;
                            break;

                        case 0x01: //BGEZ   branch on greater than or equal to zero
                            char *rs_reg = register_names[rs];
                            printf("%X  BGEZ  %s, %X\n", programcounter, rs_reg, offset);
                            programcounter += 4;
                            break;
                        
                        case 0x03: //BGEZL   if rs >=0, branch, otherwise discard delay slot instruction
                            char *rs_reg = register_names[rs];
                            printf("%X  BGEZL  %s, %X\n", programcounter, rs_reg, offset);
                            programcounter += 4;
                            break;

                        case 0x08: //TGEI     if rs >= immediate, cause trap exception
                            char *rs_reg = register_names[rs];
                            immediate = offset;
                            printf("%X  TGEI  %s, %X\n", programcounter, rs_reg, immediate);
                            programcounter += 4;
                            break;

                        case 0x09: //TGEIU     if unsigned rs >= immediate , cause trap exception
                            char *rs_reg = register_names[rs];
                            immediate = offset;
                            printf("%X  TGEIU  %s, %X\n", programcounter, rs_reg, immediate);
                            programcounter += 4;
                            break;
                        
                        case 0x0A: //TLTI     if rs < imediate cause trap exception
                            char *rs_reg = register_names[rs];
                            immediate = offset;
                            printf("%X  TLTI  %s, %X\n", programcounter, rs_reg, immediate);
                            programcounter += 4;
                            break;
                        
                        case 0x0B: //TLTIU    if unsigned rs < imediate cause trap exception
                            char *rs_reg = register_names[rs];
                            immediate = offset;
                            printf("%X  TLTIU  %s, %X\n", programcounter, rs_reg, immediate);
                            programcounter += 4;
                            break;

                        case 0x0C: //TEQI    if rs == immediate, cause trap exception
                            char *rs_reg = register_names[rs];
                            immediate = offset;
                            printf("%X  TEQI  %s, %X\n", programcounter, rs_reg, immediate);
                            programcounter += 4;
                            break;
                        
                        case 0x0E: //TNEI    if rs != immediate, cause trap exception
                            char *rs_reg = register_names[rs];
                            immediate = offset;
                            printf("%X  TNEI  %s, %X\n", programcounter, rs_reg, immediate);
                            programcounter += 4;
                            break;

                        case 0x10: //BLTZAL   if rs < 0 branch to address and store next in r31, otherwise discard
                            char *rs_reg = register_names[rs];
                            printf("%X  BLTZAL  %s, %X\n", programcounter, rs_reg, offset);
                            programcounter += 4;
                            break;

                        case 0x11: //BGEZAL    branch on greater than or equal to zero, store next add in r31
                            char *rs_reg = register_names[rs];
                            printf("%X  BGEZAL  %s, %X\n", programcounter, rs_reg, offset);
                            programcounter += 4;
                            break;

                        case 0x13: //BGEZALL    branch on >= 0, store next add r31, if not, discard delay slot instruction
                            char *rs_reg = register_names[rs];
                            printf("%X  BGEZALL  %s, %X\n", programcounter, rs_reg, offset);
                            programcounter += 4;
                            break;

                        default:
                            printf("error on bgez or equivalent");
                            break;
                    }

                case 0x02: //J     jump to target 
                    targetaddress = instructions[i] & 0x03FFFFFF;
                    printf("%X  J  %X\n", programcounter, targetaddress);
                    programcounter += 4;
                    break;

                case 0x03: //JAL     jump to target, store return address in r31 
                    targetaddress = instructions[i] & 0x03FFFFFF;
                    printf("%X  JAL  %X\n", programcounter, targetaddress);
                    programcounter += 4;
                    break; 

                case 0x04: //BEQ    if rs == rt branch to address (delay slot + offset)
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  BEQ  %s, %s, %X", programcounter, rs_reg, rt_reg, offset);
                    programcounter += 4;
                    break;

                case 0x05: //BNE  if rs != rt branch to address delay slot + offset
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    printf("%X  BNE  %s, %s, %X", programcounter, rs_reg, rt_reg, offset);
                    programcounter += 4;
                    break;

                case 0x06: //BLEZ    if rs <= 0 branch to address
                    char *rs_reg = register_names[rs];
                    printf("%X  BLEZ  %s, %X", programcounter, rs_reg, offset);
                    programcounter += 4;
                    break;

                case 0x07: //BGTZ     if rs > 0 branch to address
                    char *rs_reg = register_names[rs];
                    printf("%X  BGTZ  %s, %X", programcounter, rs_reg, offset);
                    programcounter += 4;
                    break;

                case 0x08: //ADDI    add rs and immediate, store in rt
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    immediate = offset;
                    printf("%X  ADDI  %s, %s, %X", programcounter, rt_reg, rs_reg, immediate);
                    programcounter += 4;
                    break;

                case 0x09: //ADDIU    add unsigned rs and immediate, store in rt
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    immediate = offset;
                    printf("%X  ADDIU  %s, %s, %X", programcounter, rt_reg, rs_reg, immediate);
                    programcounter += 4;
                    break;

                case 0x0A: //SLTI    if rs < immediate, store 1 in rd otherwise 0
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    immediate = offset;
                    printf("%X  SLTI  %s, %s, %X", programcounter, rt_reg, rs_reg, immediate);
                    programcounter += 4;
                    break;

                case 0x0B: //SLTIU    if rs < immediate, store 1 in rd otherwise 0
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    immediate = offset;
                    printf("%X  SLTIU  %s, %s, %X", programcounter, rt_reg, rs_reg, immediate);
                    programcounter += 4;
                    break;

                case 0x0C: //ANDI    and rs with zero extended immediate, store in rt
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    immediate = offset;
                    printf("%X  ANDI  %s, %s, %X", programcounter, rt_reg, rs_reg, immediate);
                    programcounter += 4;
                    break;

                case 0x0D: //ORI    or rs and zero extended immediate, store in rt
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    immediate = offset;
                    printf("%X  ORI  %s, %s, %X", programcounter, rt_reg, rs_reg, immediate);
                    programcounter += 4;
                    break;

                case 0x0E: //XORI    xor rs and zero extended immediate, store in rt
                    char *rs_reg = register_names[rs];
                    char *rt_reg = register_names[rt];
                    immediate = offset;
                    printf("%X  XORI  %s, %s, %X", programcounter, rt_reg, rs_reg, immediate);
                    programcounter += 4;
                    break;

                case 0x0F: //LUI    load upper immediate , imm shifted left 16 bits using trailing 0s
                    char *rt_reg = register_names[rt];
                    immediate = offset;
                    printf("%X  LUI  %s, %X", programcounter, rt_reg, immediate);
                    programcounter += 4;
                    break;

                case 0x10:

                    switch(rs) {

                        case 0x01: //DMFC0   copy doubleword contents of CPz coprocesser rd to GPR rt
                            char *rd_reg = register_names[rd];
                            char *rt_reg = register_names[rt];
                            printf("%X  DMFC0  %s, %s", programcounter, rt_reg, rd_reg);
                            programcounter += 4;
                            break;

                        case 0x05: //DMTC0   copy doubleword contents of GPR rt to CPz coprocessor rd
                            char *rd_reg = register_names[rd];
                            char *rt_reg = register_names[rt];
                            printf("%X  DMTC0  %s, %s", programcounter, rt_reg, rd_reg);
                            programcounter += 4;
                            break;
                    }   
                
                case 0x1A: //LDL loads portion of dw at address, stores 1-8 bytes in high order portion of rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LDL  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;
                
                case 0x1B: //LDR loads portion of dw at address, stores 1-8 bytes in low order portion of rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LDR  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;

                case 0x20: //LB   load byte at mem address , stores sign extended byte in rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LB  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;

                case 0x21: //LH   load halfword at mem address , stores sign extended halfword in rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%x  LH  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;

                case 0x22: //LWL   loads portion of word at mem address, stores 1-4 bytes in high order of rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%x  LWL  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;
                
                case 0x23: //LW   load word at mem address , stores sign extended word in rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%x  LW  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;
               
                case 0x24: //LBU   same lb but stores zero extended byte in rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LBU  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;

                case 0x25: //LHU   load unsigned halfword at mem address , stores zero extended halfword in rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LHU  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;

                case 0x26: //LWR   loads portion of word at mem address, stores 1-4 bytes in low order of rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%x  LWR  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;
                
                case 0x27: //LWU   loads portion of word at mem address, stores zero extended word in rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%x  LWU  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;

                case 0x30://LL/LWCz(0)   load linked/ copies word at mem address to CP0
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LL/LWC0  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;
                
                case 0x31://LWCz(1)   copies word at mem address to CP1
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LWC1  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;
                
                case 0x32://LWCz(2)   copies word at mem address to CP2
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LWC2  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;

                case 0x33://LWCz(3)   copies word at mem address to CP3
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LWC3  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;
                
                case 0x34: //LLD or LDCz(0) co processor zero would be same op code.. need to separate case for co processor reg
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LLD/LDC0  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;
                
                case 0x35: //LDCz(1) copy double word stored at mem address , to co processor 1
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LDC1  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;
                
                case 0x36: //LDCz(2) copy double word stored at mem address , to co processor 2
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LDC2  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;

                case 0x37: //LD/LCDz(3)   load doubleword and store double word in rt
                    char *rt_reg = register_names[rt];
                    char *rs_reg = register_names[rs];
                    printf("%X  LD/LCD3  %s, %d(%s)\n", programcounter, rt_reg, offset, rs_reg);
                    programcounter += 4;
                    break;
            }     
        }
}

//r format
char* find_functname(unsigned int number) { //takes an opcode/function number and returns its name

    for(int i = 0; i < function_count; i++) {
        
        if(R_function_codes[i].number == number) {
            return R_function_codes[i].name;
        }
    }
}

char* find_opname(unsigned int number) { //takes an opcode/function number and returns its name

    for(int i = 0; i < operation_count; i++) {
        
        if(I_opcodes[i].number == number) {
            return I_opcodes[i].name;
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