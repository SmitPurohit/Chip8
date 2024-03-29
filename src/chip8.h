
#include <raylib.h>
#include <stdint.h>

#define MEM_SIZE 4096
#define PROGRAM_START 512
#define FONT_END 80
#define DISPLAY_ROWS 32
#define DISPLAY_COLS 64
#define PIXEL_SIZE 10
#define ROW_OFFSET 340
#define COL_OFFSET 180
#define CPU_DELAY 1000
#define TIMER_DIV 9
#define NUM_REGISTERS 16
#define INSTRUCTION(opcode) opcode & 0xF000
#define REGX(opcode) (opcode & 0xF00) >> 8
#define REGY(opcode) (opcode & 0xF0) >> 4
#define NUM_COLORS 25
#define NUM_CONTROL_KEYS 1

Color mapStringToColor(char* color, char colorType);
char* colorList[NUM_COLORS] = { "LIGHTGRAY", "GRAY", "DARKGRAY", "YELLOW", "GOLD", "ORANGE", "PINK", "RED", "MAROON", "GREEN", "LIME", "DARKGREEN", "SKYBLUE", "BLUE", "DARKBLUE", "PURPLE", "VIOLET", "DARKPURPLE", "BEIGE", "BROWN", "DARKBROWN", "WHITE", "BLACK", "MAGENTA", "RAYWHITE" };


uint8_t KEY_MAP[16] = 
{
    KEY_X,   //0
    KEY_ONE, //1
    KEY_TWO, //2
    KEY_THREE, //3
    KEY_Q, //4
    KEY_W, //5
    KEY_E, //6
    KEY_A, //7
    KEY_S, //8
    KEY_D, //9
    KEY_Z, //A
    KEY_C, //B
    KEY_FOUR, //C
    KEY_R, //D
    KEY_F, //E
    KEY_V, //F
};

//Set the size of this to however many control buttons there are
//0: Pause/Play

uint8_t CONTROL_KEY_MAP[NUM_CONTROL_KEYS] = {KEY_P};
/*unsigned char KEY_MAP[16] = 
{
    'x',   //0
    '1', //1
    '2', //2
    '3', //3
    'q', //4
    'w', //5
    'e', //6
    'a', //7
    's', //8
    'd', //9
    'z', //A
    'c', //B
    '4', //C
    'r', //D
    'f', //E
    'v', //F
};*/
uint8_t fontset[FONT_END] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

uint8_t display[DISPLAY_ROWS][DISPLAY_COLS];
uint8_t rom_buffer[MEM_SIZE];
uint8_t memory[MEM_SIZE];
uint16_t opcode;
uint8_t  registers[NUM_REGISTERS];
uint16_t I;
uint8_t delay_timer;
uint8_t sound_timer;
uint16_t PC;
uint16_t SP;
uint16_t stack[16];
uint8_t keyboard[16];
uint8_t key_pressed;
uint8_t update_screen;
uint8_t display_interrupt;
uint8_t paused = 0;