/**
 * \file gpiomanager.cpp
 * \author Thibault Schueller <ryp.sqrt@gmail.com>
 * \brief GPIO device manager class
 */

#include "gpiomanager.hpp"

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
}

#include <thread>

#include "exception/gpioexception.hpp"
#include "igpiolistener.hpp"
#include "tools/unixsyscall.hpp"
#include "tools/log.hpp"
#include <tools/leosac.hpp>

GPIOManager::GPIOManager()
:   _isRunning(false),
    _pollTimeout(DefaultTimeout)
{}

GPIOManager::~GPIOManager()
{
    for (auto gpio : _Gpios)
        delete gpio.second;
}

GPIO* GPIOManager::getGPIO(int idx)
{
    if (_Gpios.count(idx) > 0)
        return (_Gpios.at(idx));
    else
    {
        GPIO*   gpio = instanciateGpio(idx);
        _Gpios[idx] = gpio;
        return (gpio);
    }
}

void GPIOManager::registerListener(IGPIOListener* instance, GPIO* gpio)
{
    ListenerInfo    listener;

    listener.instance = instance;
    listener.gpioNo = gpio->getPinNo();

    std::lock_guard<std::mutex> lg(_listenerMutex);
    _listeners.push_back(listener);
}

void GPIOManager::unregisterListener(IGPIOListener* listener, GPIO* gpio)
{
    std::lock_guard<std::mutex> lg(_listenerMutex);

    for (auto it = _listeners.begin(); it != _listeners.end();)
    {
        if (listener == it->instance && gpio->getPinNo() == it->gpioNo)
            it = _listeners.erase(it);
        else
            ++it;
    }
}

const GPIOManager::GpioAliases& GPIOManager::getGpioAliases() const
{
    return (_gpioAliases);
}

void GPIOManager::setGpioAlias(int gpioNo, const std::string& alias)
{
    _gpioAliases[gpioNo] = alias;
    LOG() << "Gpio " << gpioNo << " alias is now " << alias;
}

void GPIOManager::startPolling()
{
    _isRunning = true;
    _pollThread = std::thread([this] () { pollLoop(); } );
}

void GPIOManager::stopPolling()
{
    _isRunning = false;
    _pollThread.join();
}

void GPIOManager::pollLoop()
{
    char            buffer[PollBufferSize];
    int             ret;
    unsigned int    fdsetSize;

    LOG() << "starting poller";
    buildFdSet();
    fdsetSize = _fdset.size();
    while (_isRunning)
    {
        if ((ret = ::poll(&_fdset[0], fdsetSize, _pollTimeout)) < 0)
        {
            if (errno != EINTR)
                throw (GpioException(UnixSyscall::getErrorString("poll", errno)));
            else
                LOG() << UnixSyscall::getErrorString("poll", errno);
        }
        else if (!ret)
            timeout();
        else
        {
            for (unsigned int i = 0; i < fdsetSize; ++i)
            {
                if (_fdset[i].revents & (POLLPRI | POLLERR ))
                {
                    _fdset[i].revents &= ~(POLLPRI | POLLERR);
                    if ((ret = ::read(_fdset[i].fd, buffer, PollBufferSize - 1)) < 0)
                        throw (GpioException(UnixSyscall::getErrorString("read", errno)));
                    if (::lseek(_fdset[i].fd, 0, SEEK_SET) < 0)
                        throw (GpioException(UnixSyscall::getErrorString("lseek", errno)));
                    for (auto& listener : _listeners)
                    {
                        if (listener.fdIdx == i)
                            listener.instance->notify(listener.gpioNo);
                    }
                }
            }
        }
    }
}

GPIO* GPIOManager::instanciateGpio(int gpioNo)
{
    if (_gpioAliases.count(gpioNo) > 0)
        return (new GPIO(gpioNo, _gpioAliases.at(gpioNo)));
    else
        return (new GPIO(gpioNo, "gpio" + std::to_string(gpioNo)));
}

void GPIOManager::timeout()
{
    for (auto& listener : _listeners)
        listener.instance->timeout();
}

void GPIOManager::buildFdSet()
{
    std::lock_guard<std::mutex> lg(_listenerMutex);
    int                         i = 0;

    _fdset.resize(_listeners.size());
    for (auto& listener : _listeners)
    {
        _fdset[i].fd = _Gpios[listener.gpioNo]->getPollFd();
        _fdset[i].events = POLLPRI | POLLERR;
        _fdset[i].revents = 0;
        listener.fdIdx = i;
        ++i;
    }
}

