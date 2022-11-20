# 语法制导翻译 (Syntax-Directed Translation, SDT)

参考: 紫龙书第5章, 第6章. 

假设整个语法制导定义(SDD)都是综合属性 (Synthesized attribute)的, 即所有属性推导都是自底至上的, 或者说是对注释语法树进行后序遍历得到的. 

中间代码生成采用四元式表示, 并要求符合静态单赋值形式(SSA). 

## Annotated Parsing Tree 设计

采用如下约定

- 分配新临时变量名: `assign_new_address()`
- 生成三地址码: `gen_code(operator, a1_addr, a2_addr, r_addr)`
- 


### 表达式 (expression)

- 必有属性: `.addr`
- 可有属性: `.val` (表示值已知, 可能是常量表达式)

```txt
E1 <- E2 * E3
	E1.addr = assign_new_address()
	gen_code("*", E2.addr, E3.addr, E1.addr, )
```

以 `a * (4 - b)` 为例: 

```txt
 E
 ├─ E .addr = t1       	0) E.addr <- get_symbol_address("a") = t1;
 ├─ '*'
 └─ E .addr = t4       	3) E.addr <- assign_new_address() = t4; gen_code("t4 <- t2 - t3")
    ├─ E .addr = t2    	1) E.addr <- assign_new_address() = t2; gen_code("t2 <- 4");
    ├─ '-'
    └─ E .addr = t3    	2) E.addr <- assign_new_address() = t3; gen_code("t2 <- 4");
```

以`a[i][j]` 为例 (假设先前已有定义 `int a[4][6];`, `E_i.addr = t1`, `E_j.addr = t2`). 
(关于数组的处理将在后面详细定义)

```txt
 E						2) E.addr <- t6; gen_code("t5 <- t3 + t4"); gen_code("t6 <- a[t5]");
 └─ L					1) gen_code("t4 <- t2 * 4");
    ├─ L 				0) gen_code("t3 <- t1 * 24");
    │  ├─ "a"
    │  ├─ '['
    │  ├─ 2
    │  └─ ']'
    ├─ '['
    ├─ 3
    └─ ']'
```

### 类型表达式 (declarator)

以`int a[2][3]`为例. 
array_table 记录符号名, 数据宽度，类型，形状和对应栈区偏移地址(局部)或数据区偏移地址(全局)

```txt
 D .type = INT, .symbol = "a" .shape = {2, 3}	2) push_array_table(.type, .symbol, .shape)
 ├─ INT
 └─ Decl .symbol = "a" .shape = {2, 3}			1) 
    ├─ Decl .symbol = "a" .shape = {2}			0) 
    │  ├─ "a"
    │  ├─ '['
    │  ├─ E .val = 2
    │  │  └─ 2
    │  └─ ']'
    ├─ '['
    ├─ E .val = 3
    │  └─ 3
    └─ ']'
```

### 控制流

以`if (E) S1 else S2`为例. 

```
 selection_statement
 ├─ IF .
 ├─ '('
 ├─ E
 ├─ ')'
 ├─ S1
 ├─ ELSE
 └─ S2

```


