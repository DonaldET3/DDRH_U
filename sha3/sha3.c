/* Opal SHA-3
 * for Unix
 */


/* pieces section */

#include <errno.h>
/* errno
 */

#include <stdio.h>
/* FILE
 * NULL
 * EOF
 * stdin
 * stderr
 * getc()
 * putc()
 * putchar()
 * fputs()
 * getline()
 * printf()
 * fprintf()
 * scanf()
 * sscanf()
 * fopen()
 * fclose()
 * perror()
 */

#include <stdlib.h>
/* size_t
 * NULL
 * EXIT_SUCCESS
 * EXIT_FAILURE
 * free()
 * exit()
 */

#include <string.h>
/* strstr()
 * strtok()
 * strerror_l()
 */

#include <stdint.h>
/* uint64_t
 * uintmax_t
 */

#include <stdbool.h>
/* bool
 * true
 * false
 */

#include <locale.h>
/* uselocale()
 */

#include <unistd.h>
/* getopt()
 */


/* definitions section */

/* round constants */
const uint64_t rc[24] = {
 0x0000000000000001, 0x0000000000008082,
 0x800000000000808A, 0x8000000080008000,
 0x000000000000808B, 0x0000000080000001,
 0x8000000080008081, 0x8000000000008009,
 0x000000000000008A, 0x0000000000000088,
 0x0000000080008009, 0x000000008000000A,
 0x000000008000808B, 0x800000000000008B,
 0x8000000000008089, 0x8000000000008003,
 0x8000000000008002, 0x8000000000000080,
 0x000000000000800A, 0x800000008000000A,
 0x8000000080008081, 0x8000000000008080,
 0x0000000080000001, 0x8000000080008008
};

/* rotation distances */
const int rd[5][5] = {
 {0, 36, 3, 41, 18},
 {1, 44, 10, 45, 2},
 {62, 6, 43, 15, 61},
 {28, 55, 25, 21, 56},
 {27, 20, 39, 8, 14}
};

/* pi coordinates */
const int pc[5][5] = {
 {0, 15, 5, 20, 10},
 {10, 0, 15, 5, 20},
 {20, 10, 0, 15, 5},
 {5, 20, 10, 0, 15},
 {15, 5, 20, 10, 0}
};


/* functions section */

/* print error message and quit */
void fail(char *message)
{
 /* print error message */
 fputs(message, stderr);
 /* elaborate on the error if possible */
 if(errno) fprintf(stderr, ": %s", strerror_l(errno, uselocale(0)));
 putc('\n', stderr);
 exit(EXIT_FAILURE);
}

/* "failed to" <error message> and quit */
void failed(char *message)
{
 /* prepend "failed to" to the error message */
 fputs("failed to ", stderr);
 fail(message);
}

/* help message */
void help()
{
 char message[] = "Opal SHA-3\n\n"
 "options\n"
 "h: print help and exit\n"
 "l: list mode; read filenames from standard input and output hashes\n"
 "c: check mode; read hashes and filenames from standard input and check them\n"
 "s: hash length in bits\n"
 "   (multiple of 8, at least 8, not more than 792, default: 512)\n\n"
 "By default, the program simply hashes standard input.\n";
 fputs(message, stderr);
}

/* invalid command line argument */
void invalid(char c)
{
 fprintf(stderr, "argument supplied to -%c is invalid\n", c);
 exit(EXIT_FAILURE);
}

/* binary to hexadecimal */
int hex_digit(int b)
{
 switch(b & 0xF)
 {
  case 0x0: return '0';
  case 0x1: return '1';
  case 0x2: return '2';
  case 0x3: return '3';
  case 0x4: return '4';
  case 0x5: return '5';
  case 0x6: return '6';
  case 0x7: return '7';
  case 0x8: return '8';
  case 0x9: return '9';
  case 0xA: return 'A';
  case 0xB: return 'B';
  case 0xC: return 'C';
  case 0xD: return 'D';
  case 0xE: return 'E';
  case 0xF: return 'F';
 }
}

/* rotate left */
uint64_t rot_l(uint64_t x, uint64_t n)
{
 n &= 0x3F;
 return (x << n) | (x >> (64 - n));
}

void mix(uint64_t *state)
{
 int round, x, y, w;
 uint64_t b[25], *c, *d;

 c = b; d = b + 5;

 for(round = 0; round < 24; round++)
 {
  /* theta step */
  for(x = 0; x < 5; x++)
   c[x] = state[x] ^ state[x + 5] ^ state[x + 10] ^ state[x + 15] ^ state[x + 20];

  d[0] = c[4] ^ rot_l(c[1], 1);
  d[1] = c[0] ^ rot_l(c[2], 1);
  d[2] = c[1] ^ rot_l(c[3], 1);
  d[3] = c[2] ^ rot_l(c[4], 1);
  d[4] = c[3] ^ rot_l(c[0], 1);

  for(x = 0; x < 5; x++)
   for(y = 0; y < 25; y += 5)
    state[x + y] ^= d[x];

  /* rho and pi steps */
  for(x = 0; x < 5; x++)
   for(y = 0; y < 5; y++)
    b[y + pc[x][y]] = rot_l(state[x + (y * 5)], rd[x][y]);

  /* chi step */
  for(y = 0; y < 25; y += 5)
  {
   state[y] = b[y] ^ ((~b[1 + y]) & b[2 + y]);
   state[1 + y] = b[1 + y] ^ ((~b[2 + y]) & b[3 + y]);
   state[2 + y] = b[2 + y] ^ ((~b[3 + y]) & b[4 + y]);
   state[3 + y] = b[3 + y] ^ ((~b[4 + y]) & b[y]);
   state[4 + y] = b[4 + y] ^ ((~b[y]) & b[1 + y]);
  }

  /* iota step */
  *state ^= rc[round];
 }

 return;
}

