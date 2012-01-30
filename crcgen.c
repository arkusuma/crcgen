/*
 *  Universal CRC source generator
 *  Version 0.1
 *  Author: Anugrah Redja Kusuma
 * 
 *  Started:      1 November 2001
 *  Last update: 10 February 2002
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FALSE 0
#define TRUE  1

// Constant used by genspec()
#define CRC16  1
#define XMODEM 2
#define CCIT   3
#define CRC32  4

// Macro for writing to file (since it would be a looong line)
#define WR(x) fprintf(fd, x);

struct crcspec
{
  int width;             // How many bit used ? [8, 16, 32]
  unsigned long poly;    // CRC polynomial
  unsigned long init;    // Initial value of register
  int reflect;           // Should read input from MSB-LSB or LSB-MSB ?
  unsigned long xorout;  // Register will be xored with this value at the end
};
struct crcspec spec;

int crc_id = CRC32;
int SizeForSpeed=1;
char src_name[256] = "crc";

unsigned long crctable[256];

/*
 *  Reflect bits order from n LSBs
 */
unsigned long reflect(unsigned long val, int n)
{
  int i;
  unsigned long result;

  result = 0;
  for (i=0; i<n; i++)
    {
      result <<= 1;
      if ((val & 1) != 0)
	result |= 1;
      val >>= 1;
    }
  return result;
}

/*
 *  Generate spec for an already known CRC method
 */
void genspec(int method)
{
  switch (method)
    {
    case CCIT:
      spec.width = 16;
      spec.poly = 0x1021;
      spec.init = 0xffff;
      spec.reflect = FALSE;
      spec.xorout = 0;
      break;
    case XMODEM:
      spec.width = 16;
      spec.poly = 0x8408;
      spec.init = 0;
      spec.reflect = TRUE;
      spec.xorout = 0;
      break;
    case CRC16:
      spec.width = 16;
      spec.poly = 0x8005;
      spec.init = 0;
      spec.reflect = TRUE;
      spec.xorout = 0;
      break;
    case CRC32:
      spec.width = 32;
      spec.poly = 0x04c11db7;
      spec.init = 0xffffffff;
      spec.reflect = TRUE;
      spec.xorout = 0xffffffff;
    }
}

/*
 *  Generate CRC table
 */
void gentable(void)
{
  int i, j;
  unsigned long poly, mask;

  // If input bits read from LSB to MSB simply reflect the poly
  // rather than reflecting all bytes
  poly = spec.poly;
  if (spec.reflect)
    poly = reflect(poly, spec.width);

  mask = 0;
  for (i=0 ; i<spec.width; i++)
    mask = (mask << 1) | 1;

  for (i=0; i<256; i++)
    {
      crctable[i] = i;
      for (j=0; j<8; j++)
	{
	  if ((crctable[i] & 1) != 0)
	    crctable[i] = (crctable[i] >> 1)^poly;
	  else
	    crctable[i] >>= 1;
	}
      crctable[i] &= mask;
    }
}

void strupper(char *src)
{
  int i;
  
  for (i=0; src[i]; i++)
    src[i] = toupper(src[i]);
}

void writehex(FILE *fd, unsigned long num)
{
  char fmt[]="0x%00lx";

  fmt[4] += (spec.width+3)/4;
  fprintf(fd, fmt, num);
}

void writetype(FILE *fd)
{
  if (spec.width <= 8)
    fprintf(fd, "unsigned char");
  else if (spec.width <= 16)
    fprintf(fd, "unsigned short");
  else if (spec.width <= 32)
    fprintf(fd, "unsigned long");
}

