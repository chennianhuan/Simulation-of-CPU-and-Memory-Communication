#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <time.h>
#include <stdbool.h>


int CPU_Mem[2], Mem_CPU[2]; // Two pipes to communite cpu and memory

// Read file and initialize memory
void mem_initialization(FILE* file, int memory[2000]){
    char line[100];
    int i = 0, num2 = 0;

    while(fgets(line, 100, file) != NULL){
        if(line[0] == '.'){
            if(sscanf(line, ".%d", &num2) == 1){
                i = num2;	// jump to the adrress number
            }
        }
        else{
            if(sscanf(line, "%d", &num2) == 1){
                memory[i] = num2;
                i++;
            }
        }
    }
}

// Check memory violation before execution
bool checkMem_violation(int address, int mode){
    if (address < 1000 || mode == 1){
        return true;
    }
    else{
        printf("Fail. Cannot access memory %d\n", address);
        return false;
    }
}

// Reading side and writing side of cpu to memory pipe
void cpu2mem(char type, void *buf, int position) {
    switch(position) {
        case 0:
            switch(type) {
                case 'c':// if reading a char
                    read(CPU_Mem[0], buf, sizeof(char));
                    break;
                case 'i':// if reading an int
                    read(CPU_Mem[0], buf, sizeof(int));
                    break;
            }
            break;
        case 1:
            switch(type) {
                case 'c':// Reading a char
                    write(CPU_Mem[1], buf, sizeof(char));
                    break;
                case 'i':// Reading an int
                    write(CPU_Mem[1], buf, sizeof(int));
                    break;
            }
            break;
    }
}

// Reading side and writing side of memory to cpu pipes
void mem2cpu(char type, void *buf, int position) {
    switch(position) {
        case 0:
            switch(type) {
                case 'c':// Reading a char
                    read(Mem_CPU[0], buf, sizeof(char));
                    break;
                case 'i':// Reading an int
                    read(Mem_CPU[0], buf, sizeof(int));
                    break;
            }
            break;
        case 1:
            switch(type) {
                case 'c':// Reading a char
                    write(Mem_CPU[1], buf, sizeof(char));
                    break;
                case 'i':// Reading an int
                    write(Mem_CPU[1], buf, sizeof(int));
                    break;
            }
            break;
    }
}

