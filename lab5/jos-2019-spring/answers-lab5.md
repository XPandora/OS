## Lab 5: File system, Spawn and Shell

- Student ID: 516030910141
- Name: Tianyi Xie

### Question

1. Do you have to do anything else to ensure that this I/O privilege setting is saved and restored properly when you subsequently switch from one environment to another? Why?

- I do not need to anything else to save I/O privilege setting because it is saved as IOPL bits in the EFLAGS register which has already been saved in former labs.

### Challenge

- **Challenge! Implement Unix-style exec.**

- Since it is dangerous to directly modify the memory of the process call the exec command, I use a child process to help me. Like what we have done in spawn, I first create a child process ,use the ELF file to set the initial memory and set its trapframe. Then I use a system call sys_exec to copy the child process data including trapframe and page table to the parent process so that process get the exectued ELF file content by this way. I also write the exechello.c in user directory to test the correctness of my implementation.    