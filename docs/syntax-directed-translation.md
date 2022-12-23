# 语法制导翻译 (Syntax-Directed Translation, SDT)

## 中间代码生成简介

以下述mini-c代码为例

```c
int func(int x, int y) {
   int a;
   a = x + y;
   return a;
}

void main(void) {
   int x;
   int y;
   x = 5;
   y = 3;
   x = func(x, y);
   x = x + 3;
   return;
}
```

旨在大致生成如下的中间代码 (不严谨)

```
func:
    %x = pop
    %y = pop
    %t1 = add %x %y
    %a = %t1
    ret %a

main:
    %t1 = assign 5
    %x = %t1
    %t2 = assign 3
    %y = %t2
    push %y
    push %x
    %t3 = call func
    %x2 = %t3
    %t4 = assign 3
    %t5 = add %x2 %t4
    %x3 = %t5
    halt
```

参考llvm的中间表示 (`$ clang -emit-llvm -S -c -fno-discard-value-names 06.c -o 06.ll`)

```
define dso_local i32 @func(i32 %x, i32 %y) #0 {
entry:
  %x.addr = alloca i32, align 4
  %y.addr = alloca i32, align 4
  %a = alloca i32, align 4
  store i32 %x, i32* %x.addr, align 4
  store i32 %y, i32* %y.addr, align 4
  %0 = load i32, i32* %x.addr, align 4
  %1 = load i32, i32* %y.addr, align 4
  %add = add nsw i32 %0, %1
  store i32 %add, i32* %a, align 4
  %2 = load i32, i32* %a, align 4
  ret i32 %2
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @main() #0 {
entry:
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  store i32 5, i32* %x, align 4
  store i32 3, i32* %y, align 4
  %0 = load i32, i32* %x, align 4
  %1 = load i32, i32* %y, align 4
  %call = call i32 @func(i32 %0, i32 %1)
  store i32 %call, i32* %x, align 4
  %2 = load i32, i32* %x, align 4
  %add = add nsw i32 %2, 3
  store i32 %add, i32* %x, align 4
  ret void
}
```

## 方案

大部分属性是综合的(Synthetic, S-attribute), 但也有些是继承的(inHerited, H-attribute),
前者可以在lr1归约时同时完成处理(后序遍历), 后者必须先构造完语法分析树, 才能执行翻译(中序遍历).

## 约定

采用四元式存储三地址码指令.

```c++
struct QuadTuple {
    using Label = uint32_t; // label of address, not memeory address
    using Varname = uint32_t;
    using ConstVal = uint32_t;
    enum class Op {
        kAssgin, // (var, val)
        kCopy, // (var, src1)
        kAdd, kMinus, kMult, kDiv, kMod, // (dest, src1, src2)
        kAnd, kOr, kXor, kNor, // (dest, src1, src2)
        kSll, kSrl, // (dest, src1, src2)
        kEq, kNeq, kLeq, kLt, // (dest, src1, src2)
        kAlloate, // (var_m, offset)
        kLoad, kStore, // (var_m, addr, offset)
        kPush, kPop, // (var)
        kCall, // (var_j)
        kJump, // (var_j)
        kBranch, // (var_j, addr1, addr2)
        // type: var(varname)
        kRet, // (var)
        kHalt, // ()
    };
    Op  op;
    union Data {
        struct { Varname var;   ConstVal val; }; // assign
        struct { Varname dest;  Varname src1; Varname src2; }; // calculate
        struct { Varname var_m; Varname addr; ConstVal offset; }; // mem load/store
        struct { Varname var_j; Label addr1;  ConstVal addr2; }; // jump, branch
    } args;
};
```

方便起见，约定变量 `%0` 为常数0.

不遵守单变量赋值约定(SSA), 懒得搞 $\phi$ 函数

因为minic不支持传指针、取地址，我们可以约定所有的局部变量仅分配在寄存器上.

## 表达式

考虑下述代码

```c++
    a = (3 + c) * d + e;
```

翻译成如下中间代码

```
    %t1 = assign 3
    %t2 = %t1 add %c
    %t3 = %t2 mult %d
    %t4 = %t3 add %e
    %a = %t4
```

## 数组

