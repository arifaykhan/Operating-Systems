# Readers-writers with replica load balancing in C

## Overview
This program implements a multi-replica readers-writers synchronization problem. 
Key features include:
1. **Writer priority**: writers are prioritized to prevent starvation. If a writer is waiting, new readers are blocked;
2. **Load balancing**: reader threads are automatically assigned to the file replica with the lowest current traffic;
3. **Synchronized replicas**: the writer updates all 3 file replicas simultaneously while blocking all reader access.

## Prerequisites
- A Linux or macOS environment (or WSL on Windows).
- GCC compiler installed.

## Compilation Instructions
The program must be linked with the POSIX threads library.
Run the following command in your terminal:
```bash
gcc main.c -o solution -lpthread
