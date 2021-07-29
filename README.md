# lib.chron

Schedulers, timers, and clock-based eventing for the C programming language

## Library Features

- Simple, fully programmable interfaces for
  - single-use aka 'one shot' timers
  - periodic timers
  - exponential timers
- rescheduling
- cancellation
- consistent exception-handling
- Hierarchical Wheel Timers

## Dynamic Linking

Linking to `lib.chron`:

```bash
# 1) include and use lib.chron in your project
# 2) generate object file for your project
gcc -I ../path/to/libchron -c main.c -o main.o
# 3) generate shared object file
make
# 4) link your project to lib.chron
gcc -o main main.o -L../path/to/libchron -llibchron
# you may need to add the lib location to your PATH
```

Linking to `lib.chron` on Windows:

```bash
# 1) include and use lib.chron in your project
# 2) generate object file for your project
gcc -I ../path/to/libchron -c main.c -o main.o
# 3) generate shared object file
make win
# 3) link your project to lib.chron
gcc -o main.exe main.o -L /path/to/lib.chron -llib_chron.dll
# you may need to add the lib location to your PATH
```