一维数组的声明必然伴随着内存访问, 取数组元素的地址全程是变量运算, 不用到常数offset.
局部的一维数组在声明时由栈区分配空间，其变量名存放栈指针首地址.

```c++
void main(void) {
    int a[5];
    a[2] = a[3];
}
```

翻译如下

```
main:
    %a = allocate 5
    %t1 = assign 3
    %t2 = assign 4 (size of int32_t)
    %t3 = mult %t1 %t2
    %t4 = add %a %t3 (address of a[3])
    %t5 = load %t4 %0 (%t5 = a[3])
    %t6 = assign 2
    %t7 = assign 4
    %t8 = mult %t1 %t2
    %t9 = add %a %t8 (address of a[2])
    store %t9 %0 = %5
```

## 布尔表达式

使用粗暴的布尔化技术, 完全不考虑短路特性, 对所有布尔算符的操作数作强制布尔类型转化, 考虑下述代码

```c++
    a = (b && c) || (d > e);
```

翻译如下:

```
    %t0 = %b neq %0 (等价于 t0 = bool(b))
    %t1 = %c neq %0
    %t2 = %t0 and %t1
    %t3 = le %e %d
    %t4 = %t2 neq %0
    %t5 = %t3 neq %0
    %t6 = %t4 or %t5
    %a = %t6 (哈哈)
```

## 控制流

使用粗暴的控制流翻译技术, 哪怕会产生一大堆冗余的标号 (哈哈, 我是笨比)

```c++
    if (a) {
        b = 1;
    } else {
        if (b) {
            c = 1;
        } else {
            d = 1;
        }
    }
```

翻译如下:

```
    %t0 = %a neq %0
    branch %t0, if1.true, if1.false
if1.true:
    assign %b = 1
    jump if1.end
if1.false:
    %t1 = %b neq %0
    branch %t0, if2.true, if2.false
if2.true:
    assign %c = 1
if2.false:
    assign %d = 1
if2.end:
if1.end:
```

## 过程调用

约定所有参数都通过压栈传入, 调用方从后往前压入，函数内部从前向后取出.

```c++
int func(int x, int y) {
   int a;
   a = x + y;
   return a;
}

void main(void) {
    int x;
    x = func(3, 4);
    return;
}
```

翻译如下

```
func:
    %x = pop
    %y = pop
    ret %x

main:
    assign %t1 = 4
    push %t1
    assign %t2 = 3
    push %t2
    %x = call func
    halt
```

## 局部变量

上述约定部分废弃（等我写完再来该文档），请以以下内容为准 (2022-12-14, lee)

mini-c 代码

```c
    int x;
    int y;
    x = 4;
    y = x + 3;
```

翻译成中间代码

```
    %x = alloca int
    %y = alloca int
    assign %3 = 4
    store %x = %3
    load  %4 = %x
    assign %5 = 3
    %6 = add %4 %5
    store %y = %6
```

汇编时，将局部变量替换为和栈指针相关的相对偏移地址，

```
    %x = alloca int
    %y = alloca int                 addui   $sp, $sp, -8
    assign %3 = 4                   addui   $t0, $0, 4
    store %x = %3                   sw      $t3, 0($sp)
    load  %4 = %x                   lw      $t4, 0($sp)
    assign %5 = 3                   addui   $t5, $0, 3
    %6 = add %4 %5                  addu    $t6, $t4, $t5
    store %y = %6                   sw      $t6, 4($sp)
```

中间代码的alloca显示地指定了：

1. 在栈空间上分配相应大小的空间：sp = sp - 4
2. 在定义域内为该变量分配相应的内存位置：对x的读写等价于对内存0(sp)的读写
栈空间的释放不由中间代码显示指定，在函数结束时补上.

## 函数调用

mini-c 代码

```c
int func(int x, int y) {
    int z;
    z = x + y;
    return z;
}

int main() {
    int z;
    z = func(3, 4);
}
```

中间代码

