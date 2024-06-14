# Компилятор

## Установка и запуск

```
git clone https://github.com/d3clane/Compiler.git
cd Src 
make buildDirs && make
```

При запуске кода из [examples](examples/) необходимо просто запустить скрипт `./run.bash`. 

Запуск только backend под x86_64 выглядит так:

```
./bin/backEnd [input AST] [out Binary] [optional]
```

Среди опциональных флагов на данный момент есть только `-S`, который аналогичен такому же в `gcc`, то есть включает создание ассемблерного файла с кодом.

Также при запуске скрипта `./run.bash` можно выбрать под какую архитектуру скомпилировать:
- `-march=elf64` - создание бинарного исполняемого файла elf64.
- `-march=spu57` - создание бинарного файла под мой [эмулятор процессора](https://github.com/d3clane/Processor-Emulator).

## Цель работы

В конце прошлого семестра мной был реализован собственный [язык программирования](https://github.com/d3clane/ProgrammingLanguage). Для исполнения написанных программ код сначала преобразовывался в AST, а затем в  ассемблерный код для моего эмулятора процессора. К сожалению, исполнение на эмуляторе занимает больше времени, чем на реальном. Основная цель работы - увеличение производительности написанного на моем языке кода. Для этого надо сразу преобразовывать в код, который сможет исполняться на нативной архитектуре - в данном случае x86_64. 

Сначала опишу устройство самого языка.

## Архитектура языка

Проект состоит из пяти частей:
1. [Frontend](#Frontend)
2. [Middle-end](#Middle-end)
3. [Backend](#Архитектура-backend)
4. [Back-frontend](#Back-frontend)

Фактически, это четыре независимых программы:

1. Frontend переводит написанный на моем языке код в [AST](#AST) - abstract syntax tree.
2. Middle-end упрощает полученное дерево.
3. Backend переводит AST в исполняемый бинарный файл. В проекте представлено два различных backend'а - для нативной архитектуры x86_64 и моего эмулятора собственного процессора.
4. Back-frontend - забавный бонус, который умеет переводить из AST в исходный код на моем языке.

Почему frontend, middle-end, backend - это три программы, а не одна монолитная? 

Представим ситуацию, что мне нужно реализовать frontend для $N$ различных языков программирования, а затем перевести их в $M$ различных ассемблеров. Если принять стандарт AST и переводить всегда в один и тот же вид, то получится, что мне надо реализовать всего $N$ frontend-ов и $M$ backend-ов, тем самым получив $N \cdot M$ различных вариантов компиляции. 

Если же писать монолитные программы, то потребуется $N \cdot M$ таких программ. Очевидно, что первое предпочтительнее, даже несмотря на то, что появятся лишние издержки на запись и чтение дерева из файла(временного хранилища между программами).

## AST 

AST(abstract syntax tree) - это представление какого-то исходного кода в виде подвешенного дерева. Каждая из вершин, у которой есть дети, описывает какую-то операцию(например, while или add). Листья же дерева описывают операнды(числа, переменные). 

Порядок действий, в котором надо выполнять какие-то действия определяется структурой дерева. Например, даже если в исходном коде у арифметических выражений были скобочки, которые определяли порядок выполнения действий, в AST их уже нет, а порядок определяется расположением операций друг относительно друга. Рассмотрим на примере:

Выражение int value = $(2 + 3) \cdot (4 + 5)$ выглядит так:

![add_mul](https://github.com/d3clane/ProgrammingLanguage/blob/main/ReadmeAssets/imgs/add_mul.png)

А выражение int value = $2 + 3 \cdot (4 + 5)$ вот так:

![mul_add](https://github.com/d3clane/ProgrammingLanguage/blob/main/ReadmeAssets/imgs/mul_add.png)

При спуске по дереву сначала полностью рассчитывается выражение в левом поддереве, потом в правом, а затем применяется операция в вершине к полученным результатам. Фактически, на первом примере сначала посчитается сумма $2 + 3$, затем $4 + 5$, а потом их произведение. Во втором же случае посчитается $4 + 5$, затем $3 \cdot (4 + 5)$, а затем $2 + 3 \cdot (4 + 5)$. 

При написании данного проекта у меня и у [metaironia](https://github.com/metaironia) был принят единый стандарт синтаксического дерева([кое-кто](https://github.com/worthlane) должен был присоединиться, но пока его представление дерева отличается). Таким образом, совершенно разные языки могут иметь один и тот же вид AST, а значит для них подходит один и тот же backend и middle-end. 

## Frontend

Frontend переводит написанный на моем языке код в AST. Здесь важно понять, как анализировать исходный код. Фактически, прежде чем начать писать язык, надо придумать синтаксис для него. Для этого зададим грамматику, по которой затем будем делать разбор языка с помощью алгоритма рекурсивного спуска.

Frontend также разделен на две части - лексический анализ и синтаксический анализ. Лексический анализ нужен, чтобы разбить исходный код на токены - единицы грамматики, которые затем легче анализировать. На этом этапе можно избавиться от комментариев, пробелов, отступов и тому подобных вещей в коде - то, что не влияет на результат исполнения.

Синтаксический анализ - часть, в которой непосредственно массив из лексем(токенов) превращается в AST. В моем случае это происходит с помощью рекурсивного спуска.

### Синтаксис

Грамматика:

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

Краткое справка по обозначениям:

- `[...]` - означает множество различный элементов в каком-то диапазоне. 
- `{ A | B | C | ...}` - означает, что на данном этапе должна появляться одна из грамматических конструкций в этом множестве. 
- `A+` - A повторяется один или более раз
- `A*` - A повторяется ноль или более раз
- `'A'` - значит, что должна встретиться непосредственно буква A.

Перечислю основные моменты, которые отличают мой язык программирования от C. Чтобы было понятнее, буду приводить примеры того, как бы это выглядело в C.

- В языке нет такого понятия, как запятая. Когда хочется перечислить аргументы или параметры функции, то достаточно просто поставить пробельный символ между ними. Вместо `func(val1, val2);` будет `func(val1 val2);`
- Какие-то скобки опущены. Например, при перечислении параметров функции в ее определении нет скобок, окружающих параметры. На примере C вместо `int func(int val1, int val2)` будет `int func int val1 int val2`. Также опущены открывающие скобки в конструкциях if / while.
- Если какая-то операция в языке возвращает результат, но при этом он никуда не присваивается, то считается, что это операция возврата из функции. Других способов вернуться из вызова функции нет. На примере C и функции рекурсивного вычисления факториала:

```
int Func(int n)
{
    if (n == 0)
        return 1;
    return Func(n - 1) * n;
}
```

Будет выглядеть так:

```
int Func(int n)
{
    if (n == 0)
        1;
    Func(n - 1) * n;
}
```

Конечно, из-за невнимательности может возникнуть проблема:

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

Программист ожидает, что вызовутся функции `Foo()`, `Bar()` и вернется 0, а на деле вернется результат функции `Foo()`, а дальнейший код не будет исполняться.

Это легко избежать, просто присваивая полученные значения:

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

- Все операции намеренно перепутаны: `+` это `-`, `*` это `/`, `<` это `>`, `=` это `==`.

Чтобы написать комментарий к коду, надо начать с символа `@`. 

Также, в языке присутствуют строковые литералы, которые предназначены только для использования вместе с оператором языка `print`. 

Примеры исходного кода на моем языке можно найти в папке [examples](examples/).

Перейдем к описанию обработки файлов исходного кода на моем языке.

### Лексический анализ 

Здесь ничего сложного. Просто проходясь по исходному коду, я превращаю последовательности символы в различные токены. Например, `575757` превратится в токен `TYPE_INT`, а `58` в число 58. В языке существуют неоднозначные конструкции(например, `57`), которые в разных контекстах могут означать разное. Например, иногда это может быть аналог `;`, а иногда аналог `}` в конструкциях if / while. В этом случае такие конструкции превращаются в токены вида `TOKEN_57`. 

На этом этапе также удаляются все комментарии, лишние пробелы и отступы, которые не влияют на результат исполнения.

### Рекурсивный спуск

После построения грамматики, в рекурсивном спуске нет никакой сложности. Все что надо - переписать действия, которые написаны в грамматике, в виде функций. Проходя по токенам, создавать вершины, если грамматическое правило верно, и подвязывать их, тем самым постепенно строя AST. Если ни одно из грамматических правил не подходит - выдать синтаксическую ошибку.

Пример AST, которое строится по исходному коду [программы для нахождения факториала числа](examples/factorial.txt):

![factorial](https://github.com/d3clane/ProgrammingLanguage/blob/main/ReadmeAssets/imgs/factorial.png)

Операции отмечены зеленым цветом, имена(переменных / функций / строковые литералы) голубым, числа темно-синим. 

## Middle-end

Middle-end на данный момент поддерживает сильно ограниченное количество оптимизаций, а конкретно всего две:

1. Свертка констант. Арифметические выражения, в которых не участвуют переменные, сворачиваются в одну константу. То есть, например, $(5 \cdot 6) + \frac{3}{1}$ свернется в одну вершину со значением $33$.
2. Удаление нейтральных вершин. Например, умножение любого выражения на ноль сворачивается в константу ноль. Или, умножение на единицу сворачивается просто в это же выражение, а единица и операция умножения удаляются.

Подобные оптимизации происходят до тех пор, пока они все еще могут происходить. Если после очередного цикла оптимизаций дерево не изменилось, middle-end завершает свою работу.

## Back-frontend

Это программа, которая по AST умеет строить исходный код. Зачем вообще это может быть нужно? Так как у меня и у [metaironia](https://github.com/metaironia) единый стандарт представления AST, можно переводить код из одного языка в другой - сначала из языка A в AST, а затем из AST в язык B. 

На примере, исходный код на языке metaironia:

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

Переводится сначала в AST, а затем моим back-frontend в исходный код на моем языке программирования:

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

К сожалению, обратного процесса не показать, у metaironia нет back-frontend.

## Архитектура backend

Здесь будет описание работы backend для перевода в исполняемый бинарный файл под x86_64. Как уже было сказано, в проекте также присутствует второй backend, его описание можно найти в моем [предыдущем проекте](https://github.com/d3clane/ProgrammingLanguage).

1. Backend получает на вход AST.
2. AST переводится в [IR](#Промежуточное-представление) - intermediate representation.
3. На основе IR создается исполняемый на x86_64 elf файл. 

Важно отметить, что арифметико-логические реализованы с помощью SSE инструкций. Благодаря этому, например, появляется возможность писать программу для решения квадратного уравнения на моем языке.

## Промежуточное представление

Intermediate representation(промежуточное представление) - способ представления исходного кода. В моем случае IR - двусвязный список, в котором хранятся инструкции, подобные ассемблерным. Структура одного элемента IR:

```
struct IRNode
{
    IROperation operation;          // Ассемблер-подобная инструкция(mov, pop, ...)
    char*       labelName;          // Имя метки. Какая-то нода может быть просто меткой

    size_t    numberOfOperands;     // Количество операндов у операции
    IROperand operand1;             // Первый операнд
    IROperand operand2;             // Второй операнд

    IRNode* jumpTarget;             // Куда указывает метка внутри 'JCC' или 'CALL'

    bool needPatch;                 // Нужно ли высчитывать jumpTarget на втором проходе создания IR

    size_t asmCmdBeginAddress;      // Адрес начала этой инструкции при переводе в ассемблер
    size_t asmCmdEndAddress;        // Адрес конца этой инструкции при переводе в ассемблер

    IRNode* nextNode;               // Указатель на следующую вершину
    IRNode* prevNode;               // Указатель на предыдущую вершину
};
```

Во-первых, такое промежуточное представление может быть использовано для оптимизаций. Например, в таком представлении хорошо видны последовательные `PUSH` / `POP`. Часто в таких случаях можно отказаться от использования стека. Как раз из-за того, что нужно удобно и быстро заменять инструкции в IR, удалять какие-то, вставлять новые, используется двусвязный список. На данный момент никакие оптимизации не применяются. 

Во-вторых, IR полезен, когда необходимо создавать исполняемый код под разные архитектуры. Так, не придется для каждой конкретной архитектуры писать общие оптимизации заново - все они могут быть произведены на стадии промежуточного представления, а значит, нужно будет реализовать только перевод из IR в инструкции для новой архитектуры, а также, возможно, какие-то специализированные под нее оптимизации.

В-третьих, с помощью IR можно удобно вычислять, куда указывает какой-нибудь `jmp`(`jcc`) или `call`. Для этого используется указатель `jumpTarget`, который заполняется на втором проходе.

## Создание ассемблерного файла 

Фактически, это не сложнее, чем то, что уже было мной реализовано для перевода в ассемблер моего эмулированного процессора. Основные отличия моего процессора:

- Произведение всех операций на стеке. То есть, например, арифметико-логические инструкции берут операнды со стека.
- Использование двух стеков - один стек для операций, другой для адресов возврата.
- Наличие оперативной памяти - некоторой области, в которой можно хранить локальные / глобальные переменные.

При переводе в код под x64 я отказался от использования двух стеков и оперативной памяти - локальные переменные и адреса возврата будут храниться на основном стеке. К локальным переменным на стеке я буду адресоваться через [rbp] регистр и смещения, а для этого понадобится [стековый фрейм](https://ru.wikipedia.org/wiki/Стековый_кадр).

## Кодирование инструкций 

Чтобы отказаться от nasm, необходимо понять, как самому закодировать инструкции. Информацию про это я брал с этого сайта: https://wiki.osdev.org/X86-64_Instruction_Encoding. 

На данный момент при кодировании инструкций, в которых используется какое-то константное значение, под него всегда выделяется максимальное число байт. То есть, например, если есть инструкция `PUSH IMM`, это всегда кодируется, как `PUSH IMM32` независимо от реального размера IMM.

Также поддерживается довольно ограниченное количество инструкций, которые получается закодировать - в основном это только те, которые могут на данный момент создаваться моим компилятором во время перевода в исполняемый файл.

## Создание elf64 файла

Изучим структуру elf64 файла:

![Elf64](https://github.com/d3clane/Compiler/blob/main/ReadmeAssets/imgs/elf64.png)

Src:https://scratchpad.avikdas.com/elf-explanation/elf-explanation.html

В моем случае меня интересует минимально возможный корректный elf64 файл. Для этого мне необходимы:

- Elf header. Заголовок elf файла с общей информацией о нем.
- Program header table. Заголовки, которые содержат информацию про различные области, где записаны какие-то данные / код, которые необходимо загрузить в память при исполнении программы. В моем случае у меня 3 различных header'а. 
- Сами данные / код, хранящиеся внутри elf файла. На картинке это `.text SECTION`. В моем elf файле будет 3 таких секции. 

### Elf header 

Заголовок elf файла содержит общую информацию про исполняемый файл, чтобы он корректно загружался в память. Как видно, в нем указано, что у меня в бинарнике 3 программных заголовка, а точка входа - сгенерированный программой код.

```
static const Elf64_Ehdr ElfHeader = 
{
    .e_ident = 
    {
        ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, // Magic 
        ELFCLASS64,                         // 64-bit system
        ELFDATA2LSB,                        // Little-endian
        EV_CURRENT,                         // Version = Current(1)
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
    .e_ehsize   = sizeof(Elf64_Ehdr),	       // header size

    .e_phentsize = sizeof(Elf64_Phdr),         // program header table one entry size
    .e_phnum     = 3,                          // Number of program header entries.

    .e_shentsize = sizeof(Elf64_Shdr),         // section header size in bytes
    .e_shnum     = 0,                          // number of entries in section header table(not used)
    .e_shstrndx  = SHN_UNDEF,                  // no section header string table
};
```

Три различных программных заголовка - 

1. Заголовок стандартной библиотеки, которая необходима, чтобы работали функции ввода, вывода, завершения программы.
2. Rodata, где хранятся строчки и константные значения чисел с плавающей запятой.
3. Код, сгенерированный моей программой.

### Заголовок стандартной библиотеки

```
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

Стандартная библиотека подгружается из заранее сгенерированного elf файла. Принцип, по которому это происходит объяснен [ниже](#Стандартная-библиотека).

### Заголовок rodata

```
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

Rodata предназначена для хранения констант, а потому в поле `p_flags` выставлены права только на чтение.

### Заголовок сгенерированного кода

```
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

Важно отметить, что в каждом из представленных заголовков поля `p_filesz` и `p_memsz` изначально пустые. Дело в том, что заранее неизвестно, сколько именно байт придется писать в файл(конечно, для кода стандартной библиотеки это заранее известно, но такой константы в коде я решил не заводить). Таким образом эти поля заполняются только после того, как станет уже точно известно, сколько байт занимает конкретный сегмент. 

### Стандартная библиотека

Стандартная библиотека необходима, так как мне нужны реализации функций `read`, `print`, `hlt`. Для этого реализуем их в одном ассемблерном файле, а затем скомпилируем с помощью nasm в elf файл.

Формат elf файла из которого подгружается стандартная библиотека можно увидеть с помощью readelf:

<img src="https://github.com/d3clane/Compiler/blob/main/ReadmeAssets/imgs/StdlibElfHeader.png" width="80%" height="80%">

![Program header](https://github.com/d3clane/Compiler/blob/main/ReadmeAssets/imgs/StdLibProgramHeader.png)

Для копирования стандартной библиотеки в бинарник мне необходимы программные заголовки `.text` и `.rodata`, а также соответствующие им данные/код. Они загружается по адресам `0x401000` и `0x402000` соответственно. Важно, что внутри есть адресация между сегментами(`.text` адресуется к данным в `.rodata`). То есть, чтобы не сломать эту адресацию, надо загружать эти сегменты в те же области памяти. В моем случае получается, что код стандартной библиотеки будет загружаться по адресу `0x401000`, а rodata по `0x402000`. Если бы адресация была относительная, а не абсолютная, то достаточно просто сохранить расположение этих сегментов друг относительно друга в памяти, но в данный момент это не реализовано.

Также нас интересуют адреса функций в стандартной библиотеки. Их я записал в виде констант в перечислении в файле [x64Elf.h](Src/Backend/TranslateFromIR/x64/x64Elf.h), чтобы потом иметь возможность их вызывать из сгенерированного кода.

Во время загрузки стандартной библиотеки сначала анализируется заголовок elf файла. Затем, основываясь на записанном в поле e_phoff смещении таблицы программных заголовков, адресуюсь к ней. Тут уже я предполагаю, что файл выглядит в соответствие с моими ожиданиями, а именно: заголовок кода стандартной библиотеки - второй по счету в таблице, rodata - третья по счету. Наконец, теперь по программным заголовкам можно определить местоположение и размер данных, которые теперь скопируем в elf файл для сгенерированного кода.

## Генерация кода

Во время генерации кода появляется проблема с такими инструкциями, как `call`, `jcc`. Благодаря применению IR, я знаю, на какую инструкции ссылаются они(`jumpTarget`), но, фактически, адрес этой инструкции в ассемблере может быть еще не подсчитан, так как трансляция до нее еще не дошла. Чтобы разрешить эту проблему, используется двухпроходная компиляция - на второй проход все адреса уже точно известны. 

Также при генерации кода для вызова функций на данный момент принято паскалевское соглашение. В языке нет функций с переменным числом аргументов, поэтому никаких ограничений такое соглашение не накладывает. Для выхода из функций используются инструкции вида `RET IMM16`, которые на выходе удаляют со стека последние `IMM16` байт.

## Сравнение производительности

Вернемся к основной цели работы - ускорение. Напишем две программы:

1. В цикле какое-то количество раз считаем факториал 6. 
2. Аналогично в цикле вычисляются корни квадратного уравнения $x^2+5x-7=0$

На моем эмуляторе собственного процессора будем выполнять действия в цикле $10^7$ раз, а в скомпилированном под x86_64 бинарнике $10^8$ раз. Время меряется с помощью time и исчисляется в секундах.

Эмулятор моего процессор компилируется с флагами `-Ofast -D NDEBUG` и с выключением логирования, канареек, хеширования и тд.

Запуск на эмуляторе собственного процессора:

|           | 1 запуск  | 2 запуск  | 3 запуск | Среднее        | Среднее на $10^7$ циклов    |
|:---:      |:---:      |:---:      |:---:     |:---:           |:---:                        | 
| Факториал | 11.478    | 11.580    | 12.032   | $11.7 \pm 0.3$ | $11.7 \pm 0.3$              |
| Квадратка | 16.863    | 16.498    | 16.409   | $16.6 \pm 0.2$ | $16.6 \pm 0.2$              |

Запуск сгенерированного бинарника:

|           | 1 запуск | 2 запуск  | 3 запуск | Среднее         | Среднее на $10^7$ циклов    |
|:---:      |:---:     |:---:      |:---:     |:---:            |:---:                        | 
| Факториал | 5.911    |  6.290    | 5.937    | $6.1 \pm 0.2$   | $0.61 \pm 0.02$             |
| Квадратка | 7.563    |  6.639    | 7.396    | $7.2 \pm 0.5$   | $0.72 \pm 0.05$             |

Сравнение:

|           | $10^7$ циклов x64 | $10^7$ циклов эмулятор | Ускорение        |
|:---:      |:---:              |:---:                   |:---:             |
| Факториал |$0.61 \pm 0.02$    |$11.7 \pm 0.3$          | $19.2 \pm 1.1$   |
| Квадратка |$0.72 \pm 0.05$    |$16.6 \pm 0.2$          | $23.1 \pm 1.9$   |

Очень значительный выигрыш в производительности - аж в 20 раз. При этом ускорение для решения квадратного уравнения больше. Возможно, это из-за того, что в эмуляторе собственного процессора используются вычисления с фиксированной точностью. При взятии корня необходимо сначала перевести целое число в xmm регистр, взять корень, вернуть в исходный регистр и сделать нужное умножение для сохранения точности. В то время как под x64 все вычисления сразу выполняются на xmm регистрах. При подсчете факториала такого фактора нет - там нигде не берется корень. 

Итак, исполнение программы на нативной архитектуре вместо эмулятора ускорило время работы в более чем 20 раз, а значит цель работы достигнута. 