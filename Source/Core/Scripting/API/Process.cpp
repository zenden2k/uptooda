#include "Process.h"

//#undef environ
#include <algorithm>
#include <optional>

#include <boost/current_function.hpp>
#include <boost/process/v2.hpp>
#ifdef _WIN32
#include <boost/process/v2/windows/default_launcher.hpp>
#endif
#include "Core/Logging.h"
#include "Core/Utils/CoreUtils.h"
#include "ScriptAPI.h"
#include "../Squirrelnc.h"

namespace ScriptAPI {
namespace bp2 = ::boost::process::v2;
namespace asio = ::boost::asio;

class ProcessPrivate {
public:
    ProcessPrivate()
        : readProcessOutput_(false)
        , hidden_(false)
        , exit_code_(0)
        , ctx_() {
#ifdef _WIN32
        outputEncoding_ = "cp_oem";
#endif
    }

    ~ProcessPrivate() {
        if (process_ && process_->running()) {
            process_->detach();
        }
    }

    bool start() {
        std::vector<std::string> args;
        Sqrat::Array::iterator it;
        if (!arguments_.IsNull()) {
            while (arguments_.Next(it)) {
                Sqrat::Object obj(it.getValue(), GetCurrentThreadVM());
                args.push_back(obj.Cast<std::string>());
            }
        }

        try {
            bp2::process_stdio stdio_config;
            stdio_config.in = {}; 
            stdio_config.err = {};
            if (readProcessOutput_) {
                // Create pipes for output capture
                output_pipe_ = std::make_unique<asio::readable_pipe>(ctx_);
                stdio_config.out = *output_pipe_; // redirect stdout to pipe
            }

#ifdef _WIN32
            if (hidden_) {
                bp2::windows::default_launcher dl;
                dl.creation_flags |= CREATE_NO_WINDOW;
                process_ = dl(ctx_, executable_, args, stdio_config);
            } else {
                process_ = bp2::process(ctx_, executable_, args, stdio_config);
            }
#else
            process_ = bp2::process(ctx_, executable_, args, stdio_config);
#endif       
        } catch (const std::exception& ex) {
            LOG(ERROR) << IuCoreUtils::SystemLocaleToUtf8(ex.what());
            return false;
        }

        return true;
    }

    bool launchShell(const std::string& command) {
        try {
            std::vector<std::string> args;
            Sqrat::Array::iterator it;
            if (!arguments_.IsNull()) {
                while (arguments_.Next(it)) {
                    Sqrat::Object obj(it.getValue(), GetCurrentThreadVM());
                    args.push_back(obj.Cast<std::string>());
                }
            }

            // For shell execution, we need to use system shell
#ifdef _WIN32
            std::string shell_cmd = "cmd";
            args.insert(args.begin(), { "/c", command });
#else
            std::string shell_cmd = "/bin/sh";
            args.insert(args.begin(), { "-c", command });
#endif

            bp2::process_stdio stdio_config;
            stdio_config.in = {};
            stdio_config.err = {};
            if (readProcessOutput_) {
                // Create pipes for output capture
                output_pipe_ = std::make_unique<asio::readable_pipe>(ctx_);
                stdio_config.out = *output_pipe_; // redirect stdout to pipe
            }

#ifdef _WIN32
            if (hidden_) {
                bp2::windows::default_launcher dl;
                dl.creation_flags |= CREATE_NO_WINDOW;
                process_ = dl(ctx_, shell_cmd, args, stdio_config);
            } else {
                process_ = bp2::process(ctx_, shell_cmd, args, stdio_config);
            }
#else
            process_ = bp2::process(ctx_, shell_cmd, args, stdio_config);
#endif
            // Wait for completion for shell execution
            // waitForExit();
        } catch (const std::exception& ex) {
            LOG(ERROR) << IuCoreUtils::SystemLocaleToUtf8(ex.what());
            return false;
        }
        return true;
    }

    void readOutput(std::string& res) {
        if (!process_) {
            LOG(ERROR) << BOOST_CURRENT_FUNCTION << "\r\n"
                       << "Process not started";
            return;
        }

        if (!readProcessOutput_ || !output_pipe_) {
            LOG(ERROR) << BOOST_CURRENT_FUNCTION << "\r\n"
                       << "Output capture not enabled or pipe not created";
            return;
        }

        try {
            // Read from pipe synchronously
            std::string buffer;
            char read_buffer[4096];
            boost::system::error_code ec;

            while (true) {
                std::size_t bytes_read = output_pipe_->read_some(asio::buffer(read_buffer), ec);
                if (ec || bytes_read == 0) {
                    break;
                }
                buffer.append(read_buffer, bytes_read);
            }

            res = buffer;

            if (!outputEncoding_.empty()) {
                res = IuCoreUtils::ConvertToUtf8(res, outputEncoding_);
            }
#ifndef _WIN32
            else {
                res = IuCoreUtils::SystemLocaleToUtf8(res);
            }
#endif
        } catch (const std::exception& ex) {
            LOG(ERROR) << "Error reading process output: " << ex.what();
        }
    }

    int waitForExit() {
        if (!process_) {
            LOG(ERROR) << BOOST_CURRENT_FUNCTION << "\r\n"
                       << "Process not started";
            return EXIT_FAILURE;
        }

        try {
            process_->wait();
            exit_code_ = process_->exit_code();
            return exit_code_;
        } catch (const std::exception& ex) {
            LOG(ERROR) << "Error waiting for process: " << ex.what();
            return EXIT_FAILURE;
        }
    }

    std::string executable_;
    Sqrat::Array arguments_;
    bool readProcessOutput_;
    std::string outputEncoding_;
    asio::io_context ctx_;
    std::unique_ptr<asio::readable_pipe> output_pipe_;
    std::optional<bp2::process> process_;
    bool hidden_;
    int exit_code_;
};

Process::Process() {
    init();
}

Process::Process(const std::string& program) {
    init();
    d_->executable_ = program;
}

Process::Process(const std::string& executable, bool findInPath) {
    init();
    try {
        if (findInPath) {
            // In v2, we need to search manually or use full path
            auto found = bp2::environment::find_executable(executable);
            d_->executable_ = found.string();
            if (found.empty()) {
                d_->executable_ = executable;
                LOG(WARNING) << "Executable not found in PATH: " << executable;
            }
        } else {
            d_->executable_ = executable;
        }
    } catch (std::exception& ex) {
        LOG(ERROR) << ex.what();
        d_->executable_ = executable; // fallback
    }
}

void Process::setExecutable(const std::string& program) {
    d_->executable_ = program;
}

void Process::setArguments(Sqrat::Array arguments) {
    d_->arguments_ = arguments;
}

bool Process::launchInShell(const std::string& command) {
    return d_->launchShell(command);
}

bool Process::start() {
    return d_->start();
}

int Process::waitForExit() {
    return d_->waitForExit();
}

std::string Process::readOutput() {
    std::string res;
    d_->readOutput(res);
    return res;
}

void Process::setHidden(bool hidden) {
    d_->hidden_ = hidden; // Fixed: was always setting to true
}

void Process::setCaptureOutput(bool read) {
    d_->readProcessOutput_ = read;
}

std::string Process::findExecutableInPath(const std::string& executable) {
    return bp2::environment::find_executable(executable).string();
}

void Process::setOutputEncoding(const std::string& encoding) {
    d_->outputEncoding_ = encoding;
}

void Process::init() {
    d_.reset(new ProcessPrivate());
    d_->readProcessOutput_ = false;
    d_->hidden_ = false;
}

void RegisterProcessClass(Sqrat::SqratVM& vm)
{
    using namespace Sqrat;
    Sqrat::RootTable& root = vm.GetRootTable();
    root.Bind("Process", Class<Process>(vm.GetVM(), "Process")
        .Ctor()
        .Ctor<const std::string&>()
        .Ctor<const std::string&,bool>()
        .Func("setProgram", &Process::setExecutable)
        .Func("setArguments", &Process::setArguments)
        .Func("findExecutableInPath", &Process::findExecutableInPath)
        .Func("launchInShell", &Process::launchInShell)
        .Func("readOutput", &Process::readOutput)
        .Func("setCaptureOutput", &Process::setCaptureOutput)
        .Func("waitForExit", &Process::waitForExit)
        .Func("setHidden", &Process::setHidden)
        .Func("start", &Process::start)
    );
}

}
