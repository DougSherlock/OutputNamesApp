#include <thread>
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <vector>

using namespace std;


mutex mutexLog;

void Log(const char* func, const char* msg)
{
    unique_lock<mutex> ul(mutexLog);
    cout << func << " : " << msg << endl;
}

enum PRINT_STATE { PRINT_START, PRINT_FIRST, PRINT_LAST, PRINT_NEXT, PRINT_DONE };
enum THREAD_STATE { THREAD_ACTIVE, THREAD_STOP_REQ, THREAD_STOP_ACK, THREAD_STOPPED };

class PrintThreadData
{
public:
    PrintThreadData() 
        : printState(PRINT_START)
        , printFirstThreadState(THREAD_ACTIVE) 
        , printLastThreadState(THREAD_ACTIVE)
        , nameIndex(0) {}
    mutex mutex;
    condition_variable condition;
    PRINT_STATE printState;
    THREAD_STATE printFirstThreadState;
    THREAD_STATE printLastThreadState;
    unsigned int nameIndex;
};

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
    { "Bob", "Smith" },
    { "Wayne", "Gretzky" },
    { "Ivana", "Tinkle" },
    { "Seymour", "Butts" },
    { "Jeremy", "Beremy" },
    { "Lotta", "Fagina" },
};

void PrintFirstName(PrintThreadData& data)
{
    Log(__FUNCTION__, "started");
    do {
        unique_lock<mutex> ul(data.mutex);
        data.condition.wait(ul, [&data] { return data.printState == PRINT_FIRST || data.printFirstThreadState == THREAD_STOP_REQ; });
        if (data.printState == PRINT_FIRST)
        {
            cout << data.nameIndex << " - " << names[data.nameIndex].first;
            data.printState = PRINT_LAST;
            ul.unlock();
            while (data.printState == PRINT_LAST) {
                data.condition.notify_one();
            }
        }
        if (data.printFirstThreadState == THREAD_STOP_REQ)
        {
            Log(__FUNCTION__, "stopping");
            data.printFirstThreadState = THREAD_STOP_ACK;
            ul.unlock();
            break;
        }
    } while (true);
    Log(__FUNCTION__, "done");
}

void PrintLastName(PrintThreadData& data) {
    Log(__FUNCTION__, "started");
    do {
        unique_lock<mutex> ul(data.mutex);
        data.condition.wait(ul, [&data] { return data.printState == PRINT_LAST || data.printLastThreadState == THREAD_STOP_REQ; });
        if (data.printState == PRINT_LAST)
        {
            cout << " " << names[data.nameIndex].last << endl;
            data.printState = PRINT_NEXT;
            ul.unlock();
            while (data.printState == PRINT_NEXT) {
                data.condition.notify_one();
            }
        }
        if (data.printLastThreadState == THREAD_STOP_REQ)
        {
            Log(__FUNCTION__, "stopping");
            data.printLastThreadState = THREAD_STOP_ACK;
            ul.unlock();
            break;
        }
    } while (true);

    Log(__FUNCTION__, "done");
}

int main()
{
    Log(__FUNCTION__, "started");
    PrintThreadData data;
    data.printState = PRINT_START;
    Log(__FUNCTION__, "creating threads");
    thread pfThread(PrintFirstName, ref(data));
    thread plThread(PrintLastName, ref(data));
    Log(__FUNCTION__, "entering names loop");
    while (data.nameIndex < names.size())
    {
        unique_lock<mutex> ul(data.mutex);
        data.printState = PRINT_FIRST;
        ul.unlock();
        while (data.printState == PRINT_FIRST) {
            data.condition.notify_one(); // wake up PrintFirstName thread      
        }

        ul.lock();
        data.condition.wait(ul, [&data] { return data.printState == PRINT_NEXT; });
        if (data.printState == PRINT_NEXT)
        {
            data.nameIndex++;
        }
        //ul.unlock();
    }

    Log(__FUNCTION__, "names loop done");
    {
        unique_lock<mutex> ul(data.mutex);
        data.printState = PRINT_DONE;
    }

    Log(__FUNCTION__, "stopping last name thread");
    {
        unique_lock<mutex> ul(data.mutex);
        data.printLastThreadState = THREAD_STOP_REQ;
        ul.unlock();
        Log(__FUNCTION__, "waiting for last name thread to stop");
        while (data.printLastThreadState == THREAD_STOP_REQ) {
            data.condition.notify_one(); // wake up PrintFirstName thread      
        }

        Log(__FUNCTION__, "last name thread has stopped");
    }

    Log(__FUNCTION__, "stopping first name thread");
    {
        unique_lock<mutex> ul(data.mutex);
        data.printFirstThreadState = THREAD_STOP_REQ;
        ul.unlock();
        while (data.printFirstThreadState == THREAD_STOP_REQ) {
            data.condition.notify_one(); // wake up PrintFirstName thread      
        }
    }


    // wait for threads to finish
    Log(__FUNCTION__, "waiting for all threads to terminate");

    pfThread.join();
    plThread.join();

    Log(__FUNCTION__, "all threads have terminated");
    Log(__FUNCTION__, "Press Enter key to exit");
    int n = getchar();
    return 0;
}