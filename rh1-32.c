/* Data Dependent Rotation Hash 1
 * 32-bit
 * for Unix
 * 
 * based on concepts from CubeHash and RC5
 */


/* pieces section */

#include <stdio.h>
/* getc()
 * ungetc()
 * putc()
 * getchar()
 * putchar()
 * fputs()
 * printf()
 * scanf()
 * fopen()
 * fclose()
 * perror()
 * FILE
 * NULL
 * EOF
 */

#include <stdlib.h>
/* malloc()
 * calloc()
 * realloc()
 * free()
 * NULL
 * EXIT_SUCCESS
 * EXIT_FAILURE
 */

#include <string.h>
/* strstr()
 */

#include <stdint.h>
/* uint8_t
 * uint32_t
 */

#include <stdbool.h>
/* bool
 * true
 * false
 */

#include <unistd.h>
/* getopt()
 */


/* definitions section */

/* program options */
struct options {
 /* number of words in the state */
 unsigned swn;
 /* number of initialization rounds */
 unsigned i;
 /* number of rounds per message block */
 unsigned r;
 /* number of bytes per message block */
 unsigned b;
 /* number of finalization rounds */
 unsigned f;
 /* hash length in bits */
 unsigned l;
 /* filenames */
 char **fnames;
 /* filename starting index */
 int ind;
};

struct hash_state {
 /* reorder array */
 unsigned *reo;
 /* state array */
 uint32_t *s;
 /* hash array */
 uint8_t *h;
};


/* funtions section */

/* internal failure */
void fail(char *message)
{
 fprintf(stderr, "%s\n", message);
 exit(EXIT_FAILURE);
}

/* library error */
void error(char *message)
{
 perror(message);
 exit(EXIT_FAILURE);
}

/* help message */
void help()
{
 char *message = "Data Dependent Rotation Hash, 32-bit\n\n"
 "options\n"
 "h: print help and exit\n"
 "c: check mode; read filenames and hashes from the input files and check them\n"
 "w: number of words in the state\n"
 "i: number of initialization rounds\n"
 "r: number of rounds per message block\n"
 "b: number of bytes per message block\n"
 "f: number of finalization rounds\n"
 "l: hash length in bits\n";
 fputs(message, stderr);
}

/* invalid command line argument */
void invalid(char c)
{
 fprintf(stderr, "argument supplied to -%c is invalid\n", c);
 exit(EXIT_FAILURE);
}

/* rotate left */
uint32_t rot_l(uint32_t x, uint32_t n)
{
 n &= 0x1F;
 return (x << n) | (x >> (32 - n));
}

/* rotate right */
uint32_t rot_r(uint32_t x, uint32_t n)
{
 n &= 0x1F;
 return (x >> n) | (x << (32 - n));
}

/* binary logarithm floor for unsigned integers */
unsigned log2_floor(unsigned x)
{
 unsigned y = 0;

 while(x >>= 1) y++;

 return y;
}

/* generate the reorder array */
unsigned *reorder(unsigned *reo, unsigned sco)
{
 unsigned i, tmp;

 if((reo = realloc(reo, sco * sizeof(unsigned))) == NULL) error("allocate reo");

 /* generate arithmetic progression */
 for(i = 0; i < sco; i++)
  reo[i] = i;

 while(sco > 1)
 {
  sco /= 2;
  /* swap halves */
  for(i = 0; i < sco; i++)
  {
   tmp = reo[i];
   reo[i] = reo[i + sco];
   reo[i + sco] = tmp;
  }
 }

 return reo;
}

/* transform state words */
void transform(uint32_t *s, unsigned *reo, unsigned swn, unsigned r)
{
 unsigned i, j, hswn, end;
 uint32_t a;

 hswn = swn / 2;
 a = 0xAAAAAAAA;
 end = hswn * r;

 for(i = 0; i < end; i++)
 {
  /* use the bottom half of the state to transform the top half */
  for(j = 0; j < hswn; j++)
  {
   s[j + hswn] = rot_l(s[j + hswn] ^ s[j], s[j]) + a;
   a += 0x89ABCDEF;
  }
  /* reorder the top half of the state and use it to transform the bottom half */
  for(j = 0; j < hswn; j++)
  {
   s[j] = rot_l(s[j] ^ s[reo[j]], s[reo[j]]) + a;
   a += 0x89ABCDEF;
  }
 }

 return;
}

