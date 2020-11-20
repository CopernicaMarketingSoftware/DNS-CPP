/**
 *  Idle.h
 *
 *  When the DNS library wants the event loop to set a idle watcher, it passes 
 *  a idle object to user space (see the Loop class). It is up to the 
 *  user space code to call the idle() method when the idle watcher is activated.
 *
 *  @author Michael van der Werve <michael.vanderwerve@mailerq.com>
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
class Idle
{
protected:
    /**
     *  User space should not construct or destruct idle objects,
     *  hence the constructor and destructor are protected
     */
    Idle() = default;
    
    /**
     *  Destructor is protected too
     */
    virtual ~Idle() = default;

public:
    /**
     *  Notify the idle that we're idle
     */
    virtual void idle() = 0;
};
    
/**
 *  End of namespace
 */
}
