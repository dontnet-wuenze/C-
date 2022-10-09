#include "Acceptor.h"
#include "InetAddress.h"
#include "TcpStream.h"

#include <cstdio>
#include <iostream>

#include <boost/program_options.hpp>

#include <memory>
#include <sys/time.h>

struct Options {
    uint16_t port;
    int length;
    int number;
    bool transmit, receive, nodelay;
    std::string host;
    Options()
        :   port(0), length(0), number(0),
            transmit(false), receive(false), nodelay(false)
            {}
};

struct SessionMessage {
    int32_t number;
    int32_t length;
} __attribute__((__packed__));

struct PayloadMessage {
    int32_t length;
    char data[0];
};

double now() {
    struct timeval tv = {0 ,0};
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void transmit(Options option) {
    InetAddress addr;
    if(!InetAddress::resolve(option.host, option.port, &addr)) {
        printf("Unable to resolve %s\n", option.host.c_str());
        return ;
    }

    printf("Connecting to %s\n", addr.toIpPort().c_str());
    TcpStreamPtr stream(TcpStream::connect(addr));
    if(!stream) {
        printf("Unable to connect %s\n", addr.toIpPort().c_str());
        perror("");
        return;       
    }

    if(option.nodelay)
        stream->setTcpNoDelay(true);

    printf("connected\n");

    double start = now();

    SessionMessage sessionMessage = {0, 0};
    sessionMessage.length = htonl(option.length);
    sessionMessage.number = htonl(option.number);

    if(stream->sendAll(&sessionMessage, sizeof(sessionMessage)) != sizeof(sessionMessage)) {
        perror("write SessionMessage");
        return;
    }

    const int total_len = sizeof(int32_t) + option.length;
    PayloadMessage* payload = (PayloadMessage*)malloc(total_len);
    std::unique_ptr<PayloadMessage, void(*)(void*)> freeIt(payload, ::free);
    assert(payload);

    payload->length = htonl(option.length);
    for(int i = 0; i < payload->length; i++) {
        payload->data[i] = "0123456789ABCDEF"[i % 16];
    }

    double total_mb = 1.0 * option.length * option.number / 1024 / 1024;
    printf("%.3f MiB in total\n", total_mb);

    for(int i = 0; i < option.number; i++) {
        int nw = stream->sendAll(payload, total_len);
        assert(nw == total_len);

        int ack = 0;
        int nr = stream->receiveAll(&ack, sizeof(ack));

        assert(nr == sizeof(ack));
        ack = ntohl(ack);

        assert(ack == option.length);

        double elapsed = now() - start;
        printf("%.3f seconds\n%.3f MiB/s\n", elapsed, total_mb / elapsed);
    }

}

void receive(Options option) {
    Acceptor acceptor(InetAddress(option.port));
    TcpStreamPtr stream(acceptor.accept());
    if(!stream)
        return;
    struct SessionMessage sessionMessage = {0, 0};
    if(stream->receiveAll(&sessionMessage, sizeof(sessionMessage)) != sizeof(sessionMessage)) {
        perror("read SessionMessage");
        return;
    }
    sessionMessage.number = ntohl(sessionMessage.number);
    sessionMessage.length = ntohl(sessionMessage.length);
    printf("receive buffer length = %d\nreceive number of buffers = %d\n",
         sessionMessage.length, sessionMessage.number);

    double total_mb = sessionMessage.length * sessionMessage.number / 1024.0 / 1024.0;
    printf("%.3f MiB in total\n", total_mb);

    const int total_len = sizeof(int32_t) + sessionMessage.length;
    PayloadMessage* payload = static_cast<PayloadMessage*>(::malloc(total_len));
    std::unique_ptr<PayloadMessage, void (*)(void*)> freeIt(payload, ::free);
    assert(payload);

    double start = now();

    for(int i = 0; i < sessionMessage.number; ++i) {
        payload->length = 0;
        if(stream->receiveAll(&payload->length, sizeof(payload->length) != sizeof(payload->length))) {
            perror("read length");
            return;
        }
        payload->length = ntohl(payload->length);
        assert(payload->length == sessionMessage.length);
        if (stream->receiveAll(payload->data, payload->length) != payload->length) {
            perror("read payload data");
            return;
        }
        int32_t ack = htonl(payload->length);
        if (stream->sendAll(&ack, sizeof(ack)) != sizeof(ack)) {
            perror("write ack");
            return;
        }
    }

    double elapsed = now() - start;
    printf("%.3f seconds\n%.3f MiB/s\n", elapsed, total_mb / elapsed);
}

bool parseCommandLine(int argc, char* argv[], Options* opt) {
    namespace po = boost::program_options;

    po::options_description desc ("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("port,p", po::value<uint16_t>(&opt->port)->default_value(5001), "TCP port")
        ("length,l", po::value<int>(&opt->length)->default_value(65536), "Buffer length")
        ("number,n", po::value<int>(&opt->number)->default_value(8192), "Number of buffers")
        ("trans,t", po::value<std::string>(&opt->host), "Transmit")
        ("recv,r", "Receive")
        ("nodelay,D", "set TCP_NODELAY")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    opt->transmit = vm.count("trans");
    opt->receive = vm.count("recv");
    opt->nodelay = vm.count("nodelay");

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return false;
    }

    if (opt->transmit && opt->receive) {
        std::cout << "Cannot transmit and receive at the same time" << std::endl;
        return false;
    }

    if (!opt->transmit && !opt->receive) {
        std::cout << "Must specify either transmit or receive" << std::endl;
        return false;
    }

    printf("port = %d\n", opt->port);

    if(opt->transmit) {
        printf("buffer length = %d\n", opt->length);
        printf("number of buffers = %d\n", opt->number);
    } else
        printf("accepting...\n");
    
    return true;
}

int main(int argc, char* argv[]) {
    Options options;
    if(parseCommandLine(argc, argv, &options)) {
        if(options.transmit)
            transmit(options);
        else if (options.receive) 
            receive(options);
        else
            assert(0);
    }
}