#include "source/all.hpp"


int main(int argc, char* argv[])
{
    namespace SFS = SimpleFastService_NS;

    struct SendResponse {
        SendResponse(std::string_view responseBody)
            : f_responseBody(responseBody)
        {}
        std::string_view getBody() const {
            return f_responseBody;
        }
        void operator()(const SFS::Request&, SFS::Response& resp) {
            resp.result(SFS::http::status::ok);
            resp.keep_alive(false);
            resp.body() = f_responseBody;
        }
    private:
        std::string_view f_responseBody;
    };
    try
    {
        SFS::Service service({ 9831, SFS::IpVersion::IPv4 }, {
            {{"/", SFS::http::verb::get}, SendResponse("{\"status\": \"OK\"}")},
            });
        service.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}