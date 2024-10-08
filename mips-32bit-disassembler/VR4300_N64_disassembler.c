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

char* find_opname(unsigned int number);
char* find_functname(unsigned int number);
char* get_register_name(int reg); 


int main(void) {

        //open rom dump file (.bin) and read 4 byte instructions while loop is running
        FILE *rom = fopen("OcarinaOfTime.bin", "rb");
        if(!rom) {
            perror("Failed to open ROM dump file");
            return 1;
        }

        uint32_t PC = 0xBFC00000; //starting address for instructions 
        uint32_t instruction;

        while(fread(&instruction, sizeof(uint32_t), 1, rom) == 1) {

        
            unsigned int opcode = (instruction & opcodemask) >> 26; //mask then shift to least significant bit
            unsigned int rs = (instruction & rs_mask) >> 21; 
            unsigned int rt = (instruction & rt_mask) >> 16; 
            unsigned int rd = (instruction & rd_mask) >> 11;
            unsigned int shamt = (instruction & shamtmask) >> 6;
            unsigned int function = (instruction & functmask);
            int16_t offset = (instruction & offsetmask);
            int16_t immediate;
            unsigned int targetaddress;

            const char* rt_reg;
            const char* rd_reg;
            const char* rs_reg;

            //special cases
            // ERET  0100 0010 0000 0000 0000 0000 0001 1000

            if (opcode == 0) { //handle R-format cases

                switch (function) {

                    case 0x02: //shift right logical
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  SRL  %s, %s, %d\n", PC, rd_reg, rt_reg, shamt);
                        PC += 4;
                        break;

                    case 0x03: //shift right arithmetic 
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  SRA  %s, %s, %d\n", PC, rd_reg, rt_reg, shamt);
                        PC += 4;
                        break;

                    case 0x04: //shift left logical variable
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  SLLV  %s, %s, %s\n", PC, rd_reg, rt_reg, rs_reg);
                        PC += 4;
                        break;

                    case 0x06: //shift right logical variable
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  SRLV  %s, %s, %s\n", PC, rd_reg, rt_reg, rs_reg);
                        PC += 4;
                        break;

                    case 0x07: //shift right arithmetic variable
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  SRAV  %s, %s, %s\n", PC, rd_reg, rt_reg, rs_reg);
                        PC += 4;
                        break;
                    
                    case 0x08: //jump to address stored in rs
                        rs_reg = register_names[rs];
                        printf("%X  JR  %s\n", PC, rs_reg);
                        PC += 4;
                        break;

                    case 0x09:

                        switch(shamt) {

                            case 0x1F: //jump to address stored in rs, store return in r31
                            rs_reg = register_names[rs];
                            printf("%X  JALR  %s\n", PC, rs_reg);
                            PC += 4;
                            break;

                        default: //JALR when shamt is something else
                            rs_reg = register_names[rs];
                            rd_reg = register_names[rd];
                            printf("%X  JALR  %s, %s\n", PC, rd_reg, rs_reg);
                            PC += 4;
                            break;

                        }

                    case 0x0C:
                        printf("%X  SYSCALL\n", PC);
                        PC += 4;
                        break;

                    case 0x0D:
                        printf("%X  BREAK\n", PC);
                        PC += 4;
                        break;

                    case 0x14: //doubleword shift left logical variable
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSLLV  %s, %s, %s\n", PC, rd_reg, rt_reg, rs_reg);
                        PC += 4;
                        break;

                    case 0x16: //doubleword shift right logical variable
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSRLV  %s, %s, %s\n", PC, rd_reg, rt_reg, rs_reg);
                        PC += 4;
                        break;

                    case 0x17: //doubleword shift right arithmetic variable
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSRAV  %s, %s, %s\n", PC, rd_reg, rt_reg, rs_reg);
                        PC += 4;
                        break;

                    case 0x18: //multiply
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  MULT  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x19: //multiply unsigned by unsigned
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  MULTU  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x1A: //division
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  DIV  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x1B: //division unsigned by unsigned 
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  DIVU  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x1C: //mult signed by signed store doubleword 
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  DMULT  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x1D: //mult unsigned by unsigned store doubleword 
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  DMULTU  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x1E: //divide signed by signed store quotient and remainder
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  DDIV  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;
                    
                    case 0x1F: //divide unsigned by unsigned store quotient and remainder
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  DDIVU  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x20:
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  ADD  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x21: //add unsigned and unsigned
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  ADDU  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x22:
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  SUB  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x23: //sub unsigned and unsigned
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  SUBU  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;
                    
                    case 0x24: 
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  AND  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;
                    
                    case 0x25:
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  OR  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;
                    
                    case 0x26:
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  XOR  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x27:
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  NOR  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x2A: //if signed rs < signed rt, store 1 in rd otherwise store 0
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  SLT  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;  
                    
                    case 0x2B: //if unsigned rs < unsigned rt, store 1 in rd otherwise store 0
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  SLTU  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x2C: //DADD   mode restrictions?
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DADD  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x2D: //DADDU   mode restrictions?
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DADDU  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;
                    
                    case 0x2E: //DSUB   mode restrictions?
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSUB  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x2F: //DSUBU   mode restrictions?
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSUBU  %s, %s, %s\n", PC, rd_reg, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x30: //if rs >= rt, cause trap exception
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  TGE  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;
                    
                    case 0x31: //if unsigned rs >= unsigned rt, cause trap exception
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  TGEU  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;

                    case 0x32: //if rs < rt, cause trap exception
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  TLT  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;  

                    case 0x33: //if unsigned rs < unsigned rt, cause trap exception
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  TLTU  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;
                    
                    case 0X34: //if rs == rt cause trap exception
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  TEQ  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;
                    
                    case 0x36: //trap not equal
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  TNE  %s, %s\n", PC, rs_reg, rt_reg);
                        PC += 4;
                        break;
                    
                    case 0x38: //shift rt left by shamt bits, store in rd
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSLL  %s, %s, %d\n", PC, rd_reg, rt_reg, shamt);
                        PC += 4;
                        break;

                    case 0x3A: //shift rt right by shamt bits, store in rd
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSLR  %s, %s, %d\n", PC, rd_reg, rt_reg, shamt);
                        PC += 4;
                        break;

                    case 0x3B: //shift rt right arithmetic by shamt bits, store in rd
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSRA  %s, %s, %d\n", PC, rd_reg, rt_reg, shamt);
                        PC += 4;
                        break;
                    
                    case 0x3C: //shift rt left by (32 + shamt) store in rd
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSLL32  %s, %s, %s\n", PC, rd_reg, rt_reg, rs_reg);
                        PC += 4;
                        break;
                    
                    case 0x3E: //shift rt right by (32 + shamt) store in rd
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSRL32  %s, %s, %d\n", PC, rd_reg, rt_reg, shamt);
                        PC += 4;
                        break;

                    case 0x3F: //same as above but arithmetic shift
                        rt_reg = register_names[rt];
                        rd_reg = register_names[rd];
                        printf("%X  DSRA32  %s, %s, %d\n", PC, rd_reg, rt_reg, shamt);
                        PC += 4;
                        break;
                default:
                    printf("ERROR: UNKNOWN INSTRUCTION\n");
                    PC += 4;
                    break;
                }
            
            } else if (opcode != 0) { //handle I-format cases

                switch(opcode) {

                    case 0x01:  

                        switch(rt) {
                            
                            case 0x00: //BLTZ    branch if rs is less than 0
                                rs_reg = register_names[rs];
                                printf("%X  BLTZ  %s, %X\n", PC, rs_reg, offset);
                                PC += 4;
                                break;

                            case 0x01: //BGEZ   branch on greater than or equal to zero
                                rs_reg = register_names[rs];
                                printf("%X  BGEZ  %s, %X\n", PC, rs_reg, offset);
                                PC += 4;
                                break;
                            
                            case 0x03: //BGEZL   if rs >=0, branch, otherwise discard delay slot instruction
                                rs_reg = register_names[rs];
                                printf("%X  BGEZL  %s, %X\n", PC, rs_reg, offset);
                                PC += 4;
                                break;

                            case 0x08: //TGEI     if rs >= immediate, cause trap exception
                                rs_reg = register_names[rs];
                                immediate = offset;
                                printf("%X  TGEI  %s, %X\n", PC, rs_reg, immediate);
                                PC += 4;
                                break;

                            case 0x09: //TGEIU     if unsigned rs >= immediate , cause trap exception
                                rs_reg = register_names[rs];
                                immediate = offset;
                                printf("%X  TGEIU  %s, %X\n", PC, rs_reg, immediate);
                                PC += 4;
                                break;
                            
                            case 0x0A: //TLTI     if rs < imediate cause trap exception
                                rs_reg = register_names[rs];
                                immediate = offset;
                                printf("%X  TLTI  %s, %X\n", PC, rs_reg, immediate);
                                PC += 4;
                                break;
                            
                            case 0x0B: //TLTIU    if unsigned rs < imediate cause trap exception
                                rs_reg = register_names[rs];
                                immediate = offset;
                                printf("%X  TLTIU  %s, %X\n", PC, rs_reg, immediate);
                                PC += 4;
                                break;

                            case 0x0C: //TEQI    if rs == immediate, cause trap exception
                                rs_reg = register_names[rs];
                                immediate = offset;
                                printf("%X  TEQI  %s, %X\n", PC, rs_reg, immediate);
                                PC += 4;
                                break;
                            
                            case 0x0E: //TNEI    if rs != immediate, cause trap exception
                                rs_reg = register_names[rs];
                                immediate = offset;
                                printf("%X  TNEI  %s, %X\n", PC, rs_reg, immediate);
                                PC += 4;
                                break;

                            case 0x10: //BLTZAL   if rs < 0 branch to address and store next in r31, otherwise discard
                                rs_reg = register_names[rs];
                                printf("%X  BLTZAL  %s, %X\n", PC, rs_reg, offset);
                                PC += 4;
                                break;

                            case 0x11: //BGEZAL    branch on greater than or equal to zero, store next add in r31
                                rs_reg = register_names[rs];
                                printf("%X  BGEZAL  %s, %X\n", PC, rs_reg, offset);
                                PC += 4;
                                break;

                            case 0x13: //BGEZALL    branch on >= 0, store next add r31, if not, discard delay slot instruction
                                rs_reg = register_names[rs];
                                printf("%X  BGEZALL  %s, %X\n", PC, rs_reg, offset);
                                PC += 4;
                                break;

                            default:
                                printf("error on bgez or equivalent");
                                break;
                        }

                    case 0x02: //jump to target 
                        targetaddress = instruction & 0x03FFFFFF;
                        printf("%X  J  %X\n", PC, targetaddress);
                        PC += 4;
                        break;

                    case 0x03: //jump to target, store return address in r31 
                        targetaddress = instruction & 0x03FFFFFF;
                        printf("%X  JAL  %X\n", PC, targetaddress);
                        PC += 4;
                        break; 

                    case 0x04: //if rs == rt branch to address (delay slot + offset)
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  BEQ  %s, %s, %X\n", PC, rs_reg, rt_reg, offset);
                        PC += 4;
                        break;

                    case 0x05: //if rs != rt branch to address delay slot + offset
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  BNE  %s, %s, %X\n", PC, rs_reg, rt_reg, offset);
                        PC += 4;
                        break;

                    case 0x06: //if rs <= 0 branch to address
                        rs_reg = register_names[rs];
                        printf("%X  BLEZ  %s, %X\n", PC, rs_reg, offset);
                        PC += 4;
                        break;

                    case 0x07: //if rs > 0 branch to address
                        rs_reg = register_names[rs];
                        printf("%X  BGTZ  %s, %X\n", PC, rs_reg, offset);
                        PC += 4;
                        break;

                    case 0x08: //add rs and immediate, store in rt
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        immediate = offset;
                        printf("%X  ADDI  %s, %s, %X\n", PC, rt_reg, rs_reg, immediate);
                        PC += 4;
                        break;

                    case 0x09: //add unsigned rs and immediate, store in rt
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        immediate = offset;
                        printf("%X  ADDIU  %s, %s, %X\n", PC, rt_reg, rs_reg, immediate);
                        PC += 4;
                        break;

                    case 0x0A: //if rs < immediate, store 1 in rd otherwise 0
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        immediate = offset;
                        printf("%X  SLTI  %s, %s, %X\n", PC, rt_reg, rs_reg, immediate);
                        PC += 4;
                        break;

                    case 0x0B: //if rs < immediate, store 1 in rd otherwise 0
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        immediate = offset;
                        printf("%X  SLTIU  %s, %s, %X\n", PC, rt_reg, rs_reg, immediate);
                        PC += 4;
                        break;

                    case 0x0C: //and rs with zero extended immediate, store in rt
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        immediate = offset;
                        printf("%X  ANDI  %s, %s, %X\n", PC, rt_reg, rs_reg, immediate);
                        PC += 4;
                        break;

                    case 0x0D: //or rs and zero extended immediate, store in rt
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        immediate = offset;
                        printf("%X  ORI  %s, %s, %X\n", PC, rt_reg, rs_reg, immediate);
                        PC += 4;
                        break;

                    case 0x0E: //xor rs and zero extended immediate, store in rt
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        immediate = offset;
                        printf("%X  XORI  %s, %s, %X\n", PC, rt_reg, rs_reg, immediate);
                        PC += 4;
                        break;

                    case 0x0F: //load upper immediate , imm shifted left 16 bits using trailing 0s
                        rt_reg = register_names[rt];
                        immediate = offset;
                        printf("%X  LUI  %s, %X\n", PC, rt_reg, immediate);
                        PC += 4;
                        break;

                    case 0x10:
                    case 0x11:
                    case 0x12:
                    case 0x13:
                        switch(rs) {

                            case 0x08: // BCzF, BCzT, BCzFL, BCzTL

                                switch(rt) {

                                    case 0x00: // BCzF
                                        printf("%X  BCzF  %X\n", PC, offset);
                                        PC += 4;
                                        break;
                                    case 0x01: // BCzT
                                        printf("%X  BCzT  %X\n", PC, offset);
                                        PC += 4;
                                        break;
                                    case 0x02: // BCzFL
                                        printf("%X  BCzFL  %X\n", PC, offset);
                                        PC += 4;
                                        break;
                                    case 0x03: // BCzTL
                                        printf("%X  BCzTL  %X\n", PC, offset);
                                        PC += 4;
                                        break;

                                    default:
                                        printf("%X  Unknown instruction at BCzf\n", PC);
                                        PC += 4;
                                        break;
                                }
                                break;

                            case 0x01: // DMFC0 - Copy doubleword contents of CPz coprocessor rd to GPR rt  
                                rd_reg = register_names[rd];
                                rt_reg = register_names[rt];
                                printf("%X  DMFC0  %s, %s\n", PC, rt_reg, rd_reg);
                                PC += 4;
                                break;

                            case 0x05: // DMTC0 - Copy doubleword contents of GPR rt to CPz coprocessor rd
                                rd_reg = register_names[rd];
                                rt_reg = register_names[rt];
                                printf("%X  DMTC0  %s, %s\n", PC, rt_reg, rd_reg);
                                PC += 4;
                                break;
                        }
                    
                    case 0x14: //if rs == rt branch to address
                        rs_reg = register_names[rs];
                        rt_reg = register_names[rt];
                        printf("%X  BEQL  %s, %s, %X\n", PC, rs_reg, rt_reg, offset);
                        PC += 4;
                        break;
                        
                    case 0x1A: //LDL loads portion of dw at address, stores 1-8 bytes in high order portion of rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LDL  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;
                    
                    case 0x1B: //LDR loads portion of dw at address, stores 1-8 bytes in low order portion of rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LDR  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;

                    case 0x20: //LB   load byte at mem address , stores sign extended byte in rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LB  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;

                    case 0x21: //LH   load halfword at mem address , stores sign extended halfword in rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%x  LH  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;

                    case 0x22: //LWL   loads portion of word at mem address, stores 1-4 bytes in high order of rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%x  LWL  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;
                    
                    case 0x23: //LW   load word at mem address , stores sign extended word in rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%x  LW  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;
                
                    case 0x24: //LBU   same lb but stores zero extended byte in rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LBU  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;

                    case 0x25: //LHU   load unsigned halfword at mem address , stores zero extended halfword in rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LHU  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;

                    case 0x26: //LWR   loads portion of word at mem address, stores 1-4 bytes in low order of rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%x  LWR  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;
                    
                    case 0x27: //LWU   loads portion of word at mem address, stores zero extended word in rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%x  LWU  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;

                    case 0x30://LL/LWCz(0)   load linked/ copies word at mem address to CP0
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LL/LWC0  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;
                    
                    case 0x31://LWCz(1)   copies word at mem address to CP1
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LWC1  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;
                    
                    case 0x32://LWCz(2)   copies word at mem address to CP2
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LWC2  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;

                    case 0x33://LWCz(3)   copies word at mem address to CP3
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LWC3  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;
                    
                    case 0x34: //LLD or LDCz(0) co processor zero would be same op code.. need to separate case for co processor reg
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LLD/LDC0  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;
                    
                    case 0x35: //LDCz(1) copy double word stored at mem address , to co processor 1
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LDC1  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;
                    
                    case 0x36: //LDCz(2) copy double word stored at mem address , to co processor 2
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LDC2  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;

                    case 0x37: //LD/LCDz(3)   load doubleword and store double word in rt
                        rt_reg = register_names[rt];
                        rs_reg = register_names[rs];
                        printf("%X  LD/LCD3  %s, %d(%s)\n", PC, rt_reg, offset, rs_reg);
                        PC += 4;
                        break;
                }     
            }
        }
    }
