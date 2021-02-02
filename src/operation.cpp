/**
 *  Operation.cpp
 * 
 *  Implementation file for the Operation class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/handler.h"
#include "../include/dnscpp/operation.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Cancel the operation
 */
void Operation::cancel()
{
    // if already reported back to user-space
    if (_handler == nullptr) return;
    
    // remember the handler
    auto *handler = _handler;
    
    // get rid of the handler to avoid that the result is reported
    _handler = nullptr;
    
    // the last instruction is to report it back to user-space
    handler->onCancelled(this);
}
    
/**
 *  End of namespace
 */
}
