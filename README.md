# The Solid Programming Language

This repository contains `solid`, a programming language based on LLVM. 
The language supports:
- `double` type
- Functions (`func`)
- Built-in, native functions (`native`, in particular `print` and `printc`)
- Control flow (`when` and `while`)
- Operators (`+,-,*,<`)
- User-defined unary and binary operators (`operator`)
- Mutable, local variables (`let`)
- Compilation to object files

It is based on the "Kaleidoscope" language and the related documentation [here](https://llvm.org/docs/tutorial/index.html).

The `./examples` folder contains a few programs written in `solid`.

## Usage

To use this project on your machine, you need to set up LLVM and adjust the `CMakeLists.txt` based on your LLVM installation.

Once you've built the project, you can get started using: `./solid_lang`
```
ready> 2+3;
Evaluated to 5.000000
ready> func a() 3;
4 + a();
Evaluated to 7.000000
ready>
```

Use `./solid_lang --help` to see all compiler options:
```
OVERVIEW: The Solid Programming Language
USAGE: solid_lang [options] <input filename>

OPTIONS:

Compiler options:

--IR          - Print generated LLVM IR
-o <filename> - Output filename

...
```

### LLVM IR

To get the generated LLVM IR, use: `./solid_lang --IR`
```
ready> 2+3;

Generated LLVM IR:
define double @__anonymous_top_level_expr() {
entry:
  ret double 5.000000e+00
}

Evaluated to 5.000000
ready> func a() 3;

Generated LLVM IR:
define double @a() {
entry:
  ret double 3.000000e+00
}

4 + a();

Generated LLVM IR:
define double @__anonymous_top_level_expr() {
entry:
  %calltmp = call double @a()
  %addtmp = fadd double 4.000000e+00, %calltmp
  ret double %addtmp
}

Evaluated to 7.000000
ready> 
```

### Object files

To create an object file, put your program into a file (like `Average.solid`) and use: 
```
./solid_lang ../examples/Average.solid 
created ../examples/Average.o
```

To use this object file, use your program in `C++` (like in `link.cpp`) and run:
```
clang++ ../examples/link.cpp ../examples/Average.o -o main

./main
avg of 3 and 4: 3.5
```