/* check options */
int check_o(struct options *opts)
{
 int e = 0;

 if(opts->swn < 2)
 {
  fputs("\"w\" must be at least 2\n", stderr);
  e++;
 }

 if((1 << log2_floor(opts->swn)) != opts->swn)
 {
  fputs("\"w\" must be a power of two\n", stderr);
  e++;
 }

 if(opts->i < 1)
 {
  fputs("\"i\" must be at least 1\n", stderr);
  e++;
 }

 if(opts->r < 1)
 {
  fputs("\"r\" must be at least 1\n", stderr);
  e++;
 }

 if((!opts->b) || (opts->b > (opts->swn * 4)))
 {
  fputs("\"b\" cannot be less than 1 or longer than the state\n", stderr);
  e++;
 }

 if(opts->f < 1)
 {
  fputs("\"f\" must be at least 1\n", stderr);
  e++;
 }

 if((opts->l % 8) || (opts->l > (opts->swn * 16)))
 {
  fputs("\"l\" must be a multiple of 8 not greater than half of the state bit length\n", stderr);
  e++;
 }

 return e;
}

/* load data block */
bool load_block(FILE *fp, uint32_t *s, unsigned b)
{
 int i, c;

 for (i = 0; i < b; i++)
 {
  if((c = getc(fp)) == EOF)
  {
   s[i / 4] ^= ((uint32_t)0x80) << ((3 - (i & 3)) * 8);
   return false;
  }

  s[i / 4] ^= ((uint32_t)c) << ((3 - (i & 3)) * 8);
 }

 return true;
}

/* extract hash from state */
void extract_h(uint32_t *s, unsigned l, uint8_t *h)
{
 int i, end;

 end = l / 8;

 for(i = 0; i < end; i++)
  h[i] = s[i / 4] >> ((3 - (i & 3)) * 8);

 return;
}

/* hash a file */
void hash_file(FILE *fp, struct hash_state *hs, struct options *opts)
{
 int i;
 bool more = true;
 uint32_t *s;

 s = hs->s;

 for(i = 0; i < opts->swn; i++)
  s[i] = 0;

 /* initialize words */
 s[0] = opts->l / 8;
 s[1] = opts->b;
 s[2 & (opts->swn - 1)] += opts->r;

 /* initialization rounds */
 transform(s, hs->reo, opts->swn, opts->i);

 /* process data */
 while(more)
 {
  more = load_block(fp, s, opts->b);
  transform(s, hs->reo, opts->swn, opts->r);
 }

 s[opts->swn - 1] ^= 1;
 /* finalization rounds */
 transform(s, hs->reo, opts->swn, opts->f);
 extract_h(s, opts->l, hs->h);
 return;
}

/* binary to hexadecimal */
int b2h(uint8_t b)
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

/* hexadecimal to binary */
int h2b(int c)
{
 switch(c)
 {
  case '0': return 0x0;
  case '1': return 0x1;
  case '2': return 0x2;
  case '3': return 0x3;
  case '4': return 0x4;
  case '5': return 0x5;
  case '6': return 0x6;
  case '7': return 0x7;
  case '8': return 0x8;
  case '9': return 0x9;
  case 'A': return 0xA;
  case 'B': return 0xB;
  case 'C': return 0xC;
  case 'D': return 0xD;
  case 'E': return 0xE;
  case 'F': return 0xF;
  case 'a': return 0xA;
  case 'b': return 0xB;
  case 'c': return 0xC;
  case 'd': return 0xD;
  case 'e': return 0xE;
  case 'f': return 0xF;
  default: return 0x0;
 }
}

/* write hash */
void write_hash(uint8_t *h, unsigned l)
{
 int i, end;

 end = l / 8;

 for(i = 0; i < end; i++)
 {
  if(putchar(b2h(h[i] >> 4)) == EOF) error("write hash");
  if(putchar(b2h(h[i])) == EOF) error("write hash");
 }

 return;
}

