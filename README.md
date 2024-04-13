# Overview
A lexer for the C langauge.

Given an input file of C syntax, the `clexer` tool will tokenize the symbols in the file and print them out.

For example, a simple file named `hello_world.c` containing the following code.
```C
#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("hello, clexer!\n");
    return 0;
}
```

When passed to `clexer`, yields the following result.

```sh
$ clexer hello_world.c
1: # include < stdio . h > 
3: int main ( int argc , char * argv [ ] ) { 
4: printf ( "hello, clexer!\n" ) ; 
5: return 0 ; 
6: } 
```

Right now, the tool prints the parsed tokens in human-readable form. Eventually, they'll be parsed into an Abstract Syntax Tree for more useful work.

