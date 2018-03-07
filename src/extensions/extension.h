//
//  extension.h
//  Residue
//
//  Copyright 2017-present Muflihun Labs
//
//  Author: @abumusamq
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#ifndef Extension_h
#define Extension_h

#include <atomic>
#include <mutex>
#include <string>

#include "core/json-doc.h"

#ifdef RESIDUE_EXTENSION_WIN
#   ifdef  RESIDUE_EXTENSION_LIB
#      define RESIDUE_EXTENSION_API __declspec(dllexport)
#   else
#      define RESIDUE_EXTENSION_API __declspec(dllimport)
#   endif
#else
#   define RESIDUE_EXTENSION_API
#endif

namespace residue {

///
/// \brief Abstract extension
/// Please extend one of the base classes.
///
/// \note All the extensions derived from any of the base class should be unique and definitive. Any conflicting name loaded on server
/// may crash the server. e.g, a <code>PostArchiveExtension</code> may be extended to a class called <code>SendToS3BucketExtension</code>
///
class RESIDUE_EXTENSION_API Extension
{
public:
    ///
    /// \brief Result of extension execution
    ///
    struct Result
    {
        ///
        /// \brief Status code of the execution
        ///
        int statusCode;

        ///
        /// \brief Whether process related to the execution should continue or not
        ///
        /// Some extensions do not honour this depending upon their type
        ///
        bool continueProcess;
    };

    ///
    /// \brief Initialze extension by type and ID
    /// \param id ID should be unique for each loaded extension.
    ///
    Extension(unsigned int type, const std::string& id);

    virtual ~Extension() = default;

protected:
    ///
    /// \brief You will override this function for actual work
    ///
    /// Do not forget to use both 'virtual' and 'override' modifiers
    ///
    virtual Result execute(void*) = 0;

    ///
    /// \brief Constant access to configurations for this extension
    ///
    inline const JsonDoc& conf() const
    {
        return m_config;
    }

    ///
    /// \brief Wrapper for Easylogging++ Level
    ///
    enum class LogLevel
    {
        Trace = 2,
        Debug = 4,
        Error = 16,
        Warning = 32,
        Info = 128,
        Verbose = 64
    };

    ///
    /// \brief Write log using 'residue' logger
    /// \param msg The log message
    /// \param level Logging level
    /// \param vlevel Verbose level if logging level is VERBOSE
    ///
    void writeLog(const std::string& msg, LogLevel level = LogLevel::Info, unsigned short vlevel = 0) const;
private:
    unsigned int m_type;
    std::string m_id;
    std::atomic<bool> m_running;
    std::mutex m_mutex;
    JsonDoc m_config;

    friend class ResidueLogDispatcher;
    friend class LogRotator;
    friend class Configuration;

    Result trigger(void*);

    inline void setConfig(JsonDoc::Value&& j)
    {
        m_config.set(j);
    }

    static Extension* load(const char*);
};

}

#define RESIDUE_EXTENSION(Name, Version)\
    extern "C" RESIDUE_EXTENSION_API Name* create_extension()\
    {\
        static Name singl;\
        return &singl;\
    }

#endif /* Extension_h */
