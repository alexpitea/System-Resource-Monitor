# Windows System Monitor (C++)

A console-based dashboard that monitors system resources in real-time.

I built this tool because I wanted to understand how the Windows OS handles processes and memory "under the hood." Instead of just using Task Manager, I wrote my own version using the native **Windows API**.

### 🛠 What it does
* **Memory Monitoring:** Shows total RAM, used RAM, and a visual progress bar that changes color based on load (Green/Yellow/Red).
* **Process Viewer:** Lists the active processes and their PIDs using `CreateToolhelp32Snapshot`.
* **Uptime Tracker:** Calculates how long the system has been running.
* **User Friendly:** Updates smoothly without screen flickering.

### 💻 How to Run
You don't need to install anything. Just compile the code with any C++ compiler on Windows.

**Using G++:**
```bash
g++ main.cpp -o monitor
./monitor
Press 'X' anytime to close the monitor safely.

Created as a personal project to learn Windows Internals.
