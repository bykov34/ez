
#include <iostream>

#include <ez/socket.hpp>
#include <ez/http.hpp>
#include <ez/tls.hpp>
#include <ez/hex.hpp>

int main()
{
    ez::socket socket;
    ez::tls tls(socket);
    ez::http http(tls);
    
    try
    {
        auto host = "localhost";
        socket.connect({192, 168, 100, 6}, 3024, 5);
        tls.connect(host, false);
        tls.handshake();
        auto pubkey = tls.pub_key();
        std::cout << "Cert public key: " << ez::hex::encode(pubkey) << std::endl;

        // TODO: check public key here for pinning
        
        http.send_request({ "GET", "/api/", {}, {} });
        auto [status, message, headers, body] = http.recv_response();
        
        std::cout << "Response: " << status << " " << message << std::endl;
        if (body.size() > 0)
            std::cout << "Body: " << body.c_str() << std::endl;
    }
    catch(const std::exception& _ex)
    {
        std::cout << "Error: " << _ex.what() << std::endl;
        return -1;
    }

    return 0;
}
