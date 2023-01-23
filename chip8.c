/*******************************************************************************************
*
*   raylib [core] example - Basic window
*
*   Welcome to raylib!
*
*   To test examples, just press F6 and execute raylib_compile_execute script
*   Note that compiled executable is placed in the same folder as .c file
*
*   You can find all basic examples on C:\raylib\raylib\examples folder or
*   raylib official webpage: www.raylib.com
*
*   Enjoy using raylib. :)
*
*   Example originally created with raylib 1.0, last time updated with raylib 1.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2013-2022 Ramon Santamaria (@raysan5)
*
********************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "chip8.h"
#include <unistd.h>
#include <stdint.h>

FILE *rom;

//Arguments:
//  -B BackgroundColor
//  -F ForegroundColor
int main(int argc, char *argv[])
{
    // Initialization
    //--------------------------------------------------------------------------------------
    //Set background colors
    Color background = BLACK;
    Color foreground = WHITE;

    //Set default rom
    char *romString = "chip8-test-suite";
    if(argc > 1){
        //iterate through arguments
        for(int i = 1; i < argc; i++){
            if(!strcmp(argv[i],"-B")){
                background = mapStringToColor(argv[i+1], 'B');
                i++;
            }
            else if(!strcmp(argv[i],"-F")){
                foreground = mapStringToColor(argv[i+1], 'F');
                i++;
            }
            else if(!strcmp(argv[i], "-R")){
                romString = argv[i+1];
                i++;
            }
            else if(!strcmp(argv[i], "-help")){
                
                printf("Here is a list of the colors that can be used\nUse -B {color} to set background and -F {color} to set foreground");
                int i;
                for (i = 0; i < NUM_COLORS; i++) {
                    printf("%s\n", colorList[i]);
                }
                return 0;
            }
            else{
                printf("%s not valid\n",argv[i]);
                return -1;
            }
        }
    }
    const int screenWidth = 1000;
    const int screenHeight = 1000;

    InitWindow(screenWidth, screenHeight, "Chip8 Emulator");

    InitAudioDevice();      // Initialize audio device

    Sound beep = LoadSound("beep.wav");         // Load WAV audio file
    
    
    BeginDrawing();
        DrawRectangle(179,339,642,322,foreground);
        DrawRectangle(180,340,640,320,background);
    EndDrawing();

    //load font_set
    for(int i = 0; i < FONT_END; i++){
        memory[i] = fontset[i];
    }

    ////printf("Font Set Loaded\n");

    //load rom
    //"roms/Brix.ch8"
    char fullRomString[100];
    sprintf(fullRomString,"roms/%s.ch8",romString);
    rom = fopen(fullRomString,"r");
    if(rom == NULL){
        ////printf("ROM not found\n");
        return -1;
    }
    else{
        ////printf("ROM loaded\n");
    }

    fread(rom_buffer, sizeof(unsigned char), MEM_SIZE, rom);

    for(int i = 0; i < MEM_SIZE; i++){
        memory[i + PROGRAM_START] = rom_buffer[i];
    }
    fclose(rom);

    //emulation initialization
    PC = PROGRAM_START;
    for(int i = 0; i < NUM_REGISTERS; i++){
        registers[i] = 0x0;
        stack[i] = 0x0;
    }
    I = 0x0;
    delay_timer = 0x0;
    sound_timer = 0x0;
    SP = 0x0;
    for(int r = 0; r < DISPLAY_ROWS; r++){
        for(int c = 0; c < DISPLAY_COLS; c++){
            display[r][c] = 0;
        }
    }
    update_screen = 0;
    srand(time(NULL)); //seed rng
    
    int timer_div = TIMER_DIV;
    int numCycles = 0;
    double totalHz = 0;
    //--------------------------------------------------------------------------------------
    

    // Main game loop
    while (!WindowShouldClose() && PC < MEM_SIZE)    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------
        numCycles++;
        clock_t t1 = clock();
        ////printf("\n");
        //////printf("Memory: %x %x\n", memory[PC], memory[PC+1]); //decode for opcode
        opcode = memory[PC] << 8 | memory[PC+1];
        ////printf("PC: %x\n",PC);
        ////printf("Opcode: %x\n",opcode ); //decode for opcode
        ////printf("%x\n",PC);
        PC += 2;
        int regX = REGX(opcode);
        int regY = REGY(opcode);
        //giant case statement TODO: put all in another file
        //////printf("%x\n", opcode & 0xF000);
        switch(INSTRUCTION(opcode)){
            //00 - CLS or RET
            case 0x0000: {
                //CLS - Clear Screen
                if((opcode & 0xFF) == 0x00E0){
                    ////printf("CLS - Clear Screen\n");
                    for(int r = 0; r < DISPLAY_ROWS; r++){
                        for(int c = 0; c < DISPLAY_COLS; c++){
                            display[r][c] = 0;
                        }
                    }
                    BeginDrawing();
                        DrawRectangle(180,340,DISPLAY_COLS*PIXEL_SIZE,DISPLAY_ROWS*PIXEL_SIZE,background);
                    EndDrawing();
                    //ClearBackground(BLACK);
                    //system("clear");
                }
                //RET - Return
                else if((opcode & 0xFF) == 0x00EE){
                    ////printf("RET - Return from Subroutine\n");
                    PC = stack[SP];
                    if(SP != 0x0){
                        SP--;
                    }
                }
                break;
            }
            
            // 1nnn - JMP to addr specified by nnn
            case 0x1000: {
                PC = opcode & 0xFFF;
                ////printf("JMP to %x\n", PC);
                break;
            }
            /*
                2nnn - CALL addr
                Call subroutine at nnn.
                The interpreter increments the stack pointer, then puts the current PC on the top of the stack. The PC is then set to nnn.
            */
            case 0x2000: {
                SP += 1;
                stack[SP] = PC;
                PC = opcode & 0xFFF;
                ////printf("CALL - PC = %x\n", PC);
                break;
            }
            /*
                3xkk - SE Vx, byte
                Skip next instruction if Vx = kk.
                The interpreter compares register Vx to kk, and if they are equal, increments the program counter by 2.
            */
            case 0x3000: {
                
                if(registers[regX] == (opcode & 0xFF)){
                    PC += 2;
                }
                ////printf("SE - regX: %x Vx: %x kk: %x\n ",regX, registers[regX], opcode & 0xFF);
                break;
            }
            /*
                4xkk - SNE Vx, byte
                Skip next instruction if Vx != kk.
                The interpreter compares register Vx to kk, and if they are not equal, increments the program counter by 2.
            */
            case 0x4000: {
                
                if(registers[regX] != (opcode & 0xFF)){
                    PC += 2;
                }
                ////printf("SNE - regX: %x Vx: %x kk: %x\n ",regX, registers[regX], opcode & 0xFF);
                break;
            }
            /*
                5xy0 - SE Vx, Vy
                Skip next instruction if Vx = Vy.
                The interpreter compares register Vx to register Vy, and if they are equal, increments the program counter by 2.
            */
            case 0x5000: {
                
                if(registers[regX] == registers[regY]){
                    PC += 2;
                }
                ////printf("SE - regX: %x Vx: %x regY: %x Vy: %x\n", regX, registers[regX], regY, registers[regY]);
                break;
            }
            /*
                6xkk - LD Vx, byte
                Set Vx = kk.
                The interpreter puts the value kk into register Vx.
            */
            case 0x6000: {
                
                registers[regX] = opcode & 0xFF;
                ////printf("LD - regX: %x value: %x\n", regX, opcode & 0xFF);
                break;
            }
            /*
                7xkk - ADD Vx, byte
                Set Vx = Vx + kk.
                Adds the value kk to the value of register Vx, then stores the result in Vx.
            */
            case 0x7000: {
                
                registers[regX] = registers[regX] + (opcode & 0xFF);
                ////printf("ADD - regX: %x value: %x\n", regX, registers[regX] + (opcode & 0xFF));
                break;
            }
            //0x8xyk instructions are ALU instructions that deal with values in registers
            case 0x8000: {
                int operation = opcode & 0xF; //get the operation to be performed
                switch (operation) {
                    /*
                        8xy0 - LD Vx, Vy
                        Set Vx = Vy.
                        Stores the value of register Vy in register Vx.
                    */
                    case 0x0: {
                        registers[regX] = registers[regY];
                        ////printf("LD - regX: %x Vx: %x regY: %x Vy: %x\n", regX, registers[regX], regY, registers[regY]);
                        break;
                    }
                    /*
                        8xy1 - OR Vx, Vy
                        Set Vx = Vx OR Vy.
                        Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx. 
                    */
                    case 0x1: {
                        registers[regX] = registers[regX] | registers[regY];
                        registers[0xF] = 0;
                        ////printf("OR - regX: %x Vx: %x regY: %x Vy: %x\n", regX, registers[regX], regY, registers[regY]);
                        break;
                    }
                    /*
                        8xy2 - AND Vx, Vy
                        Set Vx = Vx AND Vy.
                        Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx. 
                    */
                    case 0x2: {
                        registers[regX] = registers[regX] & registers[regY];
                        registers[0xF] = 0;
                        ////printf("AND - regX: %x Vx: %x regY: %x Vy: %x\n", regX, registers[regX], regY, registers[regY]);
                        break;
                    }
                    /*
                        8xy3 - XOR Vx, Vy
                        Set Vx = Vx XOR Vy.
                        Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in Vx.
                    */
                    case 0x3: {
                        registers[regX] = registers[regX] ^ registers[regY];
                        registers[0xF] = 0;
                        ////printf("AND - regX: %x Vx: %x regY: %x Vy: %x\n", regX, registers[regX], regY, registers[regY]);
                        break;
                    }
                    /*
                        8xy4 - ADD Vx, Vy
                        Set Vx = Vx + Vy, set VF = carry.
                        The values of Vx and Vy are added together. 
                        If the result is greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0. 
                        Only the lowest 8 bits of the result are kept, and stored in Vx.
                    */
                    case 0x4: {
                        unsigned short result = registers[regX] + registers[regY];
                        ////printf("ADD - regX: %x Vx: %x regY: %x Vy: %x result: %x\n", regX, registers[regX], regY, registers[regY], result);
                        registers[regX] = result & 0xFF; //convert from 16 bit to 8 bit
                        if (result > 255){
                            registers[0xF] = 1;
                        }
                        else{
                           registers[0xF] = 0; 
                        }
                        break;
                    }
                    /*
                        8xy5 - SUB Vx, Vy
                        Set Vx = Vx - Vy, set VF = NOT borrow.
                        If Vx > Vy, then VF is set to 1, otherwise 0. 
                        Then Vy is subtracted from Vx, and the results stored in Vx.
                    */
                    case 0x5: {
                        char result = registers[regX] - registers[regY];
                        if (registers[regX] > registers[regY]){
                            registers[0xF] = 1;
                        }
                        else{
                            registers[0xF] = 0;
                        }
                        ////printf("SUBTRACT Vx - Vy -- regX: %x Vx: %x regY: %x Vy: %x result: %x\n", regX, registers[regX], regY, registers[regY], result);
                        registers[regX] = result;
                        break;
                    }
                    /*
                        8xy6 - SHR Vx {, Vy}
                        Set Vx = Vx SHR 1.
                        If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. 
                        Then Vx is divided by 2.
                    */
                    case 0x6: {
                        unsigned char Vx = registers[regY];
                        
                        registers[regX] = registers[regY] >> 1;
                        ////printf("SHR - regX: %x Vx: %x\n", regX, registers[regX]);
                        if ((Vx & 0x1) == 0x1){
                            registers[0xF] = 1;
                        }
                        else{
                            registers[0xF] = 0;
                        }
                        break;
                    }
                    /*
                        8xy7 - SUBN Vx, Vy
                        Set Vx = Vy - Vx, set VF = NOT borrow.
                        If Vy > Vx, then VF is set to 1, otherwise 0. 
                        Then Vx is subtracted from Vy, and the results stored in Vx.
                    */
                    case 0x7: {
                        unsigned char result = registers[regY] - registers[regX];
                        if (registers[regY] > registers[regX]){
                            registers[0xF] = 1;
                        }
                        else{
                            registers[0xF] = 0;
                        }
                        ////printf("SUBTRACT Vy - Vx -- regX: %x Vx: %x regY: %x Vy: %x result: %x\n", regX, registers[regX], regY, registers[regY], result);
                        registers[regX] = result;
                        break;
                    }
                    /*
                        8xyE - SHL Vx {, Vy}
                        Set Vx = Vx SHL 1.
                        If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. 
                        Then Vx is multiplied by 2.
                    */
                    case 0xE: {
                        unsigned char Vx = registers[regY];
                        
                        registers[regX] = registers[regY] << 1;
                        ////printf("SHL - regX: %x Vx: %x\n", regX, registers[regX]);
                        if ((Vx & 0x80) == 0x80){
                            registers[0xF] = 1;
                        }
                        else{
                            registers[0xF] = 0;
                        }
                        break;
                    }
                    default: {
                        ////printf("Unknown 0x8xyk instruction");
                        break;
                    }
                }
                break;
            }
            /*
                9xy0 - SNE Vx, Vy
                Skip next instruction if Vx != Vy.
                The values of Vx and Vy are compared, and if they are not equal, the program counter 
                is increased by 2.
            */
            case 0x9000: {
                int regX = (opcode & 0xF00) >> 8;
                int regY = (opcode & 0xF0) >> 4;
                
                if(registers[regX] != registers[regY]){
                    PC += 2;
                }
                ////printf("SNE - regX: %x Vx: %x regY: %x Vy: %x\n", regX, registers[regX], regY, registers[regY]);
                break;
            }
            /*
                Annn - LD I, addr
                Set I = nnn.
                The value of register I is set to nnn.
            */
            case 0xA000: {
                I = opcode & 0xFFF;
                ////printf("LD I - addr: %x opcode: %x\n", I, opcode);
                break;
            }
            /*
                Bnnn - JP V0, addr
                Jump to location nnn + V0.
                The program counter is set to nnn plus the value of V0.
            */
            case 0xB000: {
                PC = (opcode & 0xFFF) + registers[0x0];
                ////printf("JP V0, addr - V0: %x, addr: %x\n", registers[0x0], opcode & 0xFFF);
                break;
            }
            /*
                Cxkk - RND Vx, byte
                Set Vx = random byte AND kk.
                The interpreter generates a random number from 0 to 255, 
                which is then ANDed with the value kk. The results are stored in Vx. 
            */
            case 0xC000: {
                int regX = (opcode & 0xF00) >> 8;
                unsigned char byte = opcode & 0xFF;
                unsigned char randByte = rand() % 255;
                registers[regX] = randByte & byte;
                ////printf("RND - regX: %x, Vx: %x\n", regX, registers[regX]);
                break;
            }
            /*
                Dxyn - DRW Vx, Vy, nibble
                Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
                The interpreter reads n bytes from memory, starting at the address stored in I. 
                These bytes are then displayed as sprites on screen at coordinates (Vx, Vy).
                Sprites are XORed onto the existing screen. If this causes any pixels to be erased, 
                VF is set to 1, otherwise it is set to 0. 
                If the sprite is positioned so part of it is outside the coordinates of the display,
                it wraps around to the opposite side of the screen.
            */
            case 0xD000: {
                unsigned char Vx = registers[regX] % DISPLAY_COLS;
                unsigned char Vy = registers[regY] % DISPLAY_ROWS;
                unsigned char nBytes = opcode & 0xF;
                ////printf("x: %d y:%d \n", Vx, Vy);
                //read n-bytes from memory
                
                registers[0xF] = 0;
                //draw
                //assume Vx and Vy are 0
                for(int row = Vy; row < nBytes+Vy; row++){
                    if(row >= DISPLAY_ROWS){
                        ////printf("Row Wrap\n");
                        break;
                    }
                    unsigned char line = memory[(row - Vy)+I];//sprite[row - Vy];
                    ////printf("Row: %d\n",row);
                    ////printf("Line: %x\n",line);
                    for(int col = Vx; col < 8+Vx; col++){
                        //col mod 8 is index into char
                        //row is index into sprite
                        if(col >= DISPLAY_COLS){
                            ////printf("Col Wrap\n");
                            break;
                        }
                        unsigned char bit = (line & (0x80 >> ((col-Vx)%8))) >> (7-((col-Vx)%8));
                        ////printf("%x",bit);
                        if((display[row][col] ^ bit) == 1){
                            display[row][col] = 1;
                            update_screen = 1;
                        }
                        else{
                            //the xor has caused a pixel to dissapear
                            if(display[row][col] == 1){
                                registers[0xF] = 1;
                                update_screen = 1;
                            }
                            display[row][col] = 0;
                            //DrawRectangle(col*10,row*10,10,10,BLACK);
                        }
                        ////printf("%x",display[row][col]);
                    }
                   ////printf("\n");
                    
                }
                ////printf("DRW\n");
                break;
            }
            /*
                Ex9E - SKP Vx
                Skip next instruction if key with the value of Vx is pressed.
                Checks the keyboard, and if the key corresponding to the value of Vx is currently in the 
                down position, PC is increased by 2.
                ExA1 - SKNP Vx
                Skip next instruction if key with the value of Vx is not pressed.
                Checks the keyboard, and if the key corresponding to the value of Vx is currently in the
                up position, PC is increased by 2.
            */
            case 0xE000: {
                unsigned char operation = (opcode & 0xFF);
                unsigned char Vx = registers[regX];
                //Vx pressed
                if(operation == 0x9E){
                    if(keyboard[Vx] == 1){
                        PC += 2;
                    }
                }
                //Vx not pressed
                if(operation == 0xA1){
                    if(keyboard[Vx] == 0 || keyboard[Vx] == 2){
                        PC += 2;
                    }
                }
                break;
            }
            //0xF--- instructions
            case 0xF000: {
                ////printf("F: %x\n", opcode);
                unsigned short operation = (opcode & 0xFF);
                unsigned char Vx = registers[regX];
                switch(operation){
                    /*
                        Fx07 - LD Vx, DT
                        Set Vx = delay timer value.
                        The value of DT is placed into Vx.
                    */
                    case 0x07: {
                        registers[regX] = delay_timer;
                        break;
                    }
                    /*
                        Fx0A - LD Vx, K
                        Wait for a key press, store the value of the key in Vx.
                        All execution stops until a key is pressed, then the value of that key is stored 
                        in Vx.
                    */
                    case 0x0A: {
                        key_pressed = 0;
                        for(int i = 0; i < 16; i++){
                            if(keyboard[i] == 2){
                                ////printf("HFUHUHUHU\n\n");
                                registers[regX] = i;
                                key_pressed = 1;
                                keyboard[i] = 0;
                            }
                        }
                        if(key_pressed == 0){
                            PC -= 2;
                        }
                        break;
                        /*
                        if(key_pressed == 0){
                            PC -= 2; //PC automatically inc by 2 so dec by 2 to stay at the same PC
                        }
                        else{
                            registers[regX] = key_pressed - 1; //-1 bc how the key_pressed is stored 
                        }
                        break;*/
                    }
                    /*
                        Fx15 - LD DT, Vx
                        Set delay timer = Vx.
                        DT is set equal to the value of Vx.
                    */
                    case 0x15: {
                        delay_timer = Vx;
                        break;
                    }
                    /*
                    Fx18 - LD ST, Vx
                    Set sound timer = Vx.
                    ST is set equal to the value of Vx.
                    */
                    case 0x18: {
                        sound_timer = Vx;
                        break;
                    }
                    /*
                        Fx1E - ADD I, Vx
                        Set I = I + Vx.
                        The values of I and Vx are added, and the results are stored in I.
                    */
                    case 0x1E: {
                        unsigned short sum = I + Vx;
                        //add overflow check here (might not be needed?)
                        I = sum;
                        break;
                    }
                    /*
                        Fx29 - LD F, Vx
                        Set I = location of sprite for digit Vx.
                        The value of I is set to the location for the hexadecimal sprite corresponding 
                        to the value of Vx. 
                    */
                    case 0x29: {
                        //each digit is 5 bytes long
                        unsigned char sprite_location = Vx * 5;
                        I = sprite_location;
                        break;
                    }
                    /*
                        Fx33 - LD B, Vx
                        Store BCD representation of Vx in memory locations I, I+1, and I+2.
                        The interpreter takes the decimal value of Vx, and 
                        places the hundreds digit in memory at location in I, 
                        the tens digit at location I+1, and the ones digit at location I+2.
                    */
                    case 0x33: {
                        memory[I] = Vx / 100;
                        memory[I + 1] = (Vx / 10) % 10;
                        memory[I + 2] = Vx % 10;
                        break;
                    }
                    /*
                        Fx55 - LD [I], Vx
                        Store registers V0 through Vx in memory starting at location I.
                        The interpreter copies the values of registers V0 through Vx into memory, 
                        starting at the address in I.
                    */
                    case 0x55: {
                        for(int i = 0; i <= regX; i++){
                            memory[I + i] = registers[i];
                        }
                        I += regX + 1;
                        break;
                    }
                    /*
                        Fx65 - LD Vx, [I]
                        Read registers V0 through Vx from memory starting at location I.
                        The interpreter reads values from memory starting at location I into 
                        registers V0 through Vx.
                    */
                    case 0x65: {
                        for(int i = 0; i <= regX; i++){
                            registers[i] = memory[I + i];
                        }
                        I += regX + 1;
                        break;
                    }
                }
                break;
            }
            default: {
                //printf("Unknown: %x",PC);
                break;
            }
        }
        
        // Draw
        //---------------------------------------------------------------------------------
        
        if(update_screen == 1 && timer_div == 0){
            BeginDrawing();
             
            DrawRectangle(179,339,642,322,foreground);
            DrawRectangle(180,340,640,320,background);
            for(int r = 0; r < DISPLAY_ROWS; r++){
                for(int c = 0; c < DISPLAY_COLS; c++){
                    if(display[r][c] == 1){
                        DrawRectangle((PIXEL_SIZE*c)+COL_OFFSET,(PIXEL_SIZE*r)+ROW_OFFSET,PIXEL_SIZE,PIXEL_SIZE,foreground);
                    }
                    else{
                        DrawRectangle((PIXEL_SIZE*c)+COL_OFFSET,(PIXEL_SIZE*r)+ROW_OFFSET,PIXEL_SIZE,PIXEL_SIZE,background);
                    }
                }
            }
            update_screen = 0;
            EndDrawing();
        }
        PollInputEvents();
        
    
        
        //----------------------------------------------------------------------------------
        if(timer_div == 0) {
            ////printf("%d\n",timer_div);
            timer_div = TIMER_DIV;
            if(delay_timer > 0){
                delay_timer--;
            }
            if(sound_timer > 0){
                sound_timer--;
                PlaySound(beep); 
            }
            
            /*
            if(update_screen == 1){
                system("clear");
                for(int r = -1; r <= DISPLAY_ROWS; r++){
                    for(int c = -1; c <= DISPLAY_COLS; c++){
                        if(r == -1 || r == DISPLAY_ROWS){
                            printf("-");
                        }
                        else if(c == -1 || c == DISPLAY_COLS){
                            printf("|");
                        }

                        else if(display[r][c] == 1)
                            printf("#");
                        else
                        printf(" "); 
                    }
                    printf("\n");
                }
            }
            //update keys pressed
            char c;
            if(kbhit()){
                c = getch();
                for(int i = 0; i < 16; i++){
                    if(c == KEY_MAP[i]){
                        keyboard[i] = 1;
                    }
                    else{
                        keyboard[i] = 0;
                    }
                    printf("%x",keyboard[i]);
                }
            }
            else{
                for(int i = 0; i < 16; i++){
                    keyboard[i] = 0;
                }
            }*/
            
            
        }
        else{
            timer_div--;
            
        }
        
        //printf("\n");
        //printf("%x\n",keyboard[0]);
        for(int i = 0; i < 16; i++){
            //printf("%x ",keyboard[i]);
            ////printf("%x ", IsKeyPressed(KEY_MAP[i]));
            if(IsKeyDown(KEY_MAP[i])){
                //printf("Key Pressed: %i\n",i);
                //key_pressed = 1; 
                keyboard[i] = 1;
            }
            if(IsKeyUp(KEY_MAP[i])){
                
                //key_pressed = 0;
                
                if(keyboard[i] == 1){
                    //printf("Key Released: %i\n",i);
                    keyboard[i] = 2;
                }
                else{
                    keyboard[i] = 0;
                }
                
            }
        }
        ////printf("\n");
       
        
        /*for(int i = 0; i < 16; i++){
            
        }*/
        //printf("\n");
        //wait for next cycle
        
    usleep(CPU_DELAY); //1 millisecond
         
 
    
 
    
    clock_t t2 = clock();
 
    double dur = CLOCKS_PER_SEC/((double)(t2-t1));
    totalHz += dur;
 
    printf("%.2f Hz ", dur);
    printf("Avg: %.1f Hz\n",totalHz/numCycles);
    
    }
    UnloadSound(beep);     // Unload sound data
    

    CloseAudioDevice();     // Close audio device
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    
    return 0;
}

