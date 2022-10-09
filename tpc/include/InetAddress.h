#ifndef TPC__INETADDRESS_H
#define TPC__INETADDRESS_H

#include "Common.h"

#include <vector>
#include <string>

#include <netinet/in.h>
#include <sys/socket.h>

class InetAddress {
    public:
        // invalid address
        InetAddress() { addr_.sin_family = AF_UNSPEC; }
        // for connecting
        InetAddress(StringArg ip, uint16_t port);
        //for listening
        explicit InetAddress(uint16_t port, bool ipv6 = false);

        // interface with Sockets API
        explicit InetAddress(const struct sockaddr& saddr);

        // default copy / assignment are Okey

        sa_family_t family() const { return addr_.sin_family; }
        uint16_t port() const { return ntohs(addr_.sin_port); }
        void setPort(uint16_t port) { addr_.sin_port = htons(port); }

        std::string toIp() const;
        std::string toIpPort() const;

        // Interface with Sockets API
        const struct sockaddr* getSockAddr() const {
            return reinterpret_cast<const struct sockaddr*>(&addr6_);
        }

        socklen_t length() const {
            return family() == AF_INET6 ? sizeof(addr6_) : sizeof(addr_);
        }

        bool operator==(const InetAddress& rhs) const;

        // Resolves hostname to IP address;
        // Returns true on success
        // Thread safe
        static bool resolve(StringArg hostname, uint16_t port, InetAddress*);
        static std::vector<InetAddress> resolveAll(StringArg hostname, uint16_t port = 0);
        
    private:
        union {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
};


#endif