/**
 *  Core.cpp
 * 
 *  Implementation file for the Core class
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 - 2021 Copernica BV
 */

/**
 *  Dependencies
 */
#include "../include/dnscpp/core.h"
#include "../include/dnscpp/lookup.h"
#include "../include/dnscpp/loop.h"
#include "../include/dnscpp/watcher.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Constructor
 *  @param  loop        your event loop
 *  @param  defaults    should defaults from resolv.conf and /etc/hosts be loaded?
 *  @param  buffersize  send & receive buffer size of each UDP socket
 *  @throws std::runtime_error
 */
Core::Core(Loop *loop, bool defaults) :
    _loop(loop),
    _ipv4(loop, this),
    _ipv6(loop, this)
{
    // do nothing if we don't need the defaults
    if (!defaults) return;
    
    // load the defaults from /etc/resolv.conf
    ResolvConf settings;
    
    // copy the nameservers
    for (size_t i = 0; i < settings.nameservers(); ++i) _nameservers.emplace_back(settings.nameserver(i));
    
    // take over some of the settings
    _timeout = settings.timeout();
    _interval = settings.timeout();
    _attempts = settings.attempts();
    _rotate = settings.rotate();

    // we also have to load /etc/hosts
    if (!_hosts.load()) throw std::runtime_error("failed to load /etc/hosts");
}

/**
 *  Protected constructor, only the derived class may construct it
 *  @param  loop        your event loop
 *  @param  settings    settings from the resolv.conf file
 *  @param  buffersize  send & receive buffer size of each UDP socket
 */
Core::Core(Loop *loop, const ResolvConf &settings) :
    _loop(loop),
    _ipv4(loop, this),
    _ipv6(loop, this)
{
    // construct the nameservers
    for (size_t i = 0; i < settings.nameservers(); ++i) _nameservers.emplace_back(settings.nameserver(i));

    // take over some of the settings
    _timeout = settings.timeout();
    _interval = settings.timeout();
    _attempts = settings.attempts();
    _rotate = settings.rotate();
}


/**
 *  Destructor
 */
Core::~Core()
{
    // stop timer (in case it is still running)
    if (_timer == nullptr) return;
    
    // stop the timer
    _loop->cancel(_timer, this);
}

/**
 *  Add a new lookup to the list
 *  @param  lookup
 *  @return Operation
 */
Operation *Core::add(std::shared_ptr<Lookup> lookup)
{
    // we are lazy
    _scheduled.emplace_back(lookup);

    // if we already have a timer the expires immediately
    if (_timer && _immediate) return lookup.get();

    // stop existing timer
    if (_timer) _loop->cancel(_timer, this);

    // reschedule the timer
    _timer = _loop->timer(0.0, this);

    // this is an immediate-timer
    _immediate = true;
    
    // expose the operation
    return lookup.get();
}

/**
 *  Calculate the delay until the next job
 *  @return double      the delay in seconds (or < 0 if there is no need to run a timer)
 */
double Core::delay(double now)
{
    // if there is an unprocessed inbound queue, we have to expire asap
    if (_ipv4.buffered() || _ipv6.buffered()) return 0.0;
    
    // if there is nothing scheduled
    if (_lookups.empty() && _ready.empty()) return -1.0;
    
    // if only one is set
    if (_lookups.empty()) return _ready.front()->delay(now);
    if (_ready.empty()) return _lookups.front()->delay(now);
    
    // get the minimum
    return std::min(_lookups.front()->delay(now), _ready.front()->delay(now));
}

/**
 *  Reschedule the timer
 *  @param  now         current time
 */
void Core::reschedule(double now)
{
    // calculate the delay
    auto seconds = delay(now);
    
    // if timer was not set and will not be set
    if (seconds < 0.0 && _timer == nullptr) return;
    
    // if timer was immediate and stays immediate, not changes are needed
    if (seconds == 0.0 && _timer != nullptr && _immediate) return;

    // if the timer is already running we have to reset it
    if (_timer != nullptr) _loop->cancel(_timer, this);
    
    // check when the next operation should run
    _timer = seconds < 0 ? nullptr : _loop->timer(seconds, this);
    _immediate = seconds == 0.0;
}

/**
 *  An inflight lookup is done, or it was cancelled from user-space
 *  @param      lookup  The lookup
 */
