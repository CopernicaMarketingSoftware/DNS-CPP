#include <gtest/gtest.h>
#include <dnscpp.h>

using namespace DNS;

// create from string
TEST(Ip, LoopbackV4)
{
    const Ip ip("127.0.0.1");
    EXPECT_TRUE(ip.loopback());
}

// create with inet_addr
TEST(Ip, LoopbackV4FromInetAddr)
{
    in_addr address;
    address.s_addr = inet_addr("127.0.0.1");
    const Ip ip(&address);
    EXPECT_TRUE(ip.loopback());
}

// check v6 loopback address
TEST(Ip, LoopbackV6)
{
    const Ip ip("::1");
    EXPECT_TRUE(ip.loopback());
}
