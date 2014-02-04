#include "fcgiws/Server.h"
#include "Handler.h"

class MyHandler : public fcgiws::Handler
{
public:
    virtual fcgiws::ReponsePtr handle(const fcgiws::Request& request)
    {
	fcgiws::ResponsePtr response;
	response->statusCode = 200;
	response->statusMessage = OK;
	response->body = "what the fuck";
	return response;
    }
};


int main(char argc, char** argv) {
    MyHandler h;
    fcgiws::Server server;
    server.addHandler("/video/(.*)", h);
    server.run(10000);
}
