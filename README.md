# DDRH_U
Data Dependent Rotation Hash for Unix

You should review the source code of this program before using it with important files or a system that contains important files. You take all responsibility for what happens when you use this program. If you do decide to use this program, save a copy of the source code; the code in this repository may be replaced by an entirely incompatible program at any time.

This program computes hashes of files to verify data integrity.

Files may be specified on the command line or a file may be input through standard input. If no filename is provided, the program simply outputs a hash after the file ends. If files are named, the program outputs a line for each file to standard output containing the hash, a single space, and the filename.

If the program is in check mode, the program reads a file of the same format that is output when files are specified on the command line. A message is written to standard error if a file is missing. If the file checks out, the filename and ": OK" is written to standard output. If the file fails the check, the filename and ": FAILED" is written to standard output and a message is written to standard error with the number of failed checks and " check(s) failed".

### options
h: print help and exit  
c: check mode, the input file(s) contain filenames and hashes to check against  
w: number of words in the state  
i: number of initialization rounds  
r: number of rounds per message block  
b: number of bytes per message block  
f: number of finalization rounds  
l: hash length in bits  

## algorithm

I = number of initialization rounds  
R = number of rounds per message block  
B = number of bytes per message block  
F = number of finalization rounds  
L = number of output bits, must be a multiple of the byte length, at least one byte, cannot be more than half of the state  
W = words in the state  
S = state array  
byte_length = number of bits in a byte

The algorithm uses a state array containing a power of two number of words, at least two words.  
S is initialized by setting the first three words to L/byte_length, B, and R respectively. If there are only two words, then add R to the first word instead. Then S is transformed through I rounds.  
XOR the first message block into the first B bytes of S (big-endian style), transform S through R rounds, XOR the next message block into S, transform S through R rounds, and so on. Append a 1 bit to the end of the message, then append the minimum number of 0 bits needed to reach a multiple of B bytes.  
XOR 1 into the last word of S. Transform S through F rounds.  
Finally, output the first L/byte_length bytes of S.

All block word indicies are modulo the number of words.


### schedule words

8-bit  
initialization value: AA  
addend: 1B

16-bit  
initialization value: AAAA  
addend: 3977

32-bit  
initialization value: AAAAAAAA  
addend: 89ABCDEF

64-bit  
initialization value: AAAAAAAAAAAAAAAA  
addend: 0123456789ABCDEF


### transformation function

SWN = state word number  
HSWN = half state word number  
REO = reorder array

use the bottom half of S to transform the top half  
reorder the top half of the words in S  
use the top half of S to transform the bottom half  
undo the reordering of the top half

`REO = reorder(SWN)`  
`A = initialization value`  
`for i = 0 to (HSWN * R) - 1 do`  
&emsp;`for j = 0 to HSWN - 1 do`  
&emsp;&emsp;`S[j + HSWN] = ((S[j + HSWN] XOR S[j]) <<< S[j]) + A`  
&emsp;&emsp;`A += addend`  
&emsp;`for j = 0 to HSWN - 1 do`  
&emsp;&emsp;`S[j] = ((S[j] XOR S[REO[j]]) <<< S[REO[j]]) + A`  
&emsp;&emsp;`A += addend`

### reorder function

Start with a list of numbers from 0 to SWN - 1 in array REO.  
Swap the numbers in the bottom half of the array with the numbers in the top half.  
Split the numbers in the bottom half of the array in half if there are more than one, and swap those halves.  
Continue splitting the bottom half and swapping the halves until it cannot be split anymore because the half only contains one number.

`SCO = SWN`  
`while SCO > 1 do`  
&emsp;`SCO /= 2`  
&emsp;`for i = 0 to SCO - 1 do`  
&emsp;&emsp;`TMP = REO[i]`  
&emsp;&emsp;`REO[i] = REO[i + SCO]`  
&emsp;&emsp;`REO[i + SCO] = TMP`