void Core::done(std::shared_ptr<Lookup> lookup)
{
    // remove this potentially inflight lookup, potentially somewhere in the middle of this queue
    _lookups.pop(*lookup);

    // prepare to finalize the lookup
    if (finalize(Watcher(this), move(lookup))) return;

    // if we now have more room to execute another lookup, do so immediately
    if (_lookups.size() < _capacity) proceed(Now());
}

/**
 *  Method that is called when a UDP socket has a buffer that it wants to deliver
 *  @param  sockets     the sockets with a buffer
 */
void Core::onBuffered(Sockets *sockets)
{
    // if we already had an immediate timer we do not have to set it
    if (_timer != nullptr && _immediate) return;

    // if the timer is already running we have to reset it
    if (_timer != nullptr) _loop->cancel(_timer, this);

    // check when the next operation should run
    _timer = _loop->timer(0.0, this);
    _immediate = true;
}

/**
 *  Proceed with more operations
 *  @param  now
 */
void Core::proceed(double now)
{
    // enqueue more awaiting lookups
    while (_lookups.size() < _capacity && !_scheduled.empty())
    {
        // take it from the front of the queue
        std::shared_ptr<Lookup> lookup = std::move(_scheduled.front());

        // we're going to move this lookup somewhere in all possible cases
        _scheduled.pop_front();

        // if it's completely finished, we can forget about it (maybe it was prematurely cancelled)
        if (lookup->finished()) continue;

        // if it has exhausted its attempts, we place it in the ready queue to be finalized on the next iteration
        if (lookup->exhausted()) _ready.emplace_back(std::move(lookup));

        // otherwise try to run it. If it succeeds, it is now inflight
        else if (lookup->execute(now)) _lookups.push(lookup);

        // otherwise we put it at the back of the awaiter queue, to be retried later
        else _scheduled.push_back(std::move(lookup));
    }
}

/**
 *  Method that is called when the timer expires
 *  This is the "main" function of DNS-CPP
 */
void Core::expire()
{
    // forget the timer
    _loop->cancel(_timer, this); _timer = nullptr;
    
    // a call to userspace might destruct `this`
    Watcher watcher(this);
    
    // get the current time
    Now now;

    // first we check the udp sockets to see if they have data available
    // this moves lookups from _lookups to _ready
    _ipv4.deliver(std::numeric_limits<size_t>::max());
    _ipv6.deliver(std::numeric_limits<size_t>::max());

    // invoke ready lookups
    for (auto &lookup : _ready) if (finalize(watcher, move(lookup))) return;

    // all callback handlers have been invoked
    _ready.clear();

    // remove timed-out lookups at the front of the _lookups queue
    while (!_lookups.empty() && _lookups.front()->expired(now)) _scheduled.emplace_back(_lookups.pop());

    // start new lookups
    proceed(now);

    // reset the timer
    reschedule(now);
}

/**
 *  Invoke callback handlers of a lookup that's done.
 *  @param  watcher  The watcher to check if we're still valid
 *  @param  lookup   The lookup
 *  @return true if we should bail out immediately
 */
bool Core::finalize(const Watcher &watcher, std::shared_ptr<Lookup> &&lookup) const noexcept
{
    // invoke callback handler
    try
    {
        lookup->finalize();
    }
    catch (...)
    {
        // @todo: report/log this somehow?
    }
    // user-space might have destroyed us
    return !watcher.valid();
}

/**
 *  Send a message over a UDP socket
 *  @param  ip              target IP
 *  @param  query           the query to send
 *  @return Inbound         the object that receives the answer
 */
Inbound *Core::datagram(const Ip &ip, const Query &query)
{
    // check the version number of ip
    switch (ip.version()) {
    case 4:     return _ipv4.datagram(ip, query);
    case 6:     return _ipv6.datagram(ip, query);
    default:    return nullptr;
    }
}

/**
 *  Connect with TCP to a socket
 *  This is an async operation, the connection will later be passed to the connector
 *  @param  ip          IP address of the target nameservers
 *  @param  connector   the object interested in the connection
 *  @return bool
 */
bool Core::connect(const Ip &ip, Connector *connector)
{
    // check the version number of ip
    switch (ip.version()) {
    case 4:     return _ipv4.connect(ip, connector);
    case 6:     return _ipv6.connect(ip, connector);
    default:    return false;
    }
}    

/**
 *  End of namespace
 */
}

