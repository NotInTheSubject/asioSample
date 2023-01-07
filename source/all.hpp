#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "fields_alloc.hpp"

namespace SimpleFastService_NS {

	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	using tcp = boost::asio::ip::tcp;

	enum class IpVersion {
		IPv4,
		IPv6
	};

	struct NetParams {
		uint16_t port;
		IpVersion ipV;
	};

	struct EndpointKey {
		std::string_view endpoint;
		http::verb method;

		EndpointKey(std::string_view endpoint, http::verb method)
			: endpoint(endpoint)
			, method(method)
		{}

		// for using in std::map
		bool operator<(const EndpointKey& someEK) const {
			return (method == someEK.method) ? endpoint < someEK.endpoint : method < someEK.method;
		}
	};

	using Request = http::request<http::string_body, http::basic_fields<fields_alloc<char>>>;
	using Response = http::response<http::string_body, http::basic_fields<fields_alloc<char>>>;
	using RequestHandler = std::function<void(const Request&, Response&)>;
	using EndpointMap = std::map<EndpointKey, RequestHandler>;
	
	class Worker {
	public:
		Worker(const Worker&) = delete;
		Worker& operator=(const Worker&) = delete;

		Worker(tcp::acceptor& acceptor, const EndpointMap& handlers)
			: f_acceptor(acceptor), f_handlers(handlers)
		{
		}
		
		void start() {
			accept();
			checkDeadline();
		}
	private:
		using alloc_t = fields_alloc<char>;
		using ResponseSerializer = http::response_serializer<Response::body_type, Response::header::fields_type>;

		tcp::acceptor& f_acceptor;
		const EndpointMap& f_handlers;
		tcp::socket f_socket{ f_acceptor.get_executor() };
		beast::flat_static_buffer<8192> f_buffer;
		std::optional<http::request_parser<Request::body_type, alloc_t>> f_parser;
		std::optional<Response> f_respHolder;
		std::optional<ResponseSerializer> f_respSerializer;
		net::steady_timer f_deadline{
			f_acceptor.get_executor(), (std::chrono::steady_clock::time_point::max)() };
		alloc_t f_alloc{ 8192 };

		void accept()
		{
			// Clean up any previous connection.
			beast::error_code ec;
			f_socket.close(ec);
			f_buffer.consume(f_buffer.size());

			f_acceptor.async_accept(
				f_socket,
				[this](beast::error_code ec)
				{
					if (ec)
					{
						accept();
					}
					else
					{
						// Request must be fully processed within 60 seconds.
						f_deadline.expires_after(
							std::chrono::seconds(60));

						readRequest();
					}
				});
		}

		void readRequest()
		{
			// On each read the parser needs to be destroyed and
			// recreated. We store it in a boost::optional to
			// achieve that.
			//
			// Arguments passed to the parser constructor are
			// forwarded to the message object. A single argument
			// is forwarded to the body constructor.
			//
			// We construct the dynamic body with a 1MB limit
			// to prevent vulnerability to buffer attacks.
			//
			f_parser.emplace(
				std::piecewise_construct,
				std::make_tuple(),
				std::make_tuple(f_alloc));

			http::async_read(
				f_socket,
				f_buffer,
				*f_parser,
				[this](beast::error_code ec, std::size_t) {
					if (ec)
					accept();
					else {
						const auto startProcessing = std::chrono::system_clock::now();
						processRequest(f_parser->get());
						// [Processing time]
						std::cout << "[PCD]: " << static_cast<std::chrono::duration<double>>(std::chrono::system_clock::now() - startProcessing).count() << std::endl;
					}
				});
		}

		void processRequest(const Request& req)
		{
			f_respHolder.emplace(
				std::piecewise_construct,
				std::make_tuple(),
				std::make_tuple(f_alloc));

			const auto findingTarget = std::string_view(req.target().data(), std::min(req.target().find('?'), req.target().size()));
			const auto findingKey = EndpointKey(findingTarget, req.method());
			if (const auto iter = f_handlers.find(findingKey); iter != f_handlers.end()) {
				iter->second(req, f_respHolder.value());
			}
			else {
				f_respHolder->result(http::status::bad_request);
				f_respHolder->keep_alive(false);
				f_respHolder->body() = "{ \"error\": \"url cannot be resolved\" }";
			}

			f_respHolder->prepare_payload();
			f_respSerializer.emplace(f_respHolder.value());
			http::async_write(
				f_socket,
				f_respSerializer.value(),
				[this](beast::error_code ec, std::size_t) {
					f_socket.shutdown(tcp::socket::shutdown_send, ec);
					f_respHolder.reset();
					f_respSerializer.reset();
					accept();
				});
		}

		void checkDeadline()
		{
			// The deadline may have moved, so check it has really passed.
			if (f_deadline.expiry() <= std::chrono::steady_clock::now())
			{
				// Close socket to cancel any outstanding operation.
				f_socket.close();

				// Sleep indefinitely until we're given a new deadline.
				f_deadline.expires_at(
					(std::chrono::steady_clock::time_point::max)());
			}

			f_deadline.async_wait(
				[this](beast::error_code) {
					checkDeadline();
				});
		}
	};

	class Service {
	public:

		Service(const Service&) = delete;
		Service& operator=(const Service&) = delete;

		Service(NetParams netParams, EndpointMap handlers)
			: f_ioc(BOOST_ASIO_CONCURRENCY_HINT_1)
			, f_acceptor(initAccesptor(f_ioc, netParams))
			, f_handlers(handlers)
		{
			const uint32_t workersCount = 512;
			for (uint32_t i = 0; i < workersCount; ++i) {
				f_workers.emplace_back(f_acceptor, f_handlers);
				f_workers.back().start();
			}
		}

		void run() {
			f_ioc.run();
		}

	private:
		net::io_context f_ioc;
		tcp::acceptor f_acceptor;
		std::list<Worker> f_workers;
		EndpointMap f_handlers;

		static tcp::acceptor initAccesptor(net::io_context& ioc, NetParams netParams) {
			const auto address = net::ip::make_address(netParams.ipV == IpVersion::IPv4 ? "0.0.0.0" : "0::0");
			return tcp::acceptor(ioc, { address, netParams.port });
		}
	};

} // namespace SimpleFastService_NS

