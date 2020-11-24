# DNS-CPP

This is a C++ library for asynchronous DNS lookups.


## Set up an event loop integration

DNS-CPP is an asynchronous library that can be integrated in applications
with an event loop. However, the library itself is event-loop agnostic. It is
up to you to write a simple wrapper-class around your own event loop, so
that DNS-CPP can interact with it and that your event loop has an API that
DNS-CPP can work with. This wrapper class should extend from
the abstract class `DNS::Loop`.

```
class MyEventLoop : public DNS::Loop
{
    // @todo add your own implementation for all methods of DNS::Loop

};
```

If you're using libev for your event loop, you're lucky since we have
already provided an example implementation for that: DNS::LibEv.


## Create a DNS::Context

The central object for your DNS queries is the DNS::Context class. 
This is the class that you should construct to start your queries. The
constructor needs one argument: an instance of your event-loop that
extends from the DNS::Loop class.

The context-class does not automatically load the nameserver-configuration,
it does not read /etc/resolv.conf, this is something that you must do 
explicitly.


```
int main()
{
    // our own specific event loop (this could be libevent, libev, libuv,
    // or your own wrapper class around select() or poll()) -- as long
    // as it extends from the DNS::Loop class it's ok
    MyEventLoop eventloop;

    // create a dns context
    DNS::Context context(&eventloop);
    
    // install the nameservers
    context.nameserver(DNS::Ip("8.8.8.8"));
    context.nameserver(DNS::Ip("8.8.4.4"));
    
    // @todo write the rest of your application

    // run the event loop
    eventloop.run();
}
```

## Write your own handler

All methods are asynchronous. This means that there are no blocking operations
and that the result of a query is not immediately returned. Instead you need 
to pass in a handler object. This handler object will be notified when the
DNS lookup is complete (or when it has failed).

```
class MyHandler : public DNS::Handler
{
    // @todo a lot of other code specific for your application
    
    
    // method that is called when the response has been received
    virtual void onReceived(const DNS::Response &response) override
    {
        // @todo analyze the response
        
    }
}
```

## Starting a query

Internally, the DNS-CPP library uses some (but not all) features of the 
libc resolver. For example, it reuses the record-types that are defined
in arpa/nameser.h. To start a query, you can simply call the Context::query()
method:

```
// we need a handler that will process the result
MyHandler handler;

// start the query
context.query("www.example.com", ns_t_a, &handler);
```

## Inspecting the response

Inside your handler you often have to deal with DNS::Response objects.
This is a class that is a wrapper around the libc resolver functions to
parse DNS messages. If you insist, you can retrieve the internal handle
and make calls to the resolver functions. For more info about that, see
https://docstore.mik.ua/orelly/networking_2ndEd/dns/ch15_02.htm.

However, we also built a collection of helper classes to achieve the same.
For example, the DNS::Record class can be used to extract the records
from a response:

```
// our own handler method
virtual void onReceived(const DNS::Response &response) override
{
    // read out all the answers
    for (size_t i = 0; i < response.answers(); ++i)
    {
        // get an answer (DNS::Answer is a derived class from DNS::Record
        // that is optimized for records stored in the Answer section)
        DNS::Answer record(response, i);
        
        // we can already output some data associated with the record
        std::cout << "name: " << record.name() << std::endl;
        std::cout << "ttl: " << record.ttl() << std::endl;
        std::cout << "type: " << record.type() << std::endl;
        
        // each record has a certain type (A, AAAA, MX, CNAME, etc) and
        // uses a different format for storing the associated data, DNS-CPP
        // provides additional class to extract the specific data
        switch (record.type()) {
        case ns_t_a:
            // read out the properties of the A record
            DNS::A a(response, record);
            
            // print out the IP
            std::cout << "ip: " << a.ip() << std::endl;
            break;
        
        case ns_t_aaaa:
            // read out the properties of the AAAA record
            DNS::AAAA aaaa(response, record);
            
            // print out the IP
            std::cout << "ip: " << aaaa.ip() << std::endl;
            break;
    
        case ns_t_mx:
            // read out the properties of the MX record
            DNS::MX mx(response, record);
            
            // print out the priority and hostname
            std::cout << "mx: " << mx.priority() << " " << mx.hostname() << std::endl;
            break;
            
        // @todo add more cases
        
        }
    }
}
```

## More info

We've tried to add as much comments as we could to our code. So feel
free to inspect the header files to learn more.

