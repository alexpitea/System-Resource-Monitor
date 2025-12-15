#define _WIN32_WINNT 0x0600 // Needed for GetTickCount64
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <iomanip>
#include <conio.h> // Used for keyboard input

using namespace std;

// --- HELPER FUNCTIONS ---

// I use this to change text color (10 = Green, 12 = Red, 14 = Yellow, 7 = White)
void changeColor(int colorCode) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, colorCode);
}

// Moves the cursor to the top-left so we don't have to clear screen (avoids flickering)
void resetCursorPosition() {
    COORD cursorPosition;
    cursorPosition.X = 0;
    cursorPosition.Y = 0;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursorPosition);
}

// Converts bytes to Megabytes because bytes are too big to read
float convertToMB(unsigned long long bytes) {
    return (float)bytes / 1024 / 1024;
}

// --- MAIN LOGIC ---

void printHeader() {
    changeColor(11); // Cyan color for the title
    cout << "========================================" << endl;
    cout << "       SYSTEM RESOURCE MONITOR          " << endl;
    cout << "========================================" << endl;
    changeColor(7); // Reset to white
}

void printRAM() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    
    // This Windows function fills the struct with data
    GlobalMemoryStatusEx(&memInfo);

    // Calculate the values
    float totalRAM = convertToMB(memInfo.ullTotalPhys);
    float freeRAM = convertToMB(memInfo.ullAvailPhys);
    float usedRAM = totalRAM - freeRAM;
    int percentage = memInfo.dwMemoryLoad;

    changeColor(14); // Yellow for section title
    cout << "\n [ RAM MEMORY ]" << endl;
    changeColor(7);

    cout << " Total: " << (int)totalRAM << " MB" << endl;
    cout << " Used:  " << (int)usedRAM << " MB" << endl;

    // Change color based on how stressed the PC is
    if (percentage < 50) {
        changeColor(10); // Green (Good)
    } else if (percentage < 80) {
        changeColor(14); // Yellow (Warning)
    } else {
        changeColor(12); // Red (Critical)
    }

    cout << " Load:  " << percentage << "%  ";

    // Drawing the progress bar manually
    cout << "[";
    for (int i = 0; i < 20; i++) {
        if (i < (percentage / 5)) {
            cout << (char)254; // This is a block character
        } else {
            cout << " ";
        }
    }
    cout << "]" << endl;
    changeColor(7); // Always reset color at the end
}

void printProcesses() {
    // Taking a snapshot of all running processes
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnap == INVALID_HANDLE_VALUE) {
        return; // Stop if something went wrong
    }

    PROCESSENTRY32 procEntry;
    procEntry.dwSize = sizeof(PROCESSENTRY32);

    // Get the first process
    if (!Process32First(hSnap, &procEntry)) {
        CloseHandle(hSnap);
        return;
    }

    changeColor(14); // Yellow
    cout << "\n [ ACTIVE PROCESSES ]" << endl;
    changeColor(11); // Cyan for columns
    cout << left << setw(25) << " Name" << setw(10) << "PID" << endl;
    changeColor(8); // Gray for the line
    cout << " -----------------------------------" << endl;
    changeColor(7);

    int count = 0;
    // Loop through the processes
    do {
        cout << " " << left << setw(25) << procEntry.szExeFile 
             << setw(10) << procEntry.th32ProcessID << endl;
        
        count++;
    } while (Process32Next(hSnap, &procEntry) && count < 10); // Only show top 10

    CloseHandle(hSnap);
}

void printUptime() {
    unsigned long long milliseconds = GetTickCount64();
    
    // Math to get hours and minutes
    int totalSeconds = milliseconds / 1000;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;

    changeColor(14);
    cout << "\n [ SYSTEM UPTIME ]" << endl;
    changeColor(7);
    cout << " PC has been on for: " << hours << "h " << minutes << "m" << endl;
}

int main() {
    // Set the window title
    SetConsoleTitle("My Process Monitor");

    // Hide the blinking cursor to make it look cleaner
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 100;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);

    system("cls"); // Clear screen once at the start

    cout << "Starting monitor..." << endl;
    Sleep(1000);

    bool isRunning = true;

    while (isRunning) {
        resetCursorPosition(); // Overwrite screen content

        printHeader();
        printUptime();
        printRAM();
        printProcesses();

        changeColor(8); // Gray text for instructions
        cout << "\n-----------------------------------" << endl;
        cout << " Press 'X' to close the program." << endl;
        changeColor(7);

        // Check for keyboard input without pausing everything
        if (_kbhit()) {
            char key = _getch();
            if (key == 'x' || key == 'X') {
                isRunning = false;
            }
        }

        Sleep(1000); // Wait 1 second before refreshing
    }

    system("cls");
    cout << "Program closed. Goodbye!" << endl;
    return 0;
}