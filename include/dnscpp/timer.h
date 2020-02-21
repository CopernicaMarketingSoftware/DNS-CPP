/**
 *  Timer.h
 *
 *  When the DNS library wants the event loop to set a timer, it passes 
 *  a timer object to user space (see the Loop class). It is up to the 
 *  user space code to call the expire() method when the timer expires.
 *
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Begin of namespace
 */
namespace DNS {
 
/**
 *  Class definition
 */
class Timer
{
protected:
    /**
     *  User space should not construct or destruct timer objects,
     *  hence the constructor and destructor are protected
     */
    Timer() = default;
    
    /**
     *  Destructor is protected too
     */
    virtual ~Timer() = default;

public:
    /**
     *  Notify the timer that it expired
     */
    virtual void expire() = 0;
};
    
/**
 *  End of namespace
 */
}
