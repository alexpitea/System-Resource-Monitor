# Windows System Resource Monitor (C++)

A console dashboard that shows what the computer is doing in real time, written directly on the Windows API.

I built it to understand how Windows exposes processes, memory and processor time, instead of only looking at Task Manager.

## What it shows

- **CPU load**, measured from the idle and busy time the system reports between two refreshes
- **Memory**: used and total RAM with a coloured load bar (green, yellow, red)
- **Top 10 processes by memory**, with name, PID and working set
- **Uptime** and the number of running processes
- A live view that refreshes every second without flickering, and a `--once` mode that prints a single snapshot

```text
========================================
        SYSTEM RESOURCE MONITOR
========================================

 [ SYSTEM ]
 Uptime:    6d 22h 56m
 Processes: 344

 [ CPU ]
 Load:   10%  [##..................]

 [ RAM MEMORY ]
 Used:  12879 MB of 15676 MB
 Load:   82%  [################....]

 [ TOP PROCESSES BY MEMORY ]
 Name                                 PID      Memory
 ----------------------------------------------------
 chrome.exe                         39692      619 MB
 explorer.exe                       12596      318 MB
 ...
```

## Build

Requirements: Windows and a C++17 compiler.

```bat
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o monitor.exe -lpsapi
```

## Usage

```text
monitor          live view, press X to close
monitor --once   print one snapshot and exit (useful in scripts)
```

## How it works

| Information | Windows API |
|---|---|
| CPU load | `GetSystemTimes`, difference between two readings |
| Memory | `GlobalMemoryStatusEx` |
| Process list | `CreateToolhelp32Snapshot`, `Process32FirstW`, `Process32NextW` |
| Memory per process | `OpenProcess` with limited query rights, `GetProcessMemoryInfo` |
| Uptime | `GetTickCount64` |
| No flicker | the cursor is moved to the top left corner and every line is padded, instead of clearing the screen |

## Limits

- Windows does not let a normal user read the memory of system processes or of processes that belong to another user. They are counted on the last line and left out of the top 10. Run the monitor as administrator to include them.
- CPU load is for the whole machine, not per process.
- Windows only.
