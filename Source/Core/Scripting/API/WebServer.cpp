#include "WebServer.h"

//#include <boost/property_tree/ptree.hpp>

#include "Core/3rdpart/SimpleWebServer/server_http.hpp"
#include "Core/Logging.h"
#include "Core/Utils/CoreUtils.h"

#include "ScriptAPI.h"

namespace ScriptAPI {

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

class WebServerPrivate : public Stoppable {

public:
    HttpServer server_;
    std::vector<Sqrat::Function> callbacks_;

    WebServerPrivate() {
        AddServiceToVM(GetCurrentThreadVM(), this);
    }
    ~WebServerPrivate() override {
        RemoveServiceFromVM(GetCurrentThreadVM(), this);
    }
    void stop() override {
        server_.stop();
    }
};

WebServer::WebServer(): d_(std::make_shared<WebServerPrivate>()){
}

WebServer::~WebServer() {
	
}

void WebServer::resource(const std::string& path, const std::string& method, Sqrat::Function callBack, Sqrat::Object context) {
    d_->callbacks_.push_back(callBack);
    size_t callbackIndex = d_->callbacks_.size()-1;
	
    //using namespace boost::property_tree;
	
	d_->server_.resource[path][method] = [this, callbackIndex](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
        try {
            Sqrat::Table data(GetCurrentThreadVM());
            data.SetValue("method", request->method);
            data.SetValue("path", request->path);
            data.SetValue("queryString", request->query_string);
            data.SetValue("httpVersion", request->http_version);

            SimpleWeb::CaseInsensitiveMultimap queryParams = request->parse_query_string();
            Sqrat::Table queryParamsTable(GetCurrentThreadVM());
        	for(auto const& [key, val] : queryParams) {
                queryParamsTable.SetValue(key.c_str(), val);
        	}
            data.SetValue("queryParams", queryParamsTable);

            Sqrat::SharedPtr<Sqrat::Table> result = d_->callbacks_[callbackIndex].Evaluate<Sqrat::Table>(data);
            Sqrat::SharedPtr<std::string> responseBody = result->GetValue<std::string>("responseBody");
           
            if (result->HasKey("stopDelay")) {
                int stopDelay = *result->GetValue<int>("stopDelay");
            	if (stopDelay) {
            		std::thread t([this, stopDelay] {
                        using namespace std::chrono_literals;
            			try {
							std::this_thread::sleep_for(std::chrono::milliseconds(stopDelay));
                        } catch (const std::exception&) {
                           // ex.what();
                        }
							d_->server_.stop();
                        });
                    t.detach();
            	}
            }

            *response << "HTTP/1.1 200 OK\r\n"
                << "Content-Length: " << responseBody->length() << "\r\n"
        		    "Content-Type: text/html; charset=utf-8" << "\r\n\r\n"
                << *responseBody.Get();
        }
        catch (const std::exception& e) {
            *response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n"
                << e.what();
        }


        // Alternatively, using a convenience function:
        // try {
        //     ptree pt;
        //     read_json(request->content, pt);

        //     auto name=pt.get<string>("firstName")+" "+pt.get<string>("lastName");
        //     response->write(name);
        // }
        // catch(const exception &e) {
        //     response->write(SimpleWeb::StatusCode::client_error_bad_request, e.what());
        // }
    };


}

void WebServer::start() {
    d_->server_.accept_and_run();
}

int WebServer::port() const {
    return d_->server_.config.port;
}

void WebServer::stop() {
    d_->server_.stop();
}

int WebServer::bind(int port) {
    d_->server_.config.address = "127.0.0.1";
    d_->server_.config.port = static_cast<unsigned int>(port);
    try {
        return d_->server_.bind();
    }
    catch (const std::exception& ex) {
        //LOG(ERROR) << IuCoreUtils::SystemLocaleToUtf8(ex.what()) << std::endl;
    }
    return 0;
}


void RegisterWebServerClass(Sqrat::SqratVM& vm) {
    using namespace Sqrat;
    Sqrat::RootTable& root = vm.GetRootTable();

    /*root.Bind("WebServerRequest", Class<WebServer>(vm.GetVM(), "WebServerRequest")
        .Ctor()
        .Func("method", &WebServerRequest::method)
        .Func("path", &WebServerRequest::path)
        .Func("queryString", &WebServerRequest::queryString)
        .Func("httpVersion", &WebServerRequest::httpVersion)
    );*/
	
    root.Bind("WebServer", Class<WebServer>(vm.GetVM(), "WebServer")
        .Ctor()
        .Func("bind", &WebServer::bind)
        .Func("start", &WebServer::start)
        .Func("stop", &WebServer::stop)
        .Func("port", &WebServer::port)
        .Func("resource", &WebServer::resource)
    );
}

}