```
func @func int(int %x, int %y) {
                            // 交代函数定义: 返回值类型，参数类型（必须有名），存放在全局定义表里，不需要以三地址码形式存放
    %retval = alloca int
    %x = alloca int
    %y = alloca int
    %z = alloca int         // 把所有alloca连续放在函数定义体最开头，先是返回值，然后是参数，最后是局部变量定义; 连续的alloca应视作一整部分
    store %retval = %zero
    $x = param get 0
    $y = param get 1
                            // %x.param, %y.param作为参数的值何时得到在中间代码中不可见；
                            // 上述步骤是初始化步骤，包括了: 1) 栈空间分配; 2) 返回值默认值设置; 3) 函数参数初始化;
    load  %1 = %x
    load  %2 = %y
    %3 = add %1, %2
    store %z = %3
    load  %4 = %z           // 看起来有点冗余，但这样就好

    store %retval, %4
    goto %ret               // 对return z的翻译
                            // 上述步骤是函数主体的翻译，注意到无名变量的运算可以只在寄存器上进行，而有名变量的读写必须以内存为起点、终点
ret:                        // 代码结束段需要释放栈空间，中间代码负责将多处return通过跳转简并为唯一的return，约定ret始终位于函数定义体尾部
    %5 = load %retval
    return %5
}

func @main int(void) {      // 函数和函数之间可以认为是毫无关联的
    %retval = alloca int
    %z = alloca int
    store %retval = %zero

    %6 = assign 3
    %7 = assign 4

    param put %6
    param put %7            // 因为是上下文代码的一部分，以三地址码存放，拆分成了param, call几条指令，但应当视作一个整体
    %8 = call @func 2       // 2代表传入的参数数量，变量值的传入、传出具体过程也对中间代码层不可见

    store %z = %8           // 没写return，但不会导致致命错误，因为以非跳转方式进入返回段，
ret:
    %9 = load %retval
    return %9
}
```

翻译成mips代码

```
func @func int(int %x, int %y) {
                                func:
    %retval = alloca int
    %x = alloca int
    %y = alloca int
    %z = alloca int                 addui   $sp, $sp, -16
    store %retval = %zero           sw      $0, 0($sp)
    $x = param get 0                sw      $a1, 4($sp)
    $y = param get 1                sw      $a2, 8($sp)

    load  %1 = %x                   lw      $t1, 4($sp)
    load  %2 = %y                   lw      $t2, 8($sp)
    %3 = add %1, %2                 addu    $t3, $t1, $t2
    store %z = %3                   sw      $t3, 12($sp)
    load  %4 = %z                   lw      $t4, 12($sp)
    store %retval, %4               sw      $t4, 0($sp)
    goto %ret                       j       func_ret

ret:                            func_ret:
    %5 = load %retval               lw      $t5, 0($sp)
    return %5                       addui   $v0, $t5, 0
                                    jr      $ra
}

func @main int(void) {
                                main:
    %retval = alloca int
    %z = alloca int                 addui   $sp, $sp, -8
    store %retval = %zero           sw      $0, 0($sp)

    %6 = assign 3                   addui   $t0, $0, 3
    %7 = assign 4                   addui   $t1, $0, 3
    param put %6                    addui   $a0, $t0, 0
    param put %7                    addui   $a1, $t1, 0
    %8 = call @func 2               jal     func
    store %z = %8                   addui   $t2, $v0, 0

ret:                            main_ret:
    %9 = load %retval               lw      $t3, 0($sp)
    return %9                       addui   $v0, $t3, 0
                                    jr      $ra
}
```

## 全局变量

中间代码里，对于局部变量、全局变量仅在声明时区分，读写不必作区分（同样用store, load)。

```c
int a;

void func(void) {
    int b;
    a = a + 3;
    b = b - 5;
}
```

中间代码

```
@a = global int

func @func void(void) {
    %b = alloca int

    %1 = load @a
    %2 = assign 3
    %3 = add %1 %2
    @a = store %3

    %4 = load %b
    %5 = assign 5
    %6 = minus %1 %2
    %b = store %6

ret:
    return
}
```

根据load和store的地址变量是局部变量还是全局变量（例如说，可以看是`%`还是`@`），机器代码翻译结果不同.
前者还要转成关于$sp的偏移，后者直接作为一个label去用

