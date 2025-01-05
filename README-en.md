# Compiler

Translated using ChatGPT.

## Installation and Execution

```
git clone https://github.com/d3clane/Compiler.git
cd Src
make buildDirs && make
```

To run code from [examples](examples/), simply execute the `./run.bash` script.

Running the backend only for x86\_64 can be done as follows:

```
./bin/backEnd [input AST] [out Binary] [optional]
```

The optional flags currently include only `-S`, which is similar to the same flag in `gcc`, meaning it enables the creation of an assembly file with code.

When running the `./run.bash` script, you can also choose the architecture to compile for:

- `-march=elf64` - creates a binary executable in elf64 format.
- `-march=spu57` - creates a binary file for my [processor emulator](https://github.com/d3clane/Processor-Emulator).

## Goal of the Project

At the end of the last semester, I implemented my own [programming language](https://github.com/d3clane/ProgrammingLanguage). To execute written programs, the code was first converted into AST and then into assembly code for my processor emulator. Unfortunately, execution on the emulator takes more time than on real hardware. The primary goal of this project is to increase the performance of the code written in my language. To achieve this, the code needs to be directly converted into native architecture-compatible code—in this case, x86\_64.

First, I will describe the structure of the language.

## Language Architecture

The project consists of five components:

1. [Frontend](#Frontend)
2. [Middle-end](#Middle-end)
3. [Backend](#Backend-Architecture)
4. [Back-frontend](#Back-frontend)

These are effectively four independent programs:

1. The frontend translates the code written in my language into an [AST](#AST) (abstract syntax tree).
2. The middle-end simplifies the resulting tree.
3. The backend converts the AST into an executable binary file. The project features two different backends—one for the native x86\_64 architecture and one for my custom processor emulator.
4. The back-frontend is a fun bonus that can convert the AST back into source code in my language.

Why are the frontend, middle-end, and backend three separate programs instead of a single monolithic one?

Consider a scenario where I need to implement a frontend for \$N\$ different programming languages and then translate them into \$M\$ different assemblers. If we adopt a standard AST format and always translate to the same representation, it would mean implementing only \$N\$ frontends and \$M\$ backends, resulting in \$N \cdot M\$ compilation options.

If we were to write monolithic programs, \$N \cdot M\$ such programs would be required. Obviously, the first option is preferable, even though it introduces overhead in writing and reading the tree to/from a file (temporary storage between programs).

## AST

AST (abstract syntax tree) is a representation of source code as a tree structure. Each node with children describes an operation (e.g., `while` or `add`). The leaves of the tree describe operands (numbers, variables).

The order in which actions should be performed is determined by the tree structure. For example, even if parentheses in arithmetic expressions in the source code defined the order of operations, they are not present in the AST—the order is instead determined by the relationships between operations. Consider the following examples:

The expression `int value = (2 + 3) * (4 + 5)` looks like this:

![add_mul](https://github.com/d3clane/ProgrammingLanguage/blob/main/ReadmeAssets/imgs/add_mul.png)

The expression `int value = 2 + 3 * (4 + 5)` looks like this:

![mul_add](https://github.com/d3clane/ProgrammingLanguage/blob/main/ReadmeAssets/imgs/mul_add.png)

While traversing the tree, the left subtree is fully evaluated first, followed by the right subtree, and finally, the operation at the root is applied to the results. In the first example, the sum `2 + 3` is calculated first, then `4 + 5`, and finally, their product. In the second example, `4 + 5` is calculated first, followed by `3 * (4 + 5)`, and finally `2 + 3 * (4 + 5)`.

In this project, [metaironia](https://github.com/metaironia) and I agreed on a unified syntax tree standard (although [worthlane](https://github.com/worthlane) was supposed to join, his tree representation differs). Thus, completely different languages can share the same AST format, making them compatible with the same backend and middle-end.

## Frontend

The frontend translates code written in my language into an AST. It's essential to understand how to analyze source code. Essentially, before writing the language, you need to design its syntax. For this, a grammar is defined, which is then used to parse the language using a recursive descent algorithm.

The frontend is also divided into two parts—lexical analysis and syntax analysis. Lexical analysis breaks the source code into tokens—grammar units that are easier to analyze. At this stage, comments, spaces, indents, and similar elements of the code that don't affect execution results can be removed.

Syntax analysis turns an array of lexemes (tokens) into an AST using recursive descent.

### Syntax

Grammar:

```
Grammar          ::= FUNC+ '\0'
FUNC             ::= FUNC_DEF
FUNC_DEF         ::= TYPE VAR FUNC_VARS_DEF '57' OP '{'
FUNC_VAR_DEF     ::= {TYPE VAR}*
OP               ::= { IF | WHILE | '57' OP+ '{' | {VAR_DEF | PRINT | ASSIGN | RET} '57' }
IF               ::= '57?' OR '57' OP
WHILE            ::= '57!' OR '57' OP
RET              ::= OR
VAR_DEF          ::= TYPE VAR '==' OR
PRINT            ::= '{' { ARG | CONST_STRING }
READ             ::= '{'
ASSIGN           ::= VAR '==' OR
OR               ::= AND {and AND}*
AND              ::= CMP {or CMP}*
CMP              ::= ADD_SUB {[<, <=, >, >=, =, !=] ADD_SUB}*
ADD_SUB          ::= MUL_DIV {[+, -] MUL_DIV}*
MUL_DIV          ::= POW {[*, /] POW}*
POW              ::= FUNC_CALL {['^'] FUNC_CALL}*
FUNC_CALL        ::= IN_BUILT_FUNCS | MADE_FUNC_CALL | EXPR
IN_BUILT_FUNCS   ::= [sin/cos/tan/cot/sqrt] '(' EXPR ')' | READ
MADE_FUNC_CALL   ::= VAR '{' FUNC_VARS_CALL '57'
FUNC_VARS_CALL   ::= {OR}*
EXPR             ::= '(' OR ')' | ARG
ARG              ::= NUM | GET_VAR
NUM              ::= ['0'-'9']+
VAR              ::= ['a'-'z' 'A'-'Z' '_']+ ['a'-'z' 'A'-'Z' '_' '0'-'9']*
CONST_STRING     ::= '"' [ANY_ASCII_CHAR]+ '"'
TYPE             ::= 575757
```

Key notation:

- `[...]` - denotes a range of possible elements.
- `{ A | B | C | ...}` - means that at this point, one of the grammar constructs in this set must appear.
- `A+` - A repeats one or more times.
- `A*` - A repeats zero or more times.
- `'A'` - the literal character `A` must appear.

Here are the main differences between my programming language and C. For clarity, I will provide examples of how they would look in C.

- There is no comma concept in the language. When listing arguments or function parameters, a space character suffices. Instead of `func(val1, val2);`, it would be `func(val1 val2);`.
- Certain brackets are omitted. For example, when listing function parameters in its definition, the enclosing parentheses are not present. In C, instead of `int func(int val1, int val2)`, it would be `int func int val1 int val2`. Opening brackets in `if` / `while` constructs are also omitted.
- If an operation in the language returns a result but is not assigned anywhere, it is considered a return operation. Other methods of returning from a function call do not exist. For example, the C recursive factorial function:

```
int Func(int n)
{
    if (n == 0)
        return 1;
    return Func(n - 1) * n;
}
```

Would look like this:

```
int Func(int n)
{
    if (n == 0)
        1;
    Func(n - 1) * n;
}
```

This can lead to issues due to oversight:

```
int Foo();
int Bar();

int Func()
{
    Foo();
    Bar();

    0;
}
```

The programmer expects `Foo()` and `Bar()` to be executed and return 0, but in reality, the result of `Foo()` is returned, and the subsequent code is not executed.

To avoid this, simply assign the returned values:

```
int Foo();
int Bar();

int Func()
{
    int tmp = Foo();
    tmp     = Bar();

    0;
}
```

- All operations are intentionally swapped: `+` is `-`, `*` is `/`, `<` is `>`, `=` is `==`.

To write a comment, start with the symbol `@`.

The language also supports string literals, which are only intended for use with the `print` operator.

Examples of source code in my language can be found in the [examples](examples/) folder.

Next, let's discuss processing source code files in my language.

### Lexical Analysis

This stage is straightforward. By iterating through the source code, I convert character sequences into various tokens. For example, `575757` becomes the token `TYPE_INT`, and `58` becomes the number 58. The language includes ambiguous constructs (e.g., `57`), which may represent different things in different contexts. For instance, it might be equivalent to `;` or `}` in `if` / `while` constructs. Such constructs are turned into tokens like `TOKEN_57`.

At this stage, all comments, unnecessary spaces, and indents that do not affect execution are removed.

### Recursive Descent

Once the grammar is defined, recursive descent is straightforward. All that is required is to rewrite the actions specified in the grammar as functions. By iterating through tokens, creating nodes when the grammar rule is satisfied, and linking them, the AST is gradually built. If no grammar rule applies, a syntax error is raised.

An example AST generated for the source code of the [factorial program](examples/factorial.txt):

![factorial](https://github.com/d3clane/ProgrammingLanguage/blob/main/ReadmeAssets/imgs/factorial.png)

Operations are marked in green, names (variables / functions / string literals) in blue, and numbers in dark blue.

## Middle-end

The middle-end currently supports a very limited number of optimizations:

1. Constant folding. Arithmetic expressions that do not involve variables are reduced to a single constant. For example, `$(5 \cdot 6) + \frac{3}{1}$` is reduced to a single node with the value \$33\$.
2. Removal of neutral nodes. For example, multiplying any expression by zero is reduced to the constant zero. Similarly, multiplying by one reduces the expression, removing the multiplication operation and the one.

These optimizations continue until no further changes occur. If the tree remains unchanged after another optimization cycle, the middle-end completes its work.

## Back-frontend

The back-frontend is a program capable of converting an AST back into source code. Why might this be necessary? Since [metaironia](https://github.com/metaironia) and I use a unified AST standard, it is possible to translate code from one language to another—first from language A to AST, and then from AST to language B.

For example, source code in metaironia's language:

```
ща на main блоке,
мы делаем банкноты
сюда x теперь читаю йоу 
сюда мой opp будет звонить factorial x гоу
флоу плохой сказал дурак opp йоу
ладно верни тогда 0;
йоу

factorial с гэнгом n эй
если n будто 1 эй
отдавай 1 без сомнений
йоу гоу
сюда x равно n минус 1 йо
верни пуллап в factorial
x множит n миллениал йоу йоу
```

Is first translated into an AST, and then using my back-frontend, converted into source code in my programming language:

```
575757 factorial 575757 n 
57
    57? (n != 1 ) 57
    57
        1 57
    {
    575757 x == ((n + 1 ) + (0 / 2 ) ) 57
    (factorial { x 57 / n ) 57
{

575757 main 
57
    575757 x == { 57
    575757 opp == factorial { x 57 57
    . opp 57
    0 57
{
```

Unfortunately, the reverse process cannot be demonstrated, as metaironia does not have a back-frontend.

## Backend Architecture

This section describes the backend responsible for translating an AST into an executable binary file for x86\_64. As mentioned earlier, the project also includes a second backend, which is described in my [previous project](https://github.com/d3clane/ProgrammingLanguage).

1. The backend takes the AST as input.
2. The AST is translated into [IR](#Intermediate-Representation) (Intermediate Representation).
3. The IR is used to create an executable ELF file for x86\_64.

It is worth noting that arithmetic and logical operations are implemented using SSE instructions. This allows, for example, the creation of a program to solve quadratic equations in my language.

## Intermediate Representation

Intermediate Representation (IR) is a way to represent source code. In my case, IR is a doubly linked list that stores instructions similar to assembly language. The structure of an IR element is:

```
struct IRNode
{
    IROperation operation;          // Assembly-like instruction (mov, pop, ...)
    char*       labelName;          // Label name. Some nodes may simply be labels.

    size_t    numberOfOperands;     // Number of operands for the operation
    IROperand operand1;             // First operand
    IROperand operand2;             // Second operand

    IRNode* jumpTarget;             // Target address for 'JCC' or 'CALL'

    bool needPatch;                 // Indicates if jumpTarget needs to be calculated in a second IR pass

    size_t asmCmdBeginAddress;      // Start address of this instruction when translated to assembly
    size_t asmCmdEndAddress;        // End address of this instruction when translated to assembly

    IRNode* nextNode;               // Pointer to the next node
    IRNode* prevNode;               // Pointer to the previous node
};
```

First, IR can be used for optimizations. For example, sequences of `PUSH`/`POP` operations are clearly visible in this representation. In many cases, the use of a stack can be eliminated. The doubly linked list structure is chosen because it allows efficient insertion, deletion, and replacement of instructions in IR. Currently, no optimizations are implemented.

Second, IR is useful when creating executable code for different architectures. General optimizations can be applied at the IR stage, so only the translation from IR to architecture-specific instructions needs to be implemented, along with any architecture-specific optimizations.

Third, IR simplifies computing the target of `jmp`/`jcc`/`call` instructions. The `jumpTarget` pointer is filled during a second pass.

## Generating an Assembly File

This is no more complex than what I have already implemented for translating to assembly for my emulated processor. The key differences between my processor and x86\_64 are:

- All operations are performed on the stack. For example, arithmetic and logical instructions take their operands from the stack.
- Two stacks are used—one for operations and one for return addresses.
- There is a memory region for storing local/global variables.

When translating to x64, I avoided using two stacks and dedicated memory regions—local variables and return addresses are stored on the main stack. I address local variables on the stack using the [rbp] register and offsets, requiring a [stack frame](https://en.wikipedia.org/wiki/Call_stack#Structure_of_a_stack_frame).

## Encoding Instructions

To avoid relying on nasm, I needed to understand how to encode instructions myself. I referred to the following resource for this: [https://wiki.osdev.org/X86-64\_Instruction\_Encoding](https://wiki.osdev.org/X86-64_Instruction_Encoding).

Currently, when encoding instructions involving constant values, the maximum number of bytes is always allocated for the constant. For instance, a `PUSH IMM` instruction is always encoded as `PUSH IMM32`, regardless of the actual size of IMM.

Only a limited set of instructions can be encoded—primarily those currently generated by my compiler during translation to an executable file.

## Creating an ELF64 File

Let's examine the structure of an ELF64 file:

![Elf64](https://github.com/d3clane/Compiler/blob/main/ReadmeAssets/imgs/elf64.png)

Source: https://scratchpad.avikdas.com/elf-explanation/elf-explanation.html

In my case, I am interested in the smallest possible valid ELF64 file. For this, I need:

- Elf header: The ELF file header contains general information about the file.
- Program header table: Headers that contain information about various regions where some data/code resides, which need to be loaded into memory during program execution. In my case, there are 3 different headers.
- The actual data/code stored inside the ELF file. In the image, this is the `.text SECTION`. My ELF file will contain 3 such sections.

### ELF Header

The ELF file header contains general information about the executable file to ensure it is loaded correctly into memory. As seen, it specifies that my binary contains 3 program headers and that the entry point is code generated by the program.

```c
static const Elf64_Ehdr ElfHeader =
{
    .e_ident =
    {
        ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, // Magic
        ELFCLASS64,                         // 64-bit system
        ELFDATA2LSB,                        // Little-endian
        EV_CURRENT,                         // Version = Current (1)
        ELFOSABI_SYSV,                      // Unix SYSTEM V
        0                                   // filling other part with zeroes
    },

    .e_type    = ET_EXEC,                   // executable
    .e_machine = EM_X86_64,                 // x86_64
    .e_version = EV_CURRENT,                // current version

    .e_entry    = (Elf64_Addr)SegmentAddress::PROGRAM_CODE,
    .e_phoff    = sizeof(Elf64_Ehdr),          // program header table right after elf header

    .e_shoff    = 0,                           // segment header table offset - not used

    .e_flags    = 0,                           // no flags
    .e_ehsize   = sizeof(Elf64_Ehdr),          // header size

    .e_phentsize = sizeof(Elf64_Phdr),         // program header table one entry size
    .e_phnum     = 3,                          // Number of program header entries.

    .e_shentsize = sizeof(Elf64_Shdr),         // section header size in bytes
    .e_shnum     = 0,                          // number of entries in section header table (not used)
    .e_shstrndx  = SHN_UNDEF,                  // no section header string table
};
```

Three different program headers:

1. Header for the standard library, which is required for input/output and program termination functions.
2. Rodata, which stores strings and constant floating-point values.
3. Code generated by my program.

### Standard Library Header

```c
static const Elf64_Phdr StdLibPheader =
{
    .p_type   = PT_LOAD,             
    .p_flags  = PF_R | PF_X,                             // read and execute
    .p_offset = (Elf64_Off) SegmentFilePos::STDLIB_CODE, // code offset in file
    .p_vaddr  = (Elf64_Addr)SegmentAddress::STDLIB_CODE, // virtual addr
    .p_paddr  = (Elf64_Addr)SegmentAddress::STDLIB_CODE, // physical addr

    // Can't specify at this moment
    .p_filesz = 0,                                       // number of bytes to load from file
    .p_memsz  = 0,                                       // number of bytes to load in mem

    .p_align  = 0x1000,                                  // 1 page alignment
};
```

The standard library is loaded from a pre-generated ELF file. The principle behind this is explained [below](#Standard-Library).

### Rodata Header

```c
static const Elf64_Phdr RodataPheader =
{
    .p_type   = PT_LOAD,             
    .p_flags  = PF_R,                               // read
    .p_offset = (Elf64_Off) SegmentFilePos::RODATA, // code offset in file
    .p_vaddr  = (Elf64_Addr)SegmentAddress::RODATA, // virtual addr
    .p_paddr  = (Elf64_Addr)SegmentAddress::RODATA, // physical addr

    // Can't specify at the moment
    .p_filesz = 0,                                  // number of bytes to load from file
    .p_memsz  = 0,                                  // number of bytes to load in mem

    .p_align  = 0x1000,                             // 1 page alignment
};
```

Rodata is used for storing constants, and thus, the `p_flags` field is set to read-only permissions.

### Generated Code Header

```c
static const Elf64_Phdr ProgramCodePheader =
{
    .p_type   = PT_LOAD,             
    .p_flags  = PF_R | PF_X,                              // read and execute
    .p_offset = (Elf64_Off) SegmentFilePos::PROGRAM_CODE, // code offset in file
    .p_vaddr  = (Elf64_Addr)SegmentAddress::PROGRAM_CODE, // virtual addr
    .p_paddr  = (Elf64_Addr)SegmentAddress::PROGRAM_CODE, // physical addr

    // Can't specify at this moment
    .p_filesz = 0,                                        // number of bytes to load from file
    .p_memsz  = 0,                                        // number of bytes to load in mem

    .p_align  = 0x1000,                                   // 1 page alignment
};
```

It is important to note that in each of the presented headers, the `p_filesz` and `p_memsz` fields are initially empty. This is because it is not known in advance how many bytes will need to be written to the file (of course, for the standard library code, this is known in advance, but I chose not to introduce such a constant in the code). Therefore, these fields are only filled in once it is clear exactly how many bytes each segment occupies.

### Standard Library

The standard library is required because I need implementations of functions like `read`, `print`, and `hlt`. To do this, I implement them in a single assembly file and then compile it into an ELF file using NASM.

The ELF file format from which the standard library is loaded can be seen using `readelf`:

<img src="https://github.com/d3clane/Compiler/blob/main/ReadmeAssets/imgs/StdlibElfHeader.png" width="80%" height="80%">

![Program header](https://github.com/d3clane/Compiler/blob/main/ReadmeAssets/imgs/StdLibProgramHeader.png)

To copy the standard library into the binary, I need the `.text` and `.rodata` program headers and their corresponding data/code. They are loaded at addresses `0x401000` and `0x402000`, respectively. It is important that there is addressing between segments (`.text` addresses data in `.rodata`). To avoid breaking this addressing, these segments must be loaded into the same memory areas. In my case, the standard library code will be loaded at address `0x401000`, and rodata at `0x402000`. If the addressing were relative rather than absolute, it would suffice to maintain the relative positioning of these segments in memory, but this is not currently implemented.

We are also interested in the addresses of functions in the standard library. I recorded these as constants in an enumeration in the file [x64Elf.h](Src/Backend/TranslateFromIR/x64/x64Elf.h) to allow them to be called from the generated code.

During the loading of the standard library, the ELF file header is first analyzed. Then, based on the offset of the program header table recorded in the `e_phoff` field, I address it. Here, I assume that the file conforms to my expectations, specifically: the standard library code header is the second in the table, and rodata is the third. Finally, based on the program headers, it is possible to determine the location and size of the data, which are then copied into the ELF file for the generated code.

## Code Generation

During code generation, there is a problem with instructions like `call` and `jcc`. Thanks to the use of IR, I know the instruction they reference (`jumpTarget`), but the actual address of this instruction in assembly may not yet be calculated because translation has not reached it. To solve this problem, two-pass compilation is used—on the second pass, all addresses are already known.

Additionally, for function calls, Pascal calling convention is currently used. The language has no functions with a variable number of arguments, so this convention imposes no restrictions. To exit functions, instructions of the form `RET IMM16` are used, which remove the last `IMM16` bytes from the stack upon exit.

## Performance Comparison

Let's return to the main goal of the work—speedup. Write two programs:

1. In a loop, compute the factorial of 6 some number of times.
2. Similarly, compute the roots of the quadratic equation $x^2+5x-7=0$ in a loop.

On my emulator of a custom processor, these operations will be performed $10^7$ times, while on a compiled x86_64 binary, $10^8$ times. The time is measured using `time` and is expressed in seconds.

The custom processor emulator is compiled with the flags `-Ofast -D NDEBUG`, with logging, canaries, hashing, etc., disabled.

Execution on the custom processor emulator:

|           | Run 1   | Run 2   | Run 3   | Average         | Average per $10^7$ cycles |
|---      |---    |---    |---    |---           |---                       |
| Factorial | 11.478  | 11.580  | 12.032  | $11.7 ± 0.3$ | $11.7 ± 0.3$             |
| Quadratic | 16.863  | 16.498  | 16.409  | $16.6 ± 0.2$ | $16.6 ± 0.2$             |

Execution of the generated binary:

|           | Run 1   | Run 2   | Run 3   | Average         | Average per $10^7$ cycles |
|---      |---    |---    |---    |---           |---                       |
| Factorial | 5.911   | 6.290   | 5.937   | $6.1 ± 0.2$   | $0.61 ± 0.02$           |
| Quadratic | 7.563   | 6.639   | 7.396   | $7.2 ± 0.5$   | $0.72 ± 0.05$           |

Comparison:

|           | $10^7$ cycles x64 | $10^7$ cycles emulator | Speedup         |
|---      |---              |---                   |---            |
| Factorial |$0.61 ± 0.02$    |$11.7 ± 0.3$          | $19.2 ± 1.1$  |
| Quadratic |$0.72 ± 0.05$    |$16.6 ± 0.2$          | $23.1 ± 1.9$  |

A significant performance gain—up to 20 times faster. The speedup is greater for solving the quadratic equation. This may be because the custom processor emulator uses fixed-point calculations. When taking a square root, it is necessary to first convert the integer to an XMM register, take the root, return it to the original register, and perform the necessary multiplication to maintain precision. Meanwhile, on x64, all calculations are performed directly on XMM registers. For factorial calculations, there is no such factor—a root is never taken.

Thus, executing the program on the native architecture instead of the emulator sped up execution time by more than 20 times, achieving the goal of the work.
