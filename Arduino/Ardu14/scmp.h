/********************************************************************/
/*																                                	*/
/*						Portable MK14 emulator in 'C'			                  	*/
/*																	                                */
/*							Global Include File				                      		*/
/*																                                 	*/
/********************************************************************/

#define DIGITS (9) /* LED Digit Count */

/********************************************************************/
/*		The External Interface - Porting for various versions		*/
/********************************************************************/

#define KEY_DIGIT (n)   /* 0-9,A-F are 0x00-0x0F */
#define KEY_TERM (0x10) /* Key codes for the other keys */
#define KEY_ABORT (0x11)
#define KEY_GO (0x12)
#define KEY_MEM (0x13)
#define KEY_BREAK (0x14)
#define KEY_RESET (0x15)

void CONInitialise(void);  /* Initialise the console */
void CONTerminate(void);   /* Close down the console */
void CONWrite(char);       /* Write a character */
char CONRead(void);        /* Read a character */
int CONKeyPressed(int);    /* Is a key pressed ? */
void CONDrawLED(int, int); /* Draw LED (0-7) of given pattern */
void CONSynchronise(long); /* CPU Speed Synchronise */

/* Supported characters are standard 7 bit ASCII, 13 (CR) & 8 (BS) 	*/
/* CR on CONWrite does CR/LF */

/********************************************************************/
/*						Emulator Global Includes				                    	*/
/********************************************************************/

extern int Acc, Ext, Stat;        /* SC/MP 8 bit Registers */
extern int Ptr[4];                /* SC/MP 16 bit Registers */
extern unsigned char Memory[256]; /* SC/MP Main Memory */
extern long Cycles;               /* CPU Cycle Counter */

extern int debugCounter; /* CPU Cycle Counter */


void MinimalistEmulator(char *); /* All that's needed to emulate.. */
int LoadObject(char *);          /* Load file into memory */
int ReadMemory(int);             /* Read a byte from memory */
void WriteMemory(int, int);      /* Write a byte to memory */
void ResetCPU(void);             /* Reset the CPU */
void Execute(int);               /* Execute instructions */
void BlockExecute(void);         /* Execute an instruction block */
//int  LoadROM(void);					/* Load the SCIOS ROM into memory */
int LoadROM(int version);     /* Load the SCIOS ROM into memory */
void OutStr(char *Text);      /* Output a String */
void InitialiseDisplay(void); /* Initialise the display */
void Latency(void);           /* Latency test */


/* This does the 12 bit ptr add. Basically there is no carry from bit   */
/* 11 into bit 12, so bits 12..15 are always unchanged					*/

#define ADD12(Ptr, Ofs) ((((Ptr) + (Ofs)) & 0xFFF) | ((Ptr)&0xF000))

/* There are two fetches. The second is more accurate but slower... the */
/* first doesn't increment the PC correctly (12 bit fashion). In		*/
/* practice the faster one is functionally equivalent					*/

// #define FETCH(Tgt) Tgt = Memory[(++Ptr[0]) & 0xFFF]

/* #define FETCH(Tgt) { Ptr[0] = ADD12(Ptr[0],1);Tgt = Memory[Ptr[0]]; } */
