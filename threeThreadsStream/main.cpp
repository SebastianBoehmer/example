/**
 * Under the license of LGPL V3.0
 * http://www.gnu.org/licenses/lgpl-3.0.de.html
 * Copyright © 2015 Free Software Foundation, Inc. <http://fsf.org/>
 * Everyone is permitted to copy and distribute verbatim copies or changing
 * of this document is allowed.
 * Changed documents must contain this license notice.
 * You can use another license for your own changes.
 */
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <iostream>
#include <future>       // for std::async

#include "BoundedBuffer.h"

//  Every-where you find XXX you have to change something

/**
 * some specifications
 * Every-where you find XXX you have to change something
 */
namespace ttt {
    // XXX check the parameters:
    // the data type of the buffers. E.g. pointer on image frame. Here an integer.
    typedef int dataType;
    // capacity of all two buffers.
    static const int capacity = 100;
    // here only for simulation of input elements. This is the count.
    static const int inputMax = 10000;
    // delay in the three threads.
    static const std::chrono::milliseconds delay {8};
    // used to indicate the end of inputThread.
    static std::atomic_bool isFinished;
}


using namespace std;

// input thread
template<typename T>
void threadInputFnc(BoundedBuffer<T> &buffer, std::chrono::milliseconds delayMs)
{
    for (T i=0; i < ttt::inputMax; ++i)
    {
        // XXX here you have to grab the frame and move it to the buffer.
        buffer << std::move(i);
        std::this_thread::sleep_for(delayMs);
    }
    ttt::isFinished.store(true);
}

// working thread
template<typename T>
void threadWorkingFnc(BoundedBuffer<T> &outBuffer, BoundedBuffer<T> &inBuffer, std::chrono::milliseconds delayMs)
{
    T x;
    while (!ttt::isFinished.load())
    {
        inBuffer >> x;
        std::this_thread::sleep_for(delayMs);
        // XXX here you make your calculations on the frame. Here just multiple of 2.
        outBuffer << x*2;
    }
}

// output thread
template<typename T>
void threadOutputFnc(BoundedBuffer<T> &buffer, std::chrono::milliseconds delayMs)
{
    while (!ttt::isFinished.load())
    {
        // XXX the output can be designed in other ways.
        // you can clean the frame pointer here. Also release the memory, if you allocated above.
        std::cout << '\r' << buffer;
        std::cout.flush();
        std::this_thread::sleep_for(delayMs);
    }
}


int main()
{
    cout << "starting ..." << endl;

    BoundedBuffer<ttt::dataType> inputToWrokingBuffer(ttt::capacity);
    BoundedBuffer<ttt::dataType> workingToOutputBuffer(ttt::capacity);

    auto inputThread = std::async(std::launch::async, threadInputFnc<ttt::dataType>
                                  , std::ref(inputToWrokingBuffer)      // ref, because & ref in thread fnc.
                                  , ttt::delay);        // delay can be different values

    auto workingThread = std::async(std::launch::async, threadWorkingFnc<ttt::dataType>
                                    , std::ref(workingToOutputBuffer)
                                    , std::ref(inputToWrokingBuffer)
                                    , ttt::delay);

    auto outputThread = std::async(std::launch::async, threadOutputFnc<ttt::dataType>
                                   , std::ref(workingToOutputBuffer)
                                   , ttt::delay);

    // wait on threads finished.
    outputThread.get();
    workingThread.get();
    inputThread.get();

    cout << "Finished." << endl;

    return 0;
}

