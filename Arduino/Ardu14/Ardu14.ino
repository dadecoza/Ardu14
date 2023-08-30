
/********************************************************************/
/* Ardu14, a Science of Cambridge MK14 emulator for Arduino         */
/*                                                                  */
/*     Portable MK14 emulator in 'C'                                */
/*                                                                  */
/*     * ported to Arduino Johannes le Roux                         */
/*     * based on Doug Rice's Virtual MK14                          */
/*     * based on Virtual Nascom, a Nascom II emulator.             */
/*     * based on Paul Robson's MK14 simulator for DOS              */
/*         Copyright (C) 2000,2009,2017  Tommy Thorn                */
/*         Copyright (C) 2000,2009,2017  Paul Robson                */
/*         Copyright (C) 2017  Doug Rice                            */
/*         Copyright (C) 2023 Johannes le Roux                      */
/*                                                                  */
/********************************************************************/

/********************************************************************/
/*                                                                  */
/*                        Portable MK14 emulator in 'C'             */
/*                                                                  */
/*                                CPU Emulator                      */
/*                                                                  */
/********************************************************************/
#include "scmp.h"
#include "scios.h"
#include <LiquidCrystal.h>
#include <Keypad.h>

int Acc, Ext, Stat; /* SC/MP CPU Registers */
int Ptr[4];
unsigned char Memory[256], OptionMemory[256]; /* SC/MP Program Memory */
unsigned char DisplayBuffer[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned char Key;
long Cycles;                 /* Cycle Count */
long executeTimer = 0;       /* Track time to execute */
const int executeSpeed = 10; /* Amount of microseconds before next instruction */

static int Indexed(int); /* Local prototypes */
/* bug in Paul Robson's emulator JMPS with offset 80 do not use E .*/
static int IndexedJmp(int); /* Local prototypes */
static int AutoIndexed(int);
static int BinAdd(int, int);
static int DecAdd(int, int);

int hexLoaderState, hexCounter, hexBytes, hexAddrHi, hexAddr, hexSum, hexTmp = 0; /* Variables for hex loader */

const uint8_t segments[7][8] = { /* bitmaps of 7segments */
                                 { 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                                 { 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
                                 { 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01 },
                                 { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f },
                                 { 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10 },
                                 { 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00 },
                                 { 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00 }
};

/********************************************************************/
/* Configure LCD and GPIO                                           */
/********************************************************************/

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int activeDigit = 0;                                  // Track the last active character
int digitPersistance[] = { 0, 0, 0, 0, 0, 0, 0, 0 };  // character persistance timer, will count down from "persistanceTimeout" and will reset when addressed
const int persistanceTimeout = 300;                   // Number of cycles a character will be persistant without refresh

/********************************************************************/
/* Configure keypad layout and GPIO                                 */
/********************************************************************/

const byte ROWS = 5;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  { 'g', 'm', 'z', 'a' },
  { '7', '8', '9', 'b' },
  { '4', '5', '6', 'c' },
  { '1', '2', '3', 'd' },
  { 't', '0', 'f', 'e' }
};
byte rowPins[ROWS] = { 10, A0, A1, A2, A3 };  // connect to the row pinouts of the keypad
byte colPins[COLS] = { 6, 7, 8, 9 };          // connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

/********************************************************************/
/*              Setup hardware and initialze CPU                    */
/********************************************************************/

void setup() {
  initializeDisplay();
  Serial.begin(9600);
  ResetCPU();
  Serial.println("Ardu14 - Science of Cambridge MK14 emulator\n");
  Serial.println("Ready.");
}

/********************************************************************/
/* Main loop                                                        */
/********************************************************************/

void loop() {
  if (Serial.available() > 0) {
    int b = Serial.read();
    if (b == ':') {
      hexLoaderState = 1;
      hexCounter = 0;
    } else if (hexLoaderState == 1) {
      hexLoader(b);
    }
  } else {
    char keypress = keypad.getKey();
    if (keypress)
      Key = keypress;
    executeTimedInstruction();
    updateDigitPersistance();
  }
}

/********************************************************************/
/* Initialize Display                                               */
/********************************************************************/
void initializeDisplay() {
  lcd.begin(16, 2);
  lcd.clear();
  const unsigned char nochar[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  for (int i = 0; i < 8; i++) {
    lcd.createChar(i, nochar);
  }
}

/********************************************************************/
/* Update Display                                                   */
/********************************************************************/

void updateDisplay() {
  lcd.setCursor(4, 0);
  for (int i = 0; i < 8; i++) {
    if (DisplayBuffer[i] == 0) {
      lcd.write(' ');
    } else {
      lcd.write(i);
    }
  }
}


/********************************************************************/
/* Update Digit Persistance                                         */
/********************************************************************/

void updateDigitPersistance() {
  digitPersistance[activeDigit] = persistanceTimeout;
  for (int i = 0; i < 8; i++) {
    if (digitPersistance[i]-- <= 0) {
      DisplayBuffer[i] = 0;
      digitPersistance[i] = persistanceTimeout;
      updateDisplay();
      return;
    }
  }
}

/********************************************************************/
/* Write to display buffer                                          */
/********************************************************************/

void printDigit(char c, int pos) {
  activeDigit = pos;
  if (DisplayBuffer[pos] == c)
    return;

  uint8_t segment[8], i, n, cc;
  for (int i = 0; i < 8; i++) {
    byte cc = 0;
    for (int n = 0; n < 7; n++) {
      if (c & (1 << n))
        cc |= segments[n][i];
    }
    segment[i] = cc;
  }
  lcd.createChar(pos, segment);
  DisplayBuffer[pos] = c;
  updateDisplay();
}


/********************************************************************/
/* Reset the CPU                                                    */
/********************************************************************/

void ResetCPU(void) {
  Acc = Ext = Stat = 0; /* Zero all registers */
  Cycles = 0L;
  Key = 0;
  Ptr[0] = Ptr[1] = Ptr[2] = Ptr[3] = 0;
}

/********************************************************************/
/* Execute a given number of instructions                           */
/********************************************************************/

#define CYC(n) Cycles += (long)(n) /* Bump the cycle counter */

/* Shorthand for multiple case */
#define CAS4(n) \
  case n: \
  case n + 1: \
  case n + 2: \
  case n + 3
#define CAS3(n) \
  case n: \
  case n + 1: \
  case n + 2

#define CM(n) ((n) ^ 0xFF) /* 1's complement */

void Execute(int Count) {
  register int Opcode;
  register int Pointer;
  int n, tmp;
  long l;

  while (Count-- > 0) {
    fetch(&Opcode);       /* Fetch the opcode, hack of the */
    Pointer = Opcode & 3; /* pointer reference */
    switch (Opcode)       /* Pointer instructions first */
    {
      /* LD (Load) */
      CAS4(0xC0)
        : Acc = ReadMemory(Indexed(Pointer));
      CYC(18);
      break;
      case 0xC4:
        fetch(&Acc);
        CYC(10);
        break;
        CAS3(0xC5)
          : Acc = ReadMemory(AutoIndexed(Pointer));
        CYC(18);
        break;
      case 0x40:
        Acc = Ext;
        CYC(6);
        break;
        /* ST (Store) */
        CAS4(0xC8)
          : WriteMemory(Indexed(Pointer), Acc);
        CYC(18);
        break;
        CAS3(0xCD)
          : WriteMemory(AutoIndexed(Pointer), Acc);
        CYC(18);
        break;

        /* AND (And) */
        CAS4(0xD0)
          : Acc = Acc & ReadMemory(Indexed(Pointer));
        CYC(18);
        break;
      case 0xD4:
        fetch(&n);
        Acc = Acc & n;
        CYC(10);
        break;
        CAS3(0xD5)
          : Acc = Acc & ReadMemory(AutoIndexed(Pointer));
        CYC(18);
        break;
      case 0x50:
        Acc = Acc & Ext;
        CYC(6);
        break;

        /* OR (Or) */
        CAS4(0xD8)
          : Acc = Acc | ReadMemory(Indexed(Pointer));
        CYC(18);
        break;
      case 0xDC:
        fetch(&n);
        Acc = Acc | n;
        CYC(10);
        break;
        CAS3(0xDD)
          : Acc = Acc | ReadMemory(AutoIndexed(Pointer));
        CYC(18);
        break;
      case 0x58:
        Acc = Acc | Ext;
        CYC(6);
        break;

        /* XOR (Xor) */
        CAS4(0xE0)
          : Acc = Acc ^ ReadMemory(Indexed(Pointer));
        CYC(18);
        break;
      case 0xE4:
        fetch(&n);
        Acc = Acc ^ n;
        CYC(10);
        break;
        CAS3(0xE5)
          : Acc = Acc ^ ReadMemory(AutoIndexed(Pointer));
        CYC(18);
        break;
      case 0x60:
        Acc = Acc ^ Ext;
        CYC(6);
        break;

        /* DAD (Dec Add) */
        CAS4(0xE8)
          : Acc = DecAdd(Acc, ReadMemory(Indexed(Pointer)));
        CYC(23);
        break;
      case 0xEC:
        fetch(&n);
        Acc = DecAdd(Acc, n);
        CYC(15);
        break;
        CAS3(0xED)
          : Acc = DecAdd(Acc, ReadMemory(AutoIndexed(Pointer)));
        CYC(23);
        break;
      case 0x68:
        Acc = DecAdd(Acc, Ext);
        CYC(11);
        break;

        /* ADD (Add) */
        CAS4(0xF0)
          : Acc = BinAdd(Acc, ReadMemory(Indexed(Pointer)));
        CYC(19);
        break;
      case 0xF4:
        fetch(&n);
        CYC(11);
        Acc = BinAdd(Acc, n);
        break;
        CAS3(0xF5)
          : CYC(19);
        Acc = BinAdd(Acc, ReadMemory(AutoIndexed(Pointer)));
        break;
      case 0x70:
        Acc = BinAdd(Acc, Ext);
        CYC(7);
        break;

        /* CAD (Comp Add) */
        CAS4(0xF8)
          : Acc = BinAdd(Acc, CM(ReadMemory(Indexed(Pointer))));
        CYC(20);
        break;
      case 0xFC:
        fetch(&n);
        CYC(12);
        Acc = BinAdd(Acc, CM(n));
        break;
        CAS3(0xFD)
          : Acc = BinAdd(Acc, CM(ReadMemory(AutoIndexed(Pointer))));
        CYC(20);
        break;
      case 0x78:
        Acc = BinAdd(Acc, CM(Ext));
        CYC(8);
        break;

        CAS4(0x30)
          : /* XPAL */
            n = Ptr[Pointer];
        CYC(8);
        Ptr[Pointer] = (n & 0xFF00) | Acc;
        Acc = n & 0xFF;
        break;
        CAS4(0x34)
          : /* XPAH */
            n = Ptr[Pointer];
        CYC(8);
        Ptr[Pointer] = (n & 0xFF) | (Acc << 8);
        Acc = (n >> 8) & 0xFF;
        break;
        CAS4(0x3C)
          : /* XPPC */
            n = Ptr[Pointer];
        Ptr[Pointer] = Ptr[0];
        Ptr[0] = n;
        CYC(7);
        break;

        CAS4(0x90)
          : /* Jumps */
            CYC(11);
        Ptr[0] = IndexedJmp(Pointer);
        break;
        CAS4(0x94)
          : CYC(11);
        n = IndexedJmp(Pointer);
        if ((Acc & 0x80) == 0)
          Ptr[0] = n;
        break;
        CAS4(0x98)
          : CYC(11);
        n = IndexedJmp(Pointer);
        if (Acc == 0)
          Ptr[0] = n;
        break;
        CAS4(0x9C)
          : CYC(11);
        n = IndexedJmp(Pointer);
        if (Acc != 0)
          Ptr[0] = n;
        break;

        CAS4(0xA8)
          : /* ILD and DLD */
            n = Indexed(Pointer);
        Acc = (ReadMemory(n) + 1) & 0xFF;
        CYC(22);
        WriteMemory(n, Acc);
        break;

        CAS4(0xB8)
          : n = Indexed(Pointer);
        Acc = (ReadMemory(n) - 1) & 0xFF;
        CYC(22);
        WriteMemory(n, Acc);
        break;

      case 0x8F: /* DLY */
        fetch(&n);
        l = ((long)n) & 0xFFL;
        l = 514L * l + 13 + Acc;
        Acc = 0xFF;
        CYC(l);
        break;
      case 0x01: /* XAE */
        n = Acc;
        Acc = Ext;
        Ext = n;
        break;
      case 0x19: /* SIO */
        CYC(5);
        Ext = (Ext >> 1) & 0x7F;
        break;
      case 0x1C: /* SR */
        CYC(5);
        Acc = (Acc >> 1) & 0x7F;
        break;
      case 0x1D: /* SRL */
        Acc = (Acc >> 1) & 0x7F;
        CYC(5);
        Acc = Acc | (Stat & 0x80);
        break;
      case 0x1E: /* RR */
        n = Acc;
        Acc = (Acc >> 1) & 0x7F;
        if (n & 0x1)
          Acc = Acc | 0x80;
        CYC(5);
        break;
      case 0x1F: /* RRL */
        n = Acc;
        Acc = (Acc >> 1) & 0x7F;
        if (Stat & 0x80)
          Acc = Acc | 0x80;
        Stat = Stat & 0x7F;
        if (n & 0x1)
          Stat = Stat | 0x80;
        CYC(5);
        break;
      case 0x00: /* HALT */
        CYC(8);
        break;
      case 0x02: /* CCL */
        Stat &= 0x7F;
        CYC(5);
        break;
      case 0x03: /* SCL */
        Stat |= 0x80;
        CYC(5);
        break;
      case 0x04: /* DINT */
        Stat &= 0xF7;
        CYC(6);
        break;
      case 0x05: /* IEN */
        Stat |= 0x08;
        CYC(6);
        break;
      case 0x06: /* CSA */
        Acc = Stat;
        CYC(5);
        break;
      case 0x07: /* CAS */
        Stat = Acc & 0xCF;
        CYC(6);
        break;
      case 0x08: /* NOP */
        break;
    }
  }
}

/********************************************************************/
/* Execute timed instruction                                        */
/********************************************************************/

void executeTimedInstruction() {
  long m = micros();
  if (executeTimer < m) {
    executeTimer = m + executeSpeed;
    Execute(1);
  }
}

/********************************************************************/
/* Decimal Add                                                      */
/********************************************************************/

static int DecAdd(int v1, int v2) {
  int n1 = (v1 & 0xF) + (v2 & 0xF);   /* Add LSB */
  int n2 = (v1 & 0xF0) + (v2 & 0xF0); /* Add MSB */
  if (Stat & 0x80)
    n1++;             /* Add Carry */
  Stat = Stat & 0x7F; /* Clear CYL */
  if (n1 > 0x09)      /* Digit 1 carry ? */
  {
    n1 = n1 - 0x0A;
    n2 = n2 + 0x10;
  }
  n1 = (n1 + n2);
  if (n1 > 0x99) /* Digit 2 carry ? */
  {
    n1 = n1 - 0xA0;
    Stat = Stat | 0x80;
  }
  return (n1 & 0xFF);
}

/********************************************************************/
/* Binary Add                                                       */
/********************************************************************/

#define SGN(x) ((x)&0x80)

static int BinAdd(int v1, int v2) {
  int n;
  n = v1 + v2 + ((Stat & 0x80) ? 1 : 0); /* Add v1,v2 and carry */
  Stat = Stat & 0x3F;                    /* Clear CYL and OV */
  if (n & 0x100)
    Stat = Stat | 0x80;     /* Set CYL if required */
  if (SGN(v1) == SGN(v2) && /* Set OV if required */
      SGN(v1) != SGN(n))
    Stat |= 0x40;
  return (n & 0xFF);
}

/********************************************************************/
/* Indexing Mode                                                    */
/********************************************************************/

static int Indexed(int p) {
  int Offset;
  fetch(&Offset); /* Get offset */
  if (Offset == 0x80)
    Offset = Ext; /* Using 'E' register ? */
  if (Offset & 0x80)
    Offset = Offset - 256;        /* Sign extend */
  return (ADD12(Ptr[p], Offset)); /* Return result */
}

/********************************************************************/
/* Indexing Mode for Jumps                                          */
/********************************************************************/

static int IndexedJmp(int p) {
  int Offset;
  fetch(&Offset);                         /* Get offset */
  /* if (Offset == 0x80) Offset = Ext; */ /* Using 'E' register ? */
  if (Offset & 0x80)
    Offset = Offset - 256;        /* Sign extend */
  return (ADD12(Ptr[p], Offset)); /* Return result */
}

/********************************************************************/
/* Auto-indexing mode                                               */
/********************************************************************/

static int AutoIndexed(int p) {
  int Offset, Address;
  fetch(&Offset); /* Get offset */
  if (Offset == 0x80)
    Offset = Ext; /* Using E ? */
  if (Offset & 0x80)
    Offset = Offset - 256; /* Sign extend */
  if (Offset < 0)          /* Pre decrement on -ve offset */
    Ptr[p] = ADD12(Ptr[p], Offset);
  Address = Ptr[p]; /* The address we're using */
  if (Offset > 0)   /* Post increment on +ve offset */
    Ptr[p] = ADD12(Ptr[p], Offset);
  return (Address);
}

/********************************************************************/
/* Read a memory location                                           */
/********************************************************************/

int ReadMemory(int Address) {
  int n = Address & 0x0F00;
  int c;
  if (n < 0x800) {
    c = pgm_read_byte_near(scios + (Address % 0x200));
    c = c & 0xFF;
    return c;
  } else if (n == 0xB00) {
    return (OptionMemory[Address & 0xFF]);
  } else if (n == 0x900 || n == 0xD00) { /* Handle I/O at 900 or D00 */
    n = Address & 0x0F;                  /* Digit select latch value */
    return ReadKeyboard(n);
  } else if (n == 0xF00) {
    return (Memory[Address & 0xFF]);
  } else {
    return 0;
  }
}

/********************************************************************/
/* Write a memory location                                          */
/********************************************************************/

void WriteMemory(int Address, int Data) {
  int n = Address & 0x0F00;     /* Find out which page */
  if (n == 0x900 || n == 0xD00) /* Writing to I/O */
  {
    n = Address & 0xF;
    if (n < 8)
      printDigit(Data, 7 ^ n);
  } else if (n == 0xB00) {
    OptionMemory[Address & 0xFF] = Data;
  } else if (n == 0xF00) {
    Memory[Address & 0xFF] = Data;
  }
}

/********************************************************************/
/* Keypad scanning                                                  */
/********************************************************************/

int ReadKeyboard(int a) {
  int data = 0xff;
  if ((Key == '0') && (a == 0))
    data = 0b01111111;
  else if ((Key == '1') && (a == 1))
    data = 0b01111111;
  else if ((Key == '2') && (a == 2))
    data = 0b01111111;
  else if ((Key == '3') && (a == 3))
    data = 0b01111111;
  else if ((Key == '4') && (a == 4))
    data = 0b01111111;
  else if ((Key == '5') && (a == 5))
    data = 0b01111111;
  else if ((Key == '6') && (a == 6))
    data = 0b01111111;
  else if ((Key == '7') && (a == 7))
    data = 0b01111111;
  else if ((Key == '8') && (a == 0))
    data = 0b10111111;
  else if ((Key == '9') && (a == 1))
    data = 0b10111111;
  else if ((Key == 'a') && (a == 0))
    data = 0b11101111;
  else if ((Key == 'b') && (a == 1))
    data = 0b11101111;
  else if ((Key == 'c') && (a == 2))
    data = 0b11101111;
  else if ((Key == 'd') && (a == 3))
    data = 0b11101111;
  else if ((Key == 'e') && (a == 6))
    data = 0b11101111;
  else if ((Key == 'f') && (a == 7))
    data = 0b11101111;
  else if ((Key == 'g') && (a == 2))
    data = 0b11011111;
  else if ((Key == 'm') && (a == 3))
    data = 0b11011111;
  else if ((Key == 'z') && (a == 4))
    data = 0b11011111;
  else if ((Key == 't') && (a == 7))
    data = 0b11011111;
  if (data != 0xff)
    Key = 0;
  return data;
}

/********************************************************************/
/* Load Intel Hex over serial                                       */
/********************************************************************/

void hexLoader(int b) {
  if ((b == '\n') || (b == '\r'))
    return;
  if ((b >= 97) && (b <= 122))
    b = b - 32;
  if ((b >= 65) && (b <= 70) || (b >= 48) && (b <= 57)) {
    int intVal = (b <= 57) ? b - 48 : b - 65 + 10;
    if (hexCounter % 2 == 0) {
      hexTmp = intVal;
    } else {
      int data = (hexTmp * 16) + intVal;
      int index = hexCounter / 2;
      if (index == 0) {
        hexSum = 0;
        hexBytes = data;
        Serial.print("Loading ");
        Serial.print(data);
      } else if (index == 1) {
        hexAddrHi = data;
      } else if (index == 2) {
        hexAddr = ((hexAddrHi * 16 * 16) + data);
        Serial.print(" bytes at address ");
        Serial.print(hexAddr, HEX);
        Serial.println(" ...");
      } else if (index == 3) {
        if (data == 1) {
          Serial.println("Done");
          hexLoaderState = 0;
        }
      } else if (index < hexBytes + 4) {
        WriteMemory(hexAddr++, data);
      } else if (index == hexBytes + 4) {
        int calculatedChecksum = (~hexSum & 0x0FF) + 1;
        if (calculatedChecksum != data) {
          Serial.println("Checksum mismatch");
        }
        hexLoaderState = 0;
      }
      hexSum += data;
    }
    hexCounter++;
  } else {
    Serial.println("Unexpected character ... aborting");
    hexLoaderState = 0;
  }
}

/********************************************************************/
/* Memory fetch helper                                              */
/********************************************************************/

void fetch(int *ptr) {
  *ptr = ReadMemory((++Ptr[0]) & 0xFFF);
}
