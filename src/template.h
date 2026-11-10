// template.h:
//
// This file is part of the VSCP (https://www.vscp.org)
//
// The MIT License (MIT)
//
// Copyright © 2000-2026 Ake Hedman and contributors, the VSCP Project
// <info@vscp.org>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// ----------------------------------------------------------------------------
//  This is the main driver class for the template driver. Replace "template"
//  with the name of your driver and implement the actual driver functionality
//  in this class (see docs/creating-a-driver.md for a step by step guide).
// ----------------------------------------------------------------------------

#if !defined(VSCP_TEMPLATE_DRV_H__INCLUDED_)
#define VSCP_TEMPLATE_DRV_H__INCLUDED_

// Robust Windows platform detection for GitHub Actions and various build environments
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#ifndef WIN32
#define WIN32
#endif
#ifndef _WIN32
#define _WIN32
#endif
#endif

#ifdef WIN32
#include "StdAfx.h"
#endif

#include <list>
#include <string>

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#if WIN32
#else
#include <semaphore.h>
#include <syslog.h>
#include <unistd.h>
#endif

#include <time.h>

#include <guid.h>
#include <vscp.h>

#include <nlohmann/json.hpp> // Needs C++11  -std=c++11

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/spdlog.h"

// https://github.com/nlohmann/json
using json = nlohmann::json;

// Maximum number of events in the send queue
const uint16_t MAX_ITEMS_IN_QUEUE = 32000;

#define DRIVER_COPYRIGHT "Copyright © 2000-2026 Ake Hedman, the VSCP Project, https://www.vscp.org"

///////////////////////////////////////////////////////////////////////////////
// CTemplateDrv
//
// The main driver class. The VSCP driver API (see vscpl2drv-template.cpp)
// forwards all calls to an instance of this class.
//

class CTemplateDrv {

public:
  CTemplateDrv(void);
  ~CTemplateDrv(void);

  /*!
    @brief Init and start the driver
    @param path Path to configuration file (or JSON configuration string)
    @param pguid Pointer to GUID for the driver
    @return VSCP_ERROR_SUCCESS if all is OK, otherwise VSCP error code.
  */
  int open(std::string &path, const uint8_t *pguid);

  /*!
    @brief Close the driver
    @return VSCP_ERROR_SUCCESS if all is OK, otherwise VSCP error code.
  */
  int close(void);

  /*!
    @brief Load configuration from file or JSON string
    @param path Path to configuration file or a JSON configuration string
    @return VSCP_ERROR_SUCCESS if all is OK, otherwise VSCP error code.
            VSCP_ERROR_PARSING if there is a parsing error.
  */
  int doLoadConfig(std::string &path);

  /*!
    @brief Save configuration to file
    @return VSCP_ERROR_SUCCESS if all is OK, otherwise VSCP error code.
  */
  int doSaveConfig(void);

  /*!
    @brief Start the driver worker thread
    @return VSCP_ERROR_SUCCESS if all is OK, otherwise VSCP error code.
  */
  int start(void);

  /*!
    @brief Stop the driver worker thread
    @return VSCP_ERROR_SUCCESS if all is OK, otherwise VSCP error code.
  */
  int stop(void);

  /*!
    @brief Restart the driver worker thread
    @return VSCP_ERROR_SUCCESS if all is OK, otherwise VSCP error code.
  */
  int restart(void);

  /*!
    @brief Add an event to the send queue (host --> driver --> device)

    Events written to the driver by the host application (for example the
    VSCP daemon) end up here. The worker thread picks them up and should
    deliver them to the device/service this driver interfaces.

    @param pEvent Pointer to the VSCP event. The driver takes over ownership
      of the event and will free it when it has been handled.
    @return VSCP_ERROR_SUCCESS on success, otherwise VSCP error code.
  */
  int addEvent2SendQueue(const vscpEvent *pEvent);

  /*!
    @brief Add an event to the receive queue (device --> driver --> host)

    Call this method when the device/service this driver interfaces has
    produced data that should be sent as a VSCP event to the host
    application.

    @param pEvent Pointer to the VSCP event. The driver takes over ownership
      of the event and will free it when it has been handled.
    @return VSCP_ERROR_SUCCESS on success, otherwise VSCP error code.
  */
  int addEvent2ReceiveQueue(const vscpEvent *pEvent);

  /*!
    @brief Add an event ex to the receive queue (device --> driver --> host)
    @param ex Reference to the VSCP event ex
    @return VSCP_ERROR_SUCCESS on success, otherwise VSCP error code.
  */
  int addEvent2ReceiveQueue(vscpEventEx &ex);

public:
  /// Parsed Config file
  json m_j_config;

  // ------------------------------------------------------------------------

  // * * * Configuration

  /// Path to configuration file
  std::string m_path;

  /// True if config is remote writable
  bool m_bWriteEnable;

  /// Filters events we are not interested in
  vscpEventFilter m_rxfilter;

  /////////////////////////////////////////////////////////
  //                      Logging
  /////////////////////////////////////////////////////////

  bool m_bEnableFileLog;                       // True to enable logging
  spdlog::level::level_enum m_fileLogLevel;    // log level
  std::string m_fileLogPattern;                // log file pattern
  std::string m_path_to_log_file;              // Path to logfile
  uint32_t m_max_log_size;                     // Max size for logfile before rotating occurs
  uint16_t m_max_log_files;                    // Max log files to keep
  bool m_bConsoleLogEnable;                    // True to enable logging to console
  spdlog::level::level_enum m_consoleLogLevel; // log level
  std::string m_consoleLogPattern;             // Console log pattern

  // ------------------------------------------------------------------------

  bool m_bQuit;  // Flag to indicate that the worker thread should quit
  bool m_bDebug; // Debug flag (gives extra debug output)
  cguid m_guid;  // Driver GUID

#ifdef WIN32
  HANDLE m_semReceiveQueue; // Semaphore for receive queue
#else
  sem_t m_semReceiveQueue; // Semaphore for receive queue
#endif
  pthread_mutex_t m_mutexReceiveQueue;   // Mutex for receive queue
  std::list<vscpEvent *> m_receiveQueue; // Receive queue

#ifdef WIN32
  HANDLE m_semSendQueue; // Semaphore for send queue
#else
  sem_t m_semSendQueue; // Semaphore for send queue
#endif
  pthread_mutex_t m_mutexSendQueue;   // Mutex for send queue
  std::list<vscpEvent *> m_sendQueue; // Send queue

#ifdef WIN32
  HANDLE m_workerThread; // Worker thread for the driver
#else
  pthread_t m_workerThread; // Worker thread for the driver
#endif

private:
  uint16_t m_maxQueueSize; // Max size of send/receive queues
};

#endif // !defined(VSCP_TEMPLATE_DRV_H__INCLUDED_)
