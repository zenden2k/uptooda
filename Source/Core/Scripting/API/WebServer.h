#ifndef IU_CORE_SCRIPTAPI_WEBSERVER_H
#define IU_CORE_SCRIPTAPI_WEBSERVER_H

#pragma once 

#include <memory>
#include <string>
#include "Core/Scripting/Squirrelnc.h"

namespace ScriptAPI {
		
class WebServerPrivate;

/*!
* @brief Start a web server listening a port on 127.0.0.1
* @since version 1.3.3
*/
class WebServer {
	public:
		WebServer();
		/**
		 * @brief Bind a port. Pass 0 as argument to use random port.
		 * 
		 * @param port 
		 * @return int Bound port
		 */
		int bind(int port);
		void start();

		/**
		 * @brief Register route handler
		 * 
		 * @param path regular expression
		 * @param method GET, POST, etc.
		 * @param callBack 
		 * @param context 
		 */
		void resource(const std::string& path, const std::string& method, Sqrat::Function callBack, Sqrat::Object context);
		int port() const;
		void stop();
	protected:
		std::shared_ptr<WebServerPrivate> d_;
};

/* @cond PRIVATE */
void RegisterWebServerClass(Sqrat::SqratVM& vm);
/* @endcond */

}

#endif
