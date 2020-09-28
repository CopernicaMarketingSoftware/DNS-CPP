/**
 *  Hosts.cpp
 * 
 *  Test-program to parse the /etc/hosts file
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Dependencies
 */
#include <dnscpp.h>
#include <iostream>

/**
 *  Main procedure
 *  @return int
 */
int main()
{
    // parse the hosts file
    DNS::Hosts hosts("/etc/hosts");
    
    // lookup an up
    auto *ip = hosts.lookup("localhost");
    
    // do we have a match?
    if (ip == nullptr) std::cout << "no match" << std::endl;
    
    // otherwise we report
    else std::cout << "foud " << *ip << std::endl;
    
    // done
    return 0;
}