```
@a = global int                     .DATA
                                        a: .word 0
                                    .TEXT
func @func void(void) {             func:
    %b = alloca int                     addui   $sp, $sp, -4

    %1 = load @a                        lw      $t1, 0($sp)
    %2 = assign 3                       addi    $t2, $0, 3
    %3 = add %1 %2                      add     $t3, $t1, $2
    @a = store %3                       sw      $t3, a
                                        
    %4 = load %b                        lw      $t4, 0($sp)
    %5 = assign 5                       addi    $t5, $0, 5
    %6 = minus %1 %2                    minus   $t6, $t4, $5
    %b = store %6                       sw      $t6, 0($sp)

ret:
    return                              jr      $ra
}
```

## 关于传递顺序

代码后面出现的`break`和`continue`，可以等归约到代码前面的`while`以后，由它来负责回填。
但是，局部变量的声明和使用是没办法回填的，要是仅仅靠综合属性的传递，那么局部变量的声明信息和使用信息，
仅在两边的代码都生成完毕后才会在父节点上交汇（这可不行）。

要把前面的声明信息传给后面，有几种方式：

1. 添加继承属性动作：优点是采取动作的时机受控，缺点是传递时需要拷贝很多很多次
2. 全局变量（栈式）：优点是效率更高，缺点是有点容易被污染，不太防御式

决定采取全局变量方案，具体而言，参考解释器的处理思路，在全局有一定义域栈(domain stack)，最底层为全局变量表，
用变量名查找变量时从上往下以此查找。由lr1的性质可以推断在前一个定义域的归约结束前不可能开始下一个定义域的归约
（应该没错吧）（lr1还是有点用的嘛），只要我们在定义域开始时压入，定义域结束时及时弹出，就不会出现跨定义域访问
的错误（应该吧）。

在一个定义域内时，保证以下特性: 

1. 局部变量的声明被即时地加入到定义域中（无需等待传递），使得其可被查找到
2. 局部变量的读写访问（按变量名查找）以自顶至底顺序查找，若有多个符合要求，优先使用最近的那个

定义域栈保证以下特性

1. 仅有顶部的定义域可以被修改（添加新声明条目），非顶部的定义域只可读

其实用这种方法能消除所有backpatching的需要(break, continue)，但既然我已经写了那就懒得改了. 

在mini-c里面，这个栈的深度最多不超过三层：全局变量定义域，和函数参数定义域，函数局部变量定义域。

目前的mini-c要求所有声明必须统一出现在定义域的顶部，但因为我们做了中间代码生成，相当于至少是两趟扫描，要支持
即声明即用的语法也并不困难（第一趟扫描仅分配变量标识符，收集声明，第二趟再为定义域确定内存分配）。

更具体而言，在mini-c的文法中，在以下几个位置添加动作:

```
fun_decl
    : type_spec 
      FUNCTION_IDENT
      '('  params  ')'  {   // pidx=9, dot=5
                            func_decl.name         = FUNCTION_IDENT;
                            func_decl.return_decl  = type_spec;
                            func_decl.params_decls = params; 
                            if (global_func_decls.has_not_decl(func_decl)) {
                                global_func_decls.add_decl(func_decl);
                            }
                            func_decl = global_func_decls.ref_decl(func_decl);
                            func_decl.local_decls  = {};
                            domains.push(&func_decl.params_decls); // read only
                            domains.push(&func_decl.local_decls); // can be added
                        }
      compound_stmt     {   // pidx=9, dot=6
                            func_decl.body_code = compound_stmt.code;
                            domains.pop(); // pop local_decls
                            domains.pop(); // pop params_decls
                        }                          
    | type_spec
      FUNCTION_IDENT 
      '(' params ')'    
      ';'               {   // pidx=10, dot=6
                            func_decl.name         = FUNCTION_IDENT;
                            func_decl.return_decl  = type_spec;
                            func_decl.params_decls = params; 
                            domains.top().add_decl(func_decl);
                        }
    ;

compound_stmt
    : '{' local_decls stmt_list '}'
    ; 

local_decls
    : local_decls 
      local_decl        {   // pidx=35, dot=2
                            domains.top().add_decl(local_decl); 
                        }
    | 
    ; 
```

## 汇编指令生成

考虑这样一个函数调用