void gensource(void)
{
  FILE *fd;
  int i;
  int n; // number of items per line (in crc_table)
  char fn[256];

  /*
   *  Generate header file
   */
  sprintf(fn, "%s.h", src_name);
  fd = fopen(fn, "w");
  if (fd == 0)
    {
      fprintf(stderr, "crcgen: error writing to file %s\n", fn);
      exit(1);
    }
  strcpy(fn, src_name);
  strupper(fn);
  fprintf(fd, "#ifndef _%s_H_\n", fn);
  fprintf(fd, "#define _%s_H_\n\n", fn);
  WR("#ifdef __cplusplus\n");
  WR("extern \"C\" {\n");
  WR("#endif\n\n");
  if (!SizeForSpeed)
    WR("void crcCreateTable(void);\n");
  WR("void crcBegin(void);\n");
  writetype(fd);
  WR(" crcGetValue(void);\n");
  WR("void crcByte(char code);\n");
  WR("void crcBlock(const char *codes, int count);\n\n");
  WR("#ifdef __cplusplus\n");
  WR("}\n");
  WR("#endif\n\n");
  WR("#endif\n");
  fclose(fd);

  /*
   *  Generate c file
   */
  sprintf(fn, "%s.c", src_name);
  fd = fopen(fn, "w");
  if (fd == 0)
    {
      fprintf(stderr, "crcgen: error writing to file %s\n", fn);
      exit(1);
    }
  sprintf(fn, "%s.h", src_name);
  fprintf(fd, "#include \"%s\"\n\n", fn);
  if (!SizeForSpeed)
  {
    fprintf(fd, "#define POLY ");
    if (spec.reflect)
      writehex(fd, reflect(spec.poly, spec.width));
    else
      writehex(fd, spec.poly);
    WR("\n\n");
  }
  writetype(fd); 
  WR(" crc_reg;\n");
  writetype(fd);
  if (SizeForSpeed)
  {
    gentable();
    n = 76/(((spec.width+3)/4)+4);
    WR(" crc_table[] =\n{\n");
    for (i=0; i<255; i++) 
      {
        if (i%n == 0)
    	  WR(" ");
        WR(" ");
        writehex(fd, crctable[i]);
        WR(",");
        if ((i+1)%n == 0)
 	WR("\n");
      }
    WR(" ");
    writehex(fd, crctable[255]);
    WR("\n};\n\n");
  }
  else
  {
    WR(" crc_table[256];\n\n");
    WR("void crcCreateTable(void)\n");
    WR("{\n");
    WR("  int i, j;\n\n");
    WR("  for (i=0; i<256; i++)\n");
    WR("  {\n");
    WR("    crc_table[i] = i;\n");
    WR("    for (j=0; j<8; j++)\n");
    WR("      if ((crc_table[i] & 1) != 0)\n");
    WR("        crc_table[i] = (crc_table[i] >> 1)^POLY;\n");
    WR("      else\n");
    WR("        crc_table[i] >>= 1;\n");
    WR("  }\n");
    WR("}\n\n");
  }

  WR("void crcBegin(void)\n");
  WR("{\n");
  WR("  crc_reg = ");
  writehex(fd, spec.init);
  WR(";\n}\n\n");

  writetype(fd);
  WR(" crcGetValue(void)\n");
  WR("{\n");
  if (spec.xorout != 0)
    {
      WR("  crc_reg ^= ");
      writehex(fd, spec.xorout);
      WR(";\n");
    }
  WR("  return crc_reg;\n");
  WR("}\n\n");

  WR("void crcByte(char code)\n");
  WR("{\n");
  WR("  crc_reg = crc_table[(code^crc_reg)&0xff]^(crc_reg>>8);\n");
  WR("}\n\n");

  WR("void crcBlock(const char *code, int count)\n");
  WR("{\n");
  WR("  int i;\n\n");
  WR("  for (i=0; i<count; i++)\n");
  WR("    crc_reg = crc_table[(code[i]^crc_reg)&0xff]^(crc_reg>>8);\n");
  WR("}\n");
}

void show_usage(void)
{
  printf("crcgen 0.1\n");
  printf("Create a C source for finding CRC\n\n");

  printf("Usage: crcgen [options] CRCNAME\n");
  printf("  -o filename  Write to filename.c and filename.h\n");
  printf("  -s           Minimize size\n");
  printf("  --help       Show help page (this one)\n\n");

  printf("CRCNAME: CRC32, CRC16, CCIT, XMODEM\n");
  exit(0);
}

void unknown_param(char *param)
{
  fprintf(stderr, "crcgen - unknown parameter %s\n", param);
  fprintf(stderr, "Try 'crcgen --help' for more information\n");
  exit(1);
}

void check_param(int argc, char *argv[])
{
  int i;

  for (i=1; i<argc; i++)
    {
      if (argv[i][0] == '-')
	{
       	  if (argv[i][1] == '-')
	    {
	      if (strcmp(&argv[i][2], "help") == 0)
		show_usage();
	      else
		unknown_param(argv[i]);
	    }
	  else if (argv[i][1] == 'o')
	    {
	      strcpy(src_name, argv[i+1]);
	      i++;
	    }
	  else if (argv[i][1] == 's')
	    SizeForSpeed = 0;
	  else
	    unknown_param(argv[i]);
	}
      else
	{
	  if (strcmpi(argv[i], "XMODEM") == 0)
	    crc_id = XMODEM;
	  else if (strcmpi(argv[i], "CRC16") == 0)
	    crc_id = CRC16;
	  else if (strcmpi(argv[i], "CCIT") == 0)
	    crc_id = CCIT;
	  else if (strcmpi(argv[i], "CRC32") == 0)
	    crc_id = CRC32;
	  else
	    {
	      fprintf(stderr, "crcgen - unknown CRC %s\n", argv[i]);
	      exit(1);
	    }
	}
    }
  if (crc_id == -1)
    {
      fprintf(stderr, "crcgen - CRC name not specified\n");
      fprintf(stderr, "Try 'crcgen --help' for more information\n");
      exit(1);
    }
}

int main(int argc, char *argv[])
{
  check_param(argc, argv);
  genspec(crc_id);
  gensource();
  return 0;
}