/* hash input files */
void hash_files(struct options *opts)
{
 int i;
 bool lfw = false;
 FILE *fp;
 struct hash_state hs;

 if((hs.h = malloc(opts->l / 8)) == NULL) error("allocate hash");
 if((hs.s = malloc(opts->swn * 4)) == NULL) error("allocate state");
 hs.reo = reorder(NULL, opts->swn);

 if(opts->fnames[opts->ind] == NULL)
 {
  /* hash standard input */
  hash_file(stdin, &hs, opts);
  write_hash(hs.h, opts->l);
 }
 else
  /* write parameters */
  if(printf("w=%u i=%u r=%u b=%u f=%u l=%u\n", opts->swn, opts->i, opts->r, opts->b, opts->f, opts->l) < 0) error("write header");

 /* hash file list */
 for(i = opts->ind; opts->fnames[i] != NULL; i++)
 {
  if((fp = fopen(opts->fnames[i], "rb")) == NULL)
  {
   perror("open file");
   continue;
  }
  hash_file(fp, &hs, opts);
  fclose(fp);
  write_hash(hs.h, opts->l);
  if(strstr(opts->fnames[i], "\n") != NULL) lfw = true;
  printf(" %s\n", opts->fnames[i]);
 }

 free(hs.reo);
 free(hs.s);
 free(hs.h);
 putchar('\n');

 if(lfw) fputs("One or more of the filenames contain line feed characters.\nThe output cannot be used in check mode.\n", stderr);

 return;
}

/* read parameters from header */
int read_header(FILE *cf, struct options *opts)
{
 int c;
 char n;
 unsigned v;

 c = getc(cf);
 if(c == EOF)
  return -1;
 else
  if(ungetc(c, cf) == EOF)
   fail("ungetc fail");

 while(true)
 {
  if(fscanf(cf, "%c=%u", &n, &v) != 2)
  {
   fputs("file improperly formated\n", stderr);
   return 1;
  }

  switch(n)
  {
   case 'w': opts->swn = v; break;
   case 'i': opts->i = v; break;
   case 'r': opts->r = v; break;
   case 'b': opts->b = v; break;
   case 'f': opts->f = v; break;
   case 'l': opts->l = v; break;
  }

  c = getc(cf);
  if(c == '\n')
   break;
  if(c == EOF)
  {
   fputs("no hashes in file\n", stderr);
   return -1;
  }
 }

 if(check_o(opts))
 {
  fputs("check file corrupt\n", stderr);
  return 1;
 }

 return 0;
}

/* read a file entry from a check file */
char *get_entry(FILE *cf, uint8_t *h, unsigned l, char *fn)
{
 int i, hbl, c, pos = 0, space = 0;

 hbl = l / 8;

 h[hbl] = 0;

 /* read hash */
 for(i = 0; i < hbl; i++)
 {
  c = getc(cf);
  if(c == ' ')
  {
   h[hbl] = 1;
   break;
  }
  if((c == '\n') || (c == EOF))
  {
   if(fn != NULL) free(fn);
   return NULL;
  }
  h[i] = h2b(c) << 4;

  c = getc(cf);
  if(c == ' ')
  {
   h[hbl] = 1;
   break;
  }
  if((c == '\n') || (c == EOF))
  {
   if(fn != NULL) free(fn);
   return NULL;
  }
  h[i] |= h2b(c);
 }

 /* hash overflow */
 if(h[hbl] == 0)
  while(true)
  {
   c = getc(cf);
   if(c == ' ') break;
   h[hbl] = 1;
   if((c == '\n') || (c == EOF))
   {
    if(fn != NULL) free(fn);
    return NULL;
   }
  }

 if(fn != NULL)
  if(strlen(fn) >= 255)
   if((fn = realloc(fn, space = 256)) == NULL)
    error("allocate file name");

 /* read filename */
 while(true)
 {
  c = getc(cf);

  if((c == '\n') || (c == EOF))
  {
   if(pos == 0)
   {
    if(fn != NULL) free(fn);
    return NULL;
   }
   fn[pos] = '\0';
   return fn;
  }

  if(pos >= (space - 1))
   if((fn = realloc(fn, space += 256)) == NULL)
    error("allocate file name");
  fn[pos++] = c;
 }
}

