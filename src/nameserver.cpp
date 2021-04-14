/**
 *  Nameserver.cpp
 * 
 *  Implementation file of the Nameserver class
 *  
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/nameserver.h"
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/loop.h"
#include "../include/dnscpp/now.h"
#include "../include/dnscpp/watcher.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  core    the core object with the settings and event loop
 *  @param  ip      nameserver IP
 *  @param  udp     udp
 *  @throws std::runtime_error
 */
Nameserver::Nameserver(Core *core, const Ip &ip, Udp *udp) : _core(core), _ip(ip), _udp(udp) {}

/**
 *  Destructor
 */
Nameserver::~Nameserver() {}

/**
 *  Send a datagram to the nameserver
 *  The processor is automatically added to the list of processors, you must
 *  explicitly remove yourself when you're done
 *  @param  processor   the sending object that will be notified of all future responses
 *  @param  query       the query to send
 *  @return Processors* collection of processers that is receiving responses
 */
Processors *Nameserver::datagram(Processor *processor, const Query &query)
{
    // send the message
    return _udp->send(processor, _ip, query, _core->buffersize());
}

/**
 *  End of namespace
 */
}

