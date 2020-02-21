/**
 *  Monitor.h
 *
 *  When the DNS library wants the event loop to monitor a filedescriptor
 *  or set a timer, it passes a monitor object with these calls to user
 *  space (see the Loop class). It is up to the monitor to notify this 
 *  monitor when the filedescriptor indeed becomes active, or when the 
 *  timer expires.
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
class Monitor
{
protected:
    /**
     *  User space should not construct or destruct monitor objects,
     *  hence the constructor and destructor are protected
     */
    Monitor() = default;
    
    /**
     *  Destructor is protected too
     */
    virtual ~Monitor() = default;

public:
    /**
     *  Notify the monitor that the event that the monitor was watching
     *  for (for example activity on a filedescriptor or a timeout) has 
     *  happened or has expired
     */
    virtual void notify() = 0;
};
    
/**
 *  End of namespace
 */
}
