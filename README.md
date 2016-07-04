# Mila
A simple procedural and imperative language.

### Features ###
Integers (decimal, hexadecimal, octal form), arrays, variables (local, global), constants, input/output, control flow, loops, blocks,
procedures, functions, exit, recursion.

### Syntax ###
Mila aims to be compatible with Pascal syntax. Due to a few extensions however, a program written in Mila is not guaranteed to be compatible with Pascal syntax.
When unsure about syntax of some construct in Mila, searching the same for Pascal will be helpful enough most of the time.

### Example ###
```Bash
$ cat factorial.mila
```
```Pascal
var
  n, f: integer;
begin
  n := 5;
  f := 1;
  while n >= 2 do
  begin
    f := f * n;
    dec(n);
  end;
  writeln(f);
end.
```
```Bash
$ mila factorial.mila # create executable program
$ ./a.out
```
```Bash
120
```
[More sample programs](sfe/tests/program)

### Building from source ###
```Bash
sh build.sh
```
Mila binary can be found in
```Bash
llvm-obj/Debug+Asserts/examples
```

### License ###
MIT