//Map a string to the corresponding color
//Generated by ChatGPT
Color mapStringToColor(char* color, char colorType){

    if (strcmp(color,"LIGHTGRAY") == 0) {
        return LIGHTGRAY;
    }
    if (strcmp(color,"GRAY") == 0) {
        return GRAY;
    }
    if (strcmp(color,"DARKGRAY") == 0) {
        return DARKGRAY;
    }
    if (strcmp(color,"YELLOW") == 0) {
        return YELLOW;
    }
    if (strcmp(color,"GOLD") == 0) {
        return GOLD;
    }
    if (strcmp(color,"ORANGE") == 0) {
        return ORANGE;
    }
    if (strcmp(color,"PINK") == 0) {
        return PINK;
    }
    if (strcmp(color,"RED") == 0) {
        return RED;
    }
    if (strcmp(color,"MAROON") == 0) {
        return MAROON;
    }
    if (strcmp(color,"GREEN") == 0) {
        return GREEN;
    }
    if (strcmp(color,"LIME") == 0) {
        return LIME;
    }
    if (strcmp(color,"DARKGREEN") == 0) {
        return DARKGREEN;
    }
    if (strcmp(color,"SKYBLUE") == 0) {
        return SKYBLUE;
    }
    if (strcmp(color,"BLUE") == 0) {
        return BLUE;
    }
    if (strcmp(color,"DARKBLUE") == 0) {
        return DARKBLUE;
    }
    if (strcmp(color,"PURPLE") == 0) {
        return PURPLE;
    }
    if (strcmp(color,"VIOLET") == 0) {
        return VIOLET;
    }
    if (strcmp(color,"DARKPURPLE") == 0) {
        return DARKPURPLE;
    }
    if (strcmp(color,"BEIGE") == 0) {
        return BEIGE;
    }
    if (strcmp(color,"BROWN") == 0) {
        return BROWN;
    }
    if (strcmp(color,"DARKBROWN") == 0) {
        return DARKBROWN;
    }
    if (strcmp(color,"WHITE") == 0) {
        return WHITE;
    }
    if (strcmp(color,"BLACK") == 0) {
        return BLACK;
    }
    if (strcmp(color,"MAGENTA") == 0) {
        return MAGENTA;
    }
    if (strcmp(color,"RAYWHITE") == 0) {
        return RAYWHITE;
    }

    printf("%s Not a Valid Color, rerun with -help to get list of colors\n", color);
    printf("Setting background to BLACK and foreground to WHITE\n");
    if(colorType == 'B'){
        return BLACK;
    }
    else{
        return WHITE;
    }
    return BLANK;
}

        
    

/*
LIGHTGRAY 
GRAY      
DARKGRAY  
YELLOW    
GOLD      
ORANGE    
PINK      
RED       
MAROON    
GREEN     
LIME      
DARKGREEN 
SKYBLUE   
BLUE      
DARKBLUE  
PURPLE    
VIOLET    
DARKPURPLE
BEIGE     
BROWN     
DARKBROWN 
WHITE     
BLACK     
BLANK     
MAGENTA   
RAYWHITE  
*/