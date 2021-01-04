# Memory-Allocater
 This repository will provide implementations for creating malloc, calloc, realloc, and free.

# Compiling and Testing
This program is developed for UNIX based systems.
To compile the program run the command:

```gcc -o memory_allocation.so -fPIC -shared memory_allocation.c```

To test the program run the following command:

```export LD_PRELOAD=$PWD/memory_allocation.so```

This command will load our library before loading a different library making sure that this malloc, calloc, realloc, and/or free are used.

To test this on a program that uses memory allocation you can run a command such as:

```ls```
