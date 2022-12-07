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





## 附加
