/* Opal BLAKE2b
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

const uint64_t constants[8] = {
 0x6A09E667F3BCC908, 0xBB67AE8584CAA73B,
 0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,
 0x510E527FADE682D1, 0x9B05688C2B3E6C1F,
 0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179
};

const int sigma[10][16] = {
 {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
 {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
 {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
 {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
 {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
 {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
 {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
 {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
 {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
 {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0}
};

uint64_t state[16];
uint64_t chain[8];
uint64_t data[16];
uint64_t count[2];


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
 char message[] = "Opal BLAKE2b\n\n"
 "options\n"
 "h: print help and exit\n"
 "l: list mode; read filenames from standard input and output hashes\n"
 "c: check mode; read hashes and filenames from standard input and check them\n"
 "s: hash length in bytes (default: 64)\n\n"
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

void g(int i, int r, uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d)
{
 *a += *b + data[sigma[r][i * 2]];
 *d = *d ^ *a;
 *d = (*d << 32) | (*d >> 32);
 *c += *d;
 *b = *b ^ *c;
 *b = (*b << 40) | (*b >> 24);
 *a += *b + data[sigma[r][(i * 2) + 1]];
 *d = *d ^ *a;
 *d = (*d << 48) | (*d >> 16);
 *c += *d;
 *b = *b ^ *c;
 *b = (*b << 1) | (*b >> 63);

 return;
}

void mix()
{
 int round, r = 0;

 for(round = 0; round < 12; round++)
 {
  g(0, r, state, state + 4, state + 8, state + 12);
  g(1, r, state + 1, state + 5, state + 9, state + 13);
  g(2, r, state + 2, state + 6, state + 10, state + 14);
  g(3, r, state + 3, state + 7, state + 11, state + 15);
  g(4, r, state, state + 5, state + 10, state + 15);
  g(5, r, state + 1, state + 6, state + 11, state + 12);
  g(6, r, state + 2, state + 7, state + 8, state + 13);
  g(7, r, state + 3, state + 4, state + 9, state + 14);
  if(++r == 10) r = 0;
 }

 return;
}

void hash_stream(FILE *fp, int len)
{
 int i, c;

 /* initialize chain */
 for(i = 0; i < 8; i++) chain[i] = constants[i];
 chain[0] ^= 0x01010000 | len;

 /* load first block */
 for(i = 0; i < 16; i++) data[i] = 0;
 for(i = 0; i < 128; i++)
 {
  if((c = getc(fp)) == EOF) break;
  data[i / 8] |= ((uint64_t)c) << ((i % 8) * 8);
 }
 if(c != EOF) c = getc(fp);
 count[0] = i; count[1] = 0;

 /* hash data */
 while(c != EOF)
 {
  for(i = 0; i < 8; i++) state[i] = chain[i];
  for(i = 0; i < 8; i++) state[i + 8] = constants[i];
  state[12] ^= count[0]; state[13] ^= count[1];
  mix();
  for(i = 0; i < 8; i++) chain[i] ^= state[i] ^ state[i + 8];

  /* load next block */
  for(i = 0; i < 16; i++) data[i] = 0;
  for(i = 0; i < 128; i++)
  {
   data[i / 8] |= ((uint64_t)c) << ((i % 8) * 8);
   if((c = getc(fp)) == EOF) {i++; break;}
  }

  /* increment counter */
  count[0] += i;
  if(count[0] < i) count[1]++;
 }

 /* hash last block */
 for(i = 0; i < 8; i++) state[i] = chain[i];
 for(i = 0; i < 8; i++) state[i + 8] = constants[i];
 state[12] ^= count[0]; state[13] ^= count[1];
 state[14] = ~state[14];
 mix();
 for(i = 0; i < 8; i++) chain[i] ^= state[i] ^ state[i + 8];

 return;
}

/* output hash value */
void write_hash(int len)
{
 int i, digits;

 digits = len * 2;

 for(i = 0; i < digits; i++)
  if(putchar(hex_digit((chain[i / 16] >> (((i % 16) ^ 1) * 4)))) == EOF)
   failed("write hash value");

 return;
}

/* compare hash value */
bool cmp_hash(char *old_hash, int len)
{
 int i, digits;

 digits = len * 2;

 for(i = 0; i < digits; i++)
  if(old_hash[i] != hex_digit((chain[i / 16] >> (((i % 16) ^ 1) * 4))))
   return false;

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
 if((*len < 1) || (*len > 64)) fail("\"s\" must be at least 1 and not greater than 64");

 free(line);

 return;
}

void hash_input(int len)
{
 hash_stream(stdin, len);
 write_hash(len);
 putchar('\n');

 return;
}

void hash_files(int len)
{
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
  hash_stream(fp, len);
  write_hash(len);
  printf(" %s\n", fn);
  fclose(fp);
 }

 free(fn);

 return;
}

void check_files(int len)
{
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
  hash_stream(fp, len);
  if(cmp_hash(old_hash, len)) printf("%s: OK\n", fn);
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
 len = 64;

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
 if((len < 1) || (len > 64)) fail("\"s\" must be at least 1 and not greater than 64");

 if(mode == 0) hash_input(len);
 else if(mode == 1) hash_files(len);
 else if(mode == 2) check_files(len);
 else return EXIT_FAILURE;

 return EXIT_SUCCESS;
}