int main(int argc, char *argv[]){

    if (argc != 3){
        printf("Invalid number of argument.");
        exit(1);
    }
    if (atoi(argv[2]) <= 0){
        printf("Invalid timer X");
        exit(1);
    }

    FILE* file;
    int checkPipe[2];
    pid_t pid;
    int instructCount = 0;
    int timerX = atoi(argv[2]);
    int mode = 0;
    int increment = timerX;
    int userSP = 0;
    int userPC = 0;
    int memory[2000];
    int PC = 0;
    int SP = 1000;
    int IR = 0;
    int AC = 0;
    int X = 0;
    int Y = 0;
    int num = 0;
    int address = 0;
    int endExe = 0;
    int address1 = 0;
    int num1 = 0;

    // Open file
    file = fopen(argv[1], "r");
    // Check if open successfully
    if (!file){
        printf("Error loading file.");
    }
    else{
        // Open successfully, read file line by line and do memory initialization
        mem_initialization(file, memory);
    }
    // Close file
    fclose(file);

    checkPipe[0] = pipe(CPU_Mem);
    checkPipe[1] = pipe(Mem_CPU);
    // Check error in pipe
    if(checkPipe[0] < 0 || checkPipe[1] < 0){
        perror("Pipe failed.");
    }
    else{
        // Pipes created successfully, do fork
        pid = fork();
        // Check error in fork
        switch(pid) {
            case -1: {
                         perror("Error in fork");
                         break;
                     }
                     // Child process(cpu write to pipe)
            case 0:{
                       char rw;
                       close(CPU_Mem[0]);// close reading side of cpu to mem pipe
                       close(Mem_CPU[1]);// close writing side of mem to cpu pipe
                       while(1){
                           // time out interrupt
                           if(instructCount >= timerX && mode == 0){
                               // Switch to systme mode
                               mode = 1;
                               timerX = timerX + increment; // update timer for next interrupt
                               rw = 'w';
                               // Save SP and PC
                               userSP = SP;
                               userPC = PC;
                               SP = 2000;
                               PC = 1000;
                               SP--;
                               cpu2mem('c', &rw, 1);
                               cpu2mem('i', &SP, 1);
                               cpu2mem('i', &userSP, 1);
                               SP--;
                               cpu2mem('c', &rw, 1);
                               cpu2mem('i', &SP, 1);
                               cpu2mem('i', &userPC, 1);
                           }
                           // User mode
                           else{
                               rw = 'r';
			       // Read instruction code
                               cpu2mem('c', &rw, 1);
                               cpu2mem('i', &PC, 1);
                               mem2cpu('i', &IR, 0);
                               if(IR != 30){
                                   PC++;
                               }
                               if(mode == 0){
                                   // Track the # of instructions executed in user mode
                                   instructCount++;
                               }
                               // Instruction set
                               switch(IR){
                                   // Load the value into the AC
                                   case 1:
                                       rw = 'r';
                                       // Send read symbol, value of PC into pipe
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &PC, 1);
                                       PC = PC + 1;
                                       // Save value read from pipe into num
                                       mem2cpu('i', &num, 0);
                                       AC = num;
                                       break;
                                   case 2: // Load the value at the address into AC
                                       rw = 'r';
                                       // Send read symbol, PC value into pipe
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &PC, 1);
                                       PC = PC + 1;
                                       // Read value of address from pipe
                                       mem2cpu('i', &address, 0);
                                       // determine if address is within the accessable range
                                       // if yes, repeat the operation to load the value to AC
                                       if (checkMem_violation){
                                       		cpu2mem('c', &rw, 1);
                                       		cpu2mem('i', &address, 1);
                                       		mem2cpu('i', &num, 0);
                                           AC = num;
                                       }
                                       break;
                                   case 3: // Load the value from the address found in the given address into AC
                                       // for example, if LoadInd 500, and 500 contains 100, then load from 100
                                       // Similar to case 2
                                       rw = 'r';
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &PC, 1);
                                       PC = PC + 1;
                                       mem2cpu('i', &address, 0);
                                       if (checkMem_violation){
                                           cpu2mem('c', &rw, 1);
                                           cpu2mem('i', &address, 1);
                                           mem2cpu('i', &address, 0);
                                           if (address < 1000 || mode == 1){
                                               cpu2mem('c', &rw, 1);
                                               cpu2mem('i', &address, 1);
                                               mem2cpu('i', &num, 0);
                                               AC = num;
                                           }
                                           else{
                                               printf("Cannot Access Memory %d\n", address);
                                               break;
                                           }
                                       }
                                       break;
                                   case 4: // Load the value at (address + X) into the AC
                                       rw = 'r';
                                       // Write read symbol, value of PC into pipe
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &PC, 1);
                                       PC = PC + 1;
                                       // Read value of address from pipe
                                       mem2cpu('i', &address, 0);
                                       address = address + X;
                                       if(checkMem_violation){
                                           cpu2mem('c', &rw, 1);
                                           cpu2mem('i', &address, 1);
                                           mem2cpu('i', &num, 0);
                                           AC = num;
                                       }
                                       break;
                                   case 5: // Load the value at (address + Y) int the AC
                                       // Similar to case 4
                                       rw = 'r';
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &PC, 1);
                                       PC = PC + 1;
                                       mem2cpu('i', &address, 0);
                                       address = address + Y;
                                       if(checkMem_violation){
                                           cpu2mem('c', &rw, 1);
                                           cpu2mem('i', &address, 1);
                                           mem2cpu('i', &num, 0);
                                           AC = num;
                                       }
                                       break;
                                   case 6: // Load from (SP + X)into the AC, for example, SP = 900,
                                       // X = 1, load from 991
                                       rw = 'r';
                                       address = SP + X;
                                       if(checkMem_violation){
                                           // Write read symbol, value stored in address into pipe
                                           cpu2mem('c', &rw, 1);
                                           cpu2mem('i', &address, 1);
                                           // Read value of num stored in SP + X into AC
                                           mem2cpu('i', &num, 0);
                                           AC = num;
                                       }
                                       break;
                                   case 7: // Store the value in the AC into the address
                                       rw = 'r';
                                       // Write read symbol, value of PC into pipe
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &PC, 1);
                                       PC = PC + 1;
                                       // Read the value of address from pipe
                                       mem2cpu('i', &address, 0);
                                       if(checkMem_violation){
                                           rw = 'w';
                                           cpu2mem('c', &rw, 1);
                                           cpu2mem('i', &address, 1);
                                           cpu2mem('i', &AC, 1);
                                       }
                                       break;
                                   case 8: // Gets a random int from 1 to 100 into the AC
                                       // Seed the random-number generator with current
                                       // time so that numbers will be different
                                       srand((unsigned int) time(NULL));
                                       AC = rand() % 100 + 1;
                                       break;
                                   case 9: // Put port: if port = 1, write AC as an int to screen
                                       // if port = 2, write AC as an char
                                       //
                                       rw = 'r';
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &PC, 1);
                                       PC = PC + 1;
                                       mem2cpu('i', &num, 0);
                                       if(num == 1){
                                           printf("%d", AC);
                                       }
                                       else if(num == 2){
                                           printf("%c", AC);
                                       }
                                       break;
                                   case 10:// Add value in X to AC
                                       AC = AC + X;
                                       break;
                                   case 11: // Add value in Y to AC
                                       AC = AC + Y;
                                       break;
                                   case 12: // Subtract the value in X from AC
                                       AC = AC - X;
                                       break;
                                   case 13:// Subtract the value in Y from AC
                                       AC = AC - Y;
                                       break;
                                   case 14: // Copy the value in AC to X
                                       X = AC;
                                       break;
                                   case 15:// Copy the value in X to AC
                                       AC = X;
                                       break;
                                   case 16:// Copy the value in AC to Y
                                       Y = AC;
                                       break;
                                   case 17:// Copy the value in Y to AC
                                       AC = Y;
                                       break;
                                   case 18:// Copy the value in AC to SP
                                       SP = AC;
                                       break;
                                   case 19:// Copy the value in SP to AC
                                       AC = SP;
                                       break;
                                   case 20:// Jump to the address
                                       rw = 'r';
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &PC, 1);
                                       PC = PC + 1;
                                       mem2cpu('i', &address, 0);
                                       if(address < 1000 || mode == 1){
                                           PC = address;
                                       }
                                       else{
                                           printf("Memory Violation!. %d\n", address);
                                           break;
                                       }
                                       break;
                                   case 21: // Jump to the address only if the value in AC is 0
                                       if(AC == 0){
                                           rw = 'r';
                                           cpu2mem('c', &rw, 1);
                                           // Send the value of PC into pipe
                                           cpu2mem('i', &PC, 1);
                                       }
                                       PC = PC + 1;
                                       if(AC == 0){
                                           // Read value stored in address from pipe and save it to address
                                           mem2cpu('i', &address, 0);
                                           if(address < 1000 || mode == 1){
                                               PC = address;
                                           }
                                           else{
                                               printf("Memory Violation! %d\n", address);
                                               break;
                                           }
                                       }
                                       break;
                                   case 22: // Jump to the address only the value in AC is not zero
                                       // Similar to case 21
                                       if(AC != 0){
                                           rw = 'r';
                                           cpu2mem('c', &rw, 1);
                                           // Send the value of PC into pipe
                                           cpu2mem('i', &PC, 1);
                                       }
                                       PC = PC + 1;
                                       if(AC != 0){
                                           // Read value stored in address from pipe and save it to address
                                           mem2cpu('i', &address, 0);
                                           if(address < 1000 || mode == 1){
                                               PC = address;
                                           }
                                           else{
                                               printf("Memory Violation! %d\n", address);
                                               break;
                                           }
                                       }
                                       break;
                                   case 23: // Push return address onto stack, jump to the address
                                       rw = 'r';
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &PC, 1); // write return address(address of PC) into pipe
                                       PC = PC + 1;
                                       mem2cpu('i', &address, 0); //read return address from pipe, save to address
                                       if(checkMem_violation){
                                           rw = 'w';
                                           SP--;
                                           // Write write symbo', value of SP, value of PC to pipe
                                           cpu2mem('c', &rw, 1);
                                           cpu2mem('i', &SP, 1);
                                           cpu2mem('i', &PC, 1);
                                           PC = address;
                                       }
                                       break;
                                   case 24: // Pop return address from the stack, jump to address
                                       rw = 'r';
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &SP, 1);
                                       mem2cpu('i', &address, 0);
                                       if(address < 1000 || mode == 1){
                                           SP++;
                                           PC = address;
                                       }
                                       else{
                                           printf("Memory Violation! %d\n", address);
                                       }
                                       break;
                                   case 25:// Increment the value in X
                                       X++;
                                       break;
                                   case 26:// Decrement the value in X
                                       X--;
                                       break;
                                   case 27:// Push AC onto stack
                                       rw = 'w';
                                       SP--;
                                       // Send write symbol, value of SP, value of AC into pipe
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &SP, 1);
                                       cpu2mem('i', &AC, 1);
                                       break;
                                   case 28:// Pop from stack into AC
                                       rw = 'r';

                                       // Send read symbol, value of SP into pipe
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &SP, 1);
                                       mem2cpu('i', &AC, 0);//Save value read from pipe into AC
                                       SP++;
                                       break;
                                   case 29:// Perform system call, cause execution at address 1500
                                       mode = 1; // switch to system mode
                                       userPC = PC; // save PC
                                       userSP = SP; //save SP
                                       SP = 2000;
                                       PC = 1500;
                                       rw = 'w';
                                       SP--;
                                       // Save userSP into system stack
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &SP, 1);
                                       cpu2mem('i', &userSP, 1);
                                       SP--;
                                       // Save userPC into system stack
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &SP, 1);
                                       cpu2mem('i', &userPC, 1);
                                       break;
                                   case 30: // Return from system call
                                       mode = 0; // switch back to user mode
                                       rw = 'r';
                                       // Send read symbol and value of SP into pipe
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &SP, 1);
                                       // Save value read from pipe into userSP
                                       mem2cpu('i', &userPC, 0);
                                       SP++;
                                       cpu2mem('c', &rw, 1);
                                       cpu2mem('i', &SP, 1);
                                       mem2cpu('i', &userSP, 0);
                                       SP++;
                                       PC = userPC;
                                       SP = userSP;
                                       break;
                                   case 50: // End exectution
                                       rw = 'e';
                                       cpu2mem('c', &rw, 1);
                                       endExe = 1;
                                       break;
                               }

                           }
                           if(endExe == 1){
                               break;
                           }
                       }

                       break;
                   }
            default:{// Parent process (Memory)
                        char info;
                        close(CPU_Mem[1]);// close writing side of CPU to memory pipe
                        close(Mem_CPU[0]);// colse reading side of memory to CPU pipe
                        for(;;){
                            cpu2mem('c', &info, 0);

                            // Determine if it's a write a read to memory
                            if(info == 'w'){// a write
                                cpu2mem('i', &address1, 0);
                                cpu2mem('i', &num1, 0);
                                memory[address1] = num1;
                            }
                            else if(info == 'r'){// a read to memory
                                cpu2mem('i', &address1, 0);
                                num1 = memory[address1];
                                mem2cpu('i', &num1, 1);
                            }
                            else if(info == 'e'){
                                break;
                            }
                        }
                    }
        }
    }
}
