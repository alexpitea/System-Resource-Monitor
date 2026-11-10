// Windows System Resource Monitor - console dashboard built on the Windows API.
//
//   monitor          live view, refreshed every second (press X to close)
//   monitor --once   print one snapshot and exit

#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <conio.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {

const size_t LINE_WIDTH = 64;
const size_t BAR_CELLS = 20;
const size_t TOP_PROCESSES = 10;

enum Color { White = 7, Gray = 8, Green = 10, Cyan = 11, Red = 12, Yellow = 14 };

void setColor(int color) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color); }

// Every line is padded to a fixed width. The screen is redrawn from the top
// left corner instead of being cleared, and the padding overwrites whatever
// the previous refresh left behind, so the picture does not flicker.
void printLine(const std::string& text = "", int color = White) {
    std::string line = text.substr(0, LINE_WIDTH);
    line.append(LINE_WIDTH - line.size(), ' ');
    setColor(color);
    std::cout << line << "\n";
    setColor(White);
}

std::string padRight(const std::string& text, size_t width) {
    std::string out = text.substr(0, width);
    out.append(width - out.size(), ' ');
    return out;
}

std::string padLeft(const std::string& text, size_t width) {
    return text.size() >= width ? text : std::string(width - text.size(), ' ') + text;
}

int loadColor(int percent) {
    if (percent < 50) return Green;
    if (percent < 80) return Yellow;
    return Red;
}

std::string loadBar(int percent) {
    const size_t filled = std::min(BAR_CELLS, (size_t)percent * BAR_CELLS / 100);
    return "[" + std::string(filled, '#') + std::string(BAR_CELLS - filled, '.') + "]";
}

unsigned long long megabytes(unsigned long long bytes) { return bytes / (1024ULL * 1024ULL); }

// ------------------------------------------------------------------- CPU ---

unsigned long long toNumber(const FILETIME& time) {
    return ((unsigned long long)time.dwHighDateTime << 32) | time.dwLowDateTime;
}

// Windows reports how long the processors have spent idle and busy since
// boot. The load is the busy share of the time that passed between two
// readings.
class CpuMeter {
public:
    CpuMeter() { read(idle_, total_); }

    int usage() {
        unsigned long long idle = 0, total = 0;
        read(idle, total);
        const unsigned long long idleDelta = idle - idle_;
        const unsigned long long totalDelta = total - total_;
        idle_ = idle;
        total_ = total;
        if (totalDelta == 0 || idleDelta > totalDelta) return 0;
        return (int)((totalDelta - idleDelta) * 100 / totalDelta);
    }

private:
    static void read(unsigned long long& idle, unsigned long long& total) {
        FILETIME idleTime, kernelTime, userTime;
        if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return;
        idle = toNumber(idleTime);
        total = toNumber(kernelTime) + toNumber(userTime);  // kernel time already includes idle time
    }

    unsigned long long idle_ = 0;
    unsigned long long total_ = 0;
};

// ------------------------------------------------------------- processes ---

struct ProcessInfo {
    std::string name;
    DWORD pid = 0;
    unsigned long long memoryBytes = 0;
    bool readable = false;  // protected system processes do not give their numbers
};

std::string toConsoleText(const wchar_t* text) {
    const UINT codePage = GetConsoleOutputCP();
    const int size = WideCharToMultiByte(codePage, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return "";
    std::vector<char> buffer((size_t)size);
    WideCharToMultiByte(codePage, 0, text, -1, buffer.data(), size, nullptr, nullptr);
    return std::string(buffer.data());
}

// Memory in use (working set) of one process. Windows refuses this for
// processes that belong to the system or to another user unless the monitor
// runs as administrator; those are reported as not readable.
bool readWorkingSet(DWORD pid, unsigned long long& bytes) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return false;

    PROCESS_MEMORY_COUNTERS counters;
    const bool ok = GetProcessMemoryInfo(process, &counters, sizeof(counters)) != 0;
    if (ok) bytes = counters.WorkingSetSize;
    CloseHandle(process);
    return ok;
}

std::vector<ProcessInfo> listProcesses() {
    std::vector<ProcessInfo> list;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return list;

    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            ProcessInfo info;
            info.name = toConsoleText(entry.szExeFile);
            info.pid = entry.th32ProcessID;
            info.readable = readWorkingSet(info.pid, info.memoryBytes);
            list.push_back(info);
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);

    std::sort(list.begin(), list.end(),
              [](const ProcessInfo& a, const ProcessInfo& b) { return a.memoryBytes > b.memoryBytes; });
    return list;
}