void hash_stream(uint64_t *state, int rate, FILE *fp)
{
 int i, c;

 /* initialize state */
 for(i = 0; i < 25; i++) state[i] = 0;

 /* process data */
 while(true)
 {
  /* load block */
  for(i = 0; i < rate; i++)
  {
   if((c = getc(fp)) == EOF) goto fin;
   state[i / 8] ^= ((uint64_t)c) << ((i % 8) * 8);
  }

  /* assimilate changes */
  mix(state);
 }

 /* append padding */
 fin: state[i / 8] ^= ((uint64_t)0x06) << ((i % 8) * 8);
 state[(rate - 1) / 8] ^= ((uint64_t)0x80) << (((rate - 1) % 8) * 8);

 /* final mixing */
 mix(state);

 return;
}

/* output hash value */
void write_hash(uint64_t *state, int len)
{
 int i, j, digits, rate;

 digits = len / 4;
 rate = 400 - (len / 2);

 j = 0;
 for(i = 0; i < digits; i++)
 {
  if(j == rate){mix(state); j = 0;}
  if(putchar(hex_digit((state[j / 16] >> (((j % 16) ^ 1) * 4)))) == EOF)
   failed("write hash value");
  j++;
 }

 return;
}

/* compare hash value */
bool cmp_hash(uint64_t *state, int len, char *old_hash)
{
 int i, j, digits, rate;

 digits = len / 4;
 rate = 400 - (len / 2);

 j = 0;
 for(i = 0; i < digits; i++)
 {
  if(j == rate){mix(state); j = 0;}
  if(old_hash[i] != hex_digit((state[j / 16] >> (((j % 16) ^ 1) * 4))))
   return false;
  j++;
 }

 if(old_hash[digits] != '\0') return false;

 return true;
}

/* read parameters from header */
void read_header(int *len)
{
 char n, *line = NULL, *setting;
 int v;
 size_t size = 0;

 if(getline(&line, &size, stdin) == -1) fail("empty file");

 if((setting = strtok(line, " \n")) != NULL)
 {
  do
  {
   if(sscanf(setting, "%c=%d", &n, &v) != 2) fail("header improperly formated");

   switch(n)
   {
    case 's': *len = v; break;
    default: fail("incompatible header");
   }
  } while((setting = strtok(NULL, " \n")) != NULL);
 }

 /* check values */
 if(*len % 8) fail("\"s\" must be a multiple of 8");
 if((*len < 8) || (*len > 792)) fail("\"s\" must be at least 8 and not greater than 792");

 free(line);

 return;
}

void hash_input(int len)
{
 uint64_t state[25];

 hash_stream(state, 200 - (len / 4), stdin);
 write_hash(state, len);
 putchar('\n');

 return;
}

void hash_files(int len)
{
 uint64_t state[25];
 char *fn = NULL;
 size_t size = 0;
 FILE *fp;

 /* write parameters */
 if(printf("s=%d\n", len) < 0) failed("write header");

 while(getline(&fn, &size, stdin) != -1)
 {
  if((fp = fopen(strtok(fn, "\n"), "rb")) == NULL)
  {
   perror(fn);
   continue;
  }
  hash_stream(state, 200 - (len / 4), fp);
  write_hash(state, len);
  printf(" %s\n", fn);
  fclose(fp);
 }

 free(fn);

 return;
}

void check_files(int len)
{
 uint64_t state[25];
 uintmax_t fails = 0;
 char *line = NULL, *old_hash, *fn;
 size_t size = 0;
 FILE *fp;

 read_header(&len);

 while(getline(&line, &size, stdin) != -1)
 {
  old_hash = strtok(line, " ");
  if((fp = fopen(fn = strtok(NULL, "\n"), "rb")) == NULL)
  {
   fails++;
   perror(fn);
   continue;
  }
  hash_stream(state, 200 - (len / 4), fp);
  if(cmp_hash(state, len, old_hash)) printf("%s: OK\n", fn);
  else
  {
   printf("%s: FAILED\n", fn);
   fails++;
  }
  fclose(fp);
 }

 if(fails) fprintf(stderr, "%ju check(s) failed\n", fails);

 free(line);

 return;
}

int main(int argc, char **argv)
{
 int c, mode = 0, len;
 extern char *optarg;
 extern int opterr, optind, optopt;

 /* the errno symbol is defined in errno.h */
 errno = 0;

 /* default options */
 len = 512;

 /* parse the command line */
 while((c = getopt(argc, argv, "hlcs:")) != -1)
  switch(c)
  {
   case 'h': help(); exit(EXIT_SUCCESS);
   case 'l': mode = 1; break;
   case 'c': mode = 2; break;
   case 's': if(sscanf(optarg, "%d", &len) != 1) invalid(c); break;
   case '?': exit(EXIT_FAILURE);
  }

 /* check values */
 if(len % 8) fail("\"s\" must be a multiple of 8");
 if((len < 8) || (len > 792)) fail("\"s\" must be at least 8 and not greater than 792");

 if(mode == 0) hash_input(len);
 else if(mode == 1) hash_files(len);
 else if(mode == 2) check_files(len);
 else return EXIT_FAILURE;

 return EXIT_SUCCESS;
}
