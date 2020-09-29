/**
 *  Request.h
 * 
 *  Class to parse the request that was sent to the nameserver. You can
 *  use this class if you want to find out exactly what data was sent
 *  to the nameserver.
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
class Request : public Message
{
public:
    /**
     *  Constructor
     *  @param  query       the original query
     *  @throws std::runtime_error
     */
    Request(const Query &query) : Message(query.data(), query.size()) {}
    
    /**
     *  Constructor
     *  @param  operation   the running operation
     */
    Request(const Operation *operation) : Request(operation->query()) {}
    
    /**
     *  Destructor
     */
    virtual ~Request() = default;
};
    
/**
 *  End of namespace
 */
}
