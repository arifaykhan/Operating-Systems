# Readers-Writers with Replica Load Balancing (C Implementation)

## Overview
This program implements a multi-replica Readers-Writers synchronization problem. 
Key features include:
1. **Writer Priority**: Writers are prioritized to prevent starvation. If a writer is waiting, new readers are blocked.
2. **Load Balancing**: Reader threads are automatically assigned to the file replica with the lowest current traffic.
3. **Synchronized Replicas**: The writer updates all 3 file replicas simultaneously while blocking all reader access.

## Prerequisites
- A Linux or macOS environment (or WSL on Windows).
- GCC compiler installed.

## Compilation Instructions
As per the Lab 1 rules, the program must be linked with the POSIX threads library.
Run the following command in your terminal:
```bash
gcc main.c -o solution -lpthread
