/**
 *  Reverse.cpp
 * 
 *  Test program for the DNS::Reverse class
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
 *  Main program
 *  @return int
 */
int main()
{
    // the input variables to test ipv4
    DNS::Ip ip4("192.168.23.17");
    DNS::Reverse reverse4(ip4);
    
    // the input variables to test ipv6
    DNS::Ip ip6("2001:985:45f6:1:d94a:a54e:29d7:5115");
    DNS::Reverse reverse6(ip6);
    
    // output stuff
    std::cout << ip4 << " " << reverse4 << " " << reverse4.ip() << std::endl;
    std::cout << ip6 << " " << reverse6 << " " << reverse6.ip() << std::endl;
    
    // done
    return 0;
}