// ---------------------------------------------------------------- screen ---

void printHeader() {
    printLine("========================================", Cyan);
    printLine("        SYSTEM RESOURCE MONITOR", Cyan);
    printLine("========================================", Cyan);
}

void printSystem(size_t processCount) {
    const unsigned long long seconds = GetTickCount64() / 1000;
    const unsigned long long days = seconds / 86400;
    const unsigned long long hours = (seconds % 86400) / 3600;
    const unsigned long long minutes = (seconds % 3600) / 60;

    printLine();
    printLine(" [ SYSTEM ]", Yellow);
    printLine(" Uptime:    " + std::to_string(days) + "d " + std::to_string(hours) + "h " + std::to_string(minutes) + "m");
    printLine(" Processes: " + std::to_string(processCount));
}

void printCpu(int percent) {
    printLine();
    printLine(" [ CPU ]", Yellow);
    printLine(" Load:  " + padLeft(std::to_string(percent), 3) + "%  " + loadBar(percent), loadColor(percent));
}

void printMemory() {
    MEMORYSTATUSEX memory;
    memory.dwLength = sizeof(memory);
    if (!GlobalMemoryStatusEx(&memory)) return;

    const unsigned long long total = megabytes(memory.ullTotalPhys);
    const unsigned long long used = total - megabytes(memory.ullAvailPhys);
    const int percent = (int)memory.dwMemoryLoad;

    printLine();
    printLine(" [ RAM MEMORY ]", Yellow);
    printLine(" Used:  " + std::to_string(used) + " MB of " + std::to_string(total) + " MB");
    printLine(" Load:  " + padLeft(std::to_string(percent), 3) + "%  " + loadBar(percent), loadColor(percent));
}

void printProcesses(const std::vector<ProcessInfo>& processes) {
    printLine();
    printLine(" [ TOP PROCESSES BY MEMORY ]", Yellow);
    printLine(" " + padRight("Name", 32) + padLeft("PID", 8) + padLeft("Memory", 12), Cyan);
    printLine(" ----------------------------------------------------", Gray);

    const size_t shown = std::min(TOP_PROCESSES, processes.size());
    for (size_t i = 0; i < shown; i++) {
        const ProcessInfo& p = processes[i];
        const std::string memory = p.readable ? std::to_string(megabytes(p.memoryBytes)) + " MB" : "-";
        printLine(" " + padRight(p.name, 32) + padLeft(std::to_string(p.pid), 8) + padLeft(memory, 12));
    }
    for (size_t i = shown; i < TOP_PROCESSES; i++) printLine();

    size_t hidden = 0;
    for (const ProcessInfo& p : processes) {
        if (!p.readable) hidden++;
    }
    if (hidden > 0)
        printLine(" " + std::to_string(hidden) + " system processes not readable (run as administrator)", Gray);
    else
        printLine();
}

void drawScreen(CpuMeter& cpu) {
    const std::vector<ProcessInfo> processes = listProcesses();
    printHeader();
    printSystem(processes.size());
    printCpu(cpu.usage());
    printMemory();
    printProcesses(processes);
}

void moveCursorHome() {
    COORD home = {0, 0};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), home);
}

void showCursor(bool visible) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    if (!GetConsoleCursorInfo(console, &info)) return;
    info.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(console, &info);
}

}  // namespace

int main(int argc, char** argv) {
    const bool once = argc > 1 && std::string(argv[1]) == "--once";
    if (argc > 1 && !once) {
        std::cout << "Usage: monitor [--once]\n";
        return 2;
    }

    CpuMeter cpu;

    if (once) {
        Sleep(500);  // the CPU load needs two readings
        drawScreen(cpu);
        return 0;
    }

    SetConsoleTitleA("System Resource Monitor");
    showCursor(false);
    system("cls");

    for (;;) {
        Sleep(1000);
        moveCursorHome();
        drawScreen(cpu);
        printLine();
        printLine(" Press X to close.", Gray);

        if (_kbhit()) {
            const int key = _getch();
            if (key == 'x' || key == 'X') break;
        }
    }

    showCursor(true);
    system("cls");
    std::cout << "Monitor closed.\n";
    return 0;
}
