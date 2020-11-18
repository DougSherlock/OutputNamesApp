#include <thread>
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <vector>

using namespace std;

mutex g_mutex;
condition_variable g_condition;
enum PRINT_STATE { PRINT_START, PRINT_FIRST, PRINT_LAST, PRINT_NEXT };
PRINT_STATE g_printState = PRINT_START;
unsigned int g_nameIndex(0);
struct name
{
    struct name(string f, string l) : first(f), last(l) {}
    string first;
    string last;
}; 

vector<name> names = {
    { "Doug", "Sherlock" },
    { "Jane", "Winters" },
    { "Pam", "Howe" },
    { "Doug", "Sherlock" },
    { "Jane", "Winters" },
    { "Pam", "Howe" },
    { "Doug", "Sherlock" },
    { "Jane", "Winters" },
    { "Pam", "Howe" },
};

void printFirst() {
    do {
        unique_lock<mutex> ul(g_mutex);
        g_condition.wait(ul, [] { return g_printState == PRINT_FIRST; });
        if (g_printState == PRINT_FIRST)
        {
            cout << g_nameIndex << " - " << names[g_nameIndex].first;
            g_printState = PRINT_LAST;
            ul.unlock();
            while (g_printState == PRINT_LAST) {
                g_condition.notify_one();
            }
        }
    } while (g_nameIndex < names.size());
}

void printLast() {
    do {
        unique_lock<mutex> ul(g_mutex);
        g_condition.wait(ul, [] { return g_printState == PRINT_LAST; });
        if (g_printState == PRINT_LAST)
        {
            cout << " " << names[g_nameIndex].last << endl;
            g_printState = PRINT_NEXT;
            ul.unlock();
            while (g_printState == PRINT_NEXT) {
                g_condition.notify_one();
            }
        }
    } while (g_nameIndex < names.size());
}

int main()
{
    // init threads
    g_printState = PRINT_START;
    thread pfThread(printFirst);
    thread plThread(printLast);
    while (g_nameIndex < names.size())
    {
        unique_lock<mutex> ul(g_mutex);
        g_printState = PRINT_FIRST;
        ul.unlock();
        while (g_printState == PRINT_FIRST) {
            g_condition.notify_one(); // wake up printFirst thread      
        }

        ul.lock();
        g_condition.wait(ul, [] { return g_printState == PRINT_NEXT; });
        if (g_printState == PRINT_NEXT)
        {
            g_nameIndex++;
        }
        //ul.unlock();
    }

    // wait for threads to finish
    pfThread.join();
    plThread.join();

    cout << "Press Enter key to exit" << endl;
    int n = getchar();
    return 0;
}