/* process a check file */
unsigned check_files(FILE *cf, struct options *opts)
{
 int i, hbl, e;
 unsigned fails = 0;
 char *fn;
 bool match;
 uint8_t *h;
 FILE *fp;
 struct hash_state hs;

 hs.reo = NULL;
 hs.s = NULL;
 hs.h = NULL;
 h = NULL;
 fn = NULL;

 while(true)
 {
  /* get parameters */
  if(e = read_header(cf, opts))
  {
   if(e > 0) fails += e;
   if(hs.reo != NULL) free(hs.reo);
   if(hs.s != NULL) free(hs.s);
   if(hs.h != NULL) free(hs.h);
   if(h != NULL) free(h);
   if(fn != NULL) free(fn);
   return fails;
  }

  hbl = opts->l / 8;
  if((h = realloc(h, hbl + 1)) == NULL) error("allocate hash");
  if((hs.h = realloc(hs.h, hbl + 1)) == NULL) error("allocate hash");
  if((hs.s = realloc(hs.s, opts->swn * 4)) == NULL) error("allocate state");
  hs.reo = reorder(hs.reo, opts->swn);

  hs.h[hbl] = 0;

  /* process entries */
  while(true)
  {
   if((fn = get_entry(cf, h, opts->l, fn)) == NULL) break;
   if((fp = fopen(fn, "rb")) == NULL)
   {
    fails++;
    perror(fn);
    continue;
   }

   hash_file(fp, &hs, opts);

   fclose(fp);

   match = true;
   for(i = 0; i <= hbl; i++)
    if(hs.h[i] != h[i])
     match = false;

   if(match)
    printf("%s: OK\n", fn);
   else
   {
    fails++;
    printf("%s: FAILED\n", fn);
   }
  }
 }
}

/* process check file list */
void check_file_l(struct options *opts)
{
 int i;
 unsigned fails = 0;
 FILE *fp;

 /* read check file from standard input */
 if(opts->fnames[opts->ind] == NULL)
  fails = check_files(stdin, opts);

 /* process list of check files from the command line */
 for(i = opts->ind; opts->fnames[i] != NULL; i++)
 {
  if((fp = fopen(opts->fnames[i], "rb")) == NULL)
  {
   perror("open check file");
   continue;
  }
  fails += check_files(fp, opts);
  fclose(fp);
 }

 if(fails) fprintf(stderr, "%u check(s) failed\n", fails);

 return;
}

int main(int argc, char **argv)
{
 int c;
 bool check_m = false;
 struct options opts;
 extern char *optarg;
 extern int opterr, optind, optopt;

 opts.swn = 32;
 opts.i = 4;
 opts.r = 4;
 opts.b = 32;
 opts.f = 8;
 opts.l = 512;

 /* parse the command line */
 while((c = getopt(argc, argv, "hcw:i:r:b:f:l:")) != -1)
  switch(c)
  {
   case 'h': help(); exit(EXIT_SUCCESS);
   case 'c': check_m = true; break;
   case 'w': if(sscanf(optarg, "%u", &opts.swn) != 1) invalid(c); break;
   case 'i': if(sscanf(optarg, "%u", &opts.i) != 1) invalid(c); break;
   case 'r': if(sscanf(optarg, "%u", &opts.r) != 1) invalid(c); break;
   case 'b': if(sscanf(optarg, "%u", &opts.b) != 1) invalid(c); break;
   case 'f': if(sscanf(optarg, "%u", &opts.f) != 1) invalid(c); break;
   case 'l': if(sscanf(optarg, "%u", &opts.l) != 1) invalid(c); break;
   case '?': exit(EXIT_FAILURE);
  }

 if(check_o(&opts)) exit(EXIT_FAILURE);

 opts.fnames = argv;
 opts.ind = optind;

 if(check_m) check_file_l(&opts);
 else hash_files(&opts);

 return EXIT_SUCCESS;
}