```c
int func(int x, int y) {
    int z;
    ...
    return z;
}
int main(void) {
    ...
    z = func(3, 4);
    ...
}
```

函数调用处

```
01      %1 = assign 3
02      %2 = assign 4
03      param<0> %1
04      param<1> %2
05      %3 = call func<2>
```

遵守约定，当用 offset($sp) 时，offset始终小于等于0
也就是说先向 offset_1($sp) 写入，确定写入结束后再增长 $sp (减小) 
也就是说约定仅在调用函数才增长栈指针，函数调用结束时立刻退回栈指针

03-05行的指令生成策略如下: 

```
    $sp
when passing params:    
     -0($sp) <- $t1
     -4($sp) <- $t2
when call func
     -8($sp) <- $ra
    -12($sp) <- $fp
        $sp  <- $sp - 16 
    
    jal @func
    
after call func
        $sp  -> $sp + 16 
     -8($sp) -> $ra
    -12($sp) -> $fp

get return val
        $v0 -> $t3
```

函数被调用方: 

```
01  @func:
02      %retval = alloca int
03      %x = alloca int
04      %y = alloca int
05      %z = alloca int
06      %retval = store %0
07      %z = store %0
...
11 @ret:
12     %5 = load %retval
13     return %5
```

关注01到07行的行为

```
allocate space
    (no instructions generated in this procedure)
    ( %retval <=>  -0($sp) )
    (      %x <=>  -4($sp) )
    (      %y <=>  -8($sp) )
    (      %z <=> -12($sp) )
assign initial value
     -0($sp) <- $0
    -12($sp) <- $0

```

关注11到13行的行为

```
ret:
     -0($sp) -> $t4
         $v1 <- $t4 
    jr ra
```

当一个函数开始时, $fp 是这个函数所使用的栈空间的(栈底)，$sp 是这个函数所使用的栈空间的终点(栈顶)
简单来说, 0($fp), -4($fp) 始终是一个当前已经被使用的空间(可能是为函数参数，局部变量预留的内存位置)
而0($sp), -4($sp)始终是一个未被使用过的新的空间. 
$fp 在函数内部不会变，而 $sp可能随着寄存器名字不够、

在函数调用时，约定由函数调用方履行移动栈指针这一职责，即每个函数的开头都假设当前的栈指针 $sp = $fp，
同理，也由函数调用方恢复自己的栈指针现场. 

参数传递始终通过

```c
const int x[2] = {1, 10};

int func(int x, int y) {
    int z;
    z = x + y;
    return z;
}

int main() {
    int z;
    z = func(3, 4);
    return 0;
}
```

```mips
00000000 <func>:
   0:   27bdfff0    addiu   sp,sp,-16
   4:   afbe000c    sw  fp,12(sp)
   8:   03a0f025    move    fp,sp
   c:   afc40010    sw  a0,16(fp)
  10:   afc50014    sw  a1,20(fp)
  14:   8fc30010    lw  v1,16(fp)
  18:   8fc20014    lw  v0,20(fp)
  1c:   00621021    addu    v0,v1,v0
  20:   afc20004    sw  v0,4(fp)
  24:   8fc20004    lw  v0,4(fp)
  28:   03c0e825    move    sp,fp
  2c:   8fbe000c    lw  fp,12(sp)
  30:   27bd0010    addiu   sp,sp,16
  34:   03e00008    jr  ra
  38:   00000000    nop

0000003c <main>:
  3c:   27bdffd8    addiu   sp,sp,-40
  40:   afbf0024    sw  ra,36(sp)
  44:   afbe0020    sw  fp,32(sp)
  48:   03a0f025    move    fp,sp
  4c:   24050004    li  a1,4
  50:   24040003    li  a0,3
  54:   0c000000    jal 0 <func>
  58:   00000000    nop
  5c:   afc2001c    sw  v0,28(fp)
  60:   00001025    move    v0,zero
  64:   03c0e825    move    sp,fp
  68:   8fbf0024    lw  ra,36(sp)
  6c:   8fbe0020    lw  fp,32(sp)
  70:   27bd0028    addiu   sp,sp,40
  74:   03e00008    jr  ra
  78:   00000000    nop
  7c:   00000000    nop

```


## 附加

