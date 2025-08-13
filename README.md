# Educational RAT Framework for Linux

**Disclaimer**: This project is for educational purposes only. It is designed to demonstrate cybersecurity concepts related to Remote Access Trojans (RATs) and malware behavior. Do not use this software for malicious purposes.

## Overview

This is an educational Remote Access Trojan (RAT) framework developed for Linux systems to demonstrate how malware can operate while evading detection. The project focuses on maintaining a low profile by implementing only essential functionalities that could blend with normal application behavior.

## Key Features

- **Modular Design**: Core functionality is kept minimal, with additional capabilities loaded via shared objects (.so files)
- **Stealth Techniques**:
  - No direct system calls (like exec())
  - Only implements common behaviors (network communication, file operations)
  - Capability to load and execute code dynamically
- **C2 Communication**: Uses socket-based command and control
- **File Operations**: Can create and modify files
- **Plugin System**: Supports loading external shared libraries for extended functionality

## Technical Details

- Written in C++11
- Uses POSIX sockets for communication
- Implements dynamic library loading via `dlopen`/`dlsym`
- File operations using standard file I/O
- Multi-threaded architecture for concurrent operations
- Memory-only library loading capability (via `memfd_create`)

## Educational Purpose

This project demonstrates:
- How malware can use legitimate system functions maliciously
- Techniques for evading detection
- Common RAT capabilities
- The importance of monitoring even "normal" application behaviors
- Dynamic code loading mechanisms

## Usage Warning

⚠️ **This is not a penetration testing tool** ⚠️  
This software is purely for educational demonstration of malware techniques. Running this on systems without explicit permission may violate laws and ethical guidelines.

## Building

```bash
g++ -std=c++11 -ldl -pthread main.cpp -o educational_rat