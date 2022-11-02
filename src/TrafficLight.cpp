#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <chrono>

using namespace std::chrono_literals;

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> uLock(_mtx);
    _cond.wait(uLock, [this]
               { return !_queue.empty(); }); // pass unique lock to condition variable

    T msg = std::move(_queue.back());
    _queue.pop_back();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> uLock(_mtx);
    _queue.push_back(std::move(msg));
    _cond.notify_one(); // notify client after pushing new message into queue
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.

    while (true)
    {
        TrafficLightPhase phase = _messageQueue.receive();
        if (phase == green)
        {
            break;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    std::shared_ptr<MessageQueue<TrafficLightPhase>> _messageQueue(new MessageQueue<TrafficLightPhase>);
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    std::random_device rd;                               // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd());                              // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(4000, 6000); // 4 to 6 seconds in milliseconds

    int trafficLightChangeTime = distrib(gen);
    clock_t startTime = clock();
    while (true)
    {
        // By default, wait 1 ms each cycle
        std::this_thread::sleep_for(1ms);

        // Time for the traffic light to change
        int interval = (clock() - startTime) * 1000 / CLOCKS_PER_SEC;
        if (interval >= trafficLightChangeTime)
        {
            // Change from red to green or vice versa
            switch (_currentPhase)
            {
            case green:
                _currentPhase = red;
                break;
            default:
                _currentPhase = green;
                break;
            }

            // Send update to the message queue
            TrafficLightPhase msg = _currentPhase;
            _messageQueue.send(std::move(msg));

            // Generate a new 4-6 second period until the next traffic light change and reset the timer
            trafficLightChangeTime = distrib(gen);
            startTime = clock();
        }
    }
}
