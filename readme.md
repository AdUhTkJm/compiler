## C Compiler

A small C compiler inspired by 9cc.

#### Usage

This compiler converts C source file into x86-64 assembly of NASM syntax. It receives input from stdin and outputs to stdout.

It uses Sys V user-space calling convention, so the assembly file it produces should only work on Unix systems.

To produce an executable file, you can assemble the output to obtain an object file, and link it with libc. If you are using GCC for this, enable the `-no-pie` option.

Some test cases are included in `test` folder.

#### Implementation Progress

1. plus and minus operators.

2. multiplication, brackets and return statement.

3. function definition and int-typed variables.

4. global variables.

5. function calls.

6. if-, while- and for-statements; comparison operators.

7. compound operators (+=, -= etc). long, short and char.

8. basic semantics checking; pointers.

9. arrays; literal strings; variadic arguments

#### Unsupported features
These features might be added in the future.

- initialisation of global variables

- break, continue, switch-case

- extern

- struct, typedef and sizeof

- more operators (&&, || etc.)

- function pointers

- floating-point numbers

- preprocessing; the line-continuing backslash

- optimisation
