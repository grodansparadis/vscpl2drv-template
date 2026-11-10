// template.cpp:
//
// This file is part of the VSCP (https://www.vscp.org)
//
// The MIT License (MIT)
//
// Copyright © 2000-2026 Ake Hedman, the VSCP Project
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
//  This is the main driver class implementation for the template driver.
//  Search for "TODO" to find the places where you add your own functionality.
// ----------------------------------------------------------------------------

#ifdef WIN32
#include "StdAfx.h"
#endif

#include <fstream>
#include <iostream>
#include <string>

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <vscp.h>
#include <vscphelper.h>

#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "template.h"

// Forward declaration of worker thread
#ifdef WIN32
DWORD WINAPI
workerThread(LPVOID pData);
#else
void *
workerThread(void *pData);
#endif

////////////////////////////////////////////////////////////////////////////////
// Constructor
//

CTemplateDrv::CTemplateDrv(void)
{
  m_bQuit = false;

#ifdef WIN32
  // Initialize Windows sockets
  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    spdlog::error("WSAStartup failed: {0}", iResult);
  }
#endif

  vscp_clearVSCPFilter(&m_rxfilter); // Accept all events

#ifdef WIN32
  m_workerThread = NULL;
#else
  m_workerThread = 0;
#endif
  m_bDebug       = false;
  m_bWriteEnable = false;
  m_maxQueueSize = MAX_ITEMS_IN_QUEUE;

// Initialise semaphores
#ifdef WIN32
  m_semSendQueue    = CreateSemaphore(NULL, 0, MAX_ITEMS_IN_QUEUE, NULL);
  m_semReceiveQueue = CreateSemaphore(NULL, 0, MAX_ITEMS_IN_QUEUE, NULL);
#else
  sem_init(&m_semSendQueue, 0, 0);
  sem_init(&m_semReceiveQueue, 0, 0);
#endif

  // Initialise mutexes
  pthread_mutex_init(&m_mutexSendQueue, NULL);
  pthread_mutex_init(&m_mutexReceiveQueue, NULL);

  // Setting up logging defaults
  m_bConsoleLogEnable = true;
  m_consoleLogLevel   = spdlog::level::info;
  m_consoleLogPattern = "[vscpl2drv-template: %c] [%^%l%$] %v";

  m_bEnableFileLog = false;
  m_fileLogLevel   = spdlog::level::info;
  m_fileLogPattern = "[vscpl2drv-template: %c] [%^%l%$] %v";
#ifdef WIN32
  m_path_to_log_file = "vscpl2drv-template.log";
#else
  m_path_to_log_file = "/tmp/vscpl2drv-template.log";
#endif
  m_max_log_size  = 5242880; // 5 MB
  m_max_log_files = 7;

  // Init logging pool
  spdlog::init_thread_pool(8192, 1);

  // Flush log every five seconds
  spdlog::flush_every(std::chrono::seconds(5));

  ////////////////////////////////////////////////////////
  //      Log files are configured in doLoadConfig      //
  ////////////////////////////////////////////////////////

  // TODO: Set defaults for your own configuration values here
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
//

CTemplateDrv::~CTemplateDrv(void)
{
  close();

#ifdef WIN32
  CloseHandle(m_semSendQueue);
  CloseHandle(m_semReceiveQueue);
#else
  sem_destroy(&m_semSendQueue);
  sem_destroy(&m_semReceiveQueue);
#endif

  pthread_mutex_destroy(&m_mutexSendQueue);
  pthread_mutex_destroy(&m_mutexReceiveQueue);

  // Clear send queue
  while (!m_sendQueue.empty()) {
    vscpEvent *pEvent = m_sendQueue.front();
    m_sendQueue.pop_front();
    vscp_deleteEvent_v2(&pEvent);
  }

  // Clear receive queue
  while (!m_receiveQueue.empty()) {
    vscpEvent *pEvent = m_receiveQueue.front();
    m_receiveQueue.pop_front();
    vscp_deleteEvent_v2(&pEvent);
  }

#ifdef WIN32
  // Cleanup Windows sockets
  WSACleanup();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// open
//

int
CTemplateDrv::open(std::string &path, const uint8_t *pguid)
{
  int rv;

  // Must have a valid GUID
  if (NULL == pguid) {
    return VSCP_ERROR_PARAMETER;
  }

  // Set GUID
  m_guid.getFromArray(pguid);

  // Save path for config file
  m_path = path;

  // Read configuration file
  if (VSCP_ERROR_SUCCESS != (rv = doLoadConfig(path))) {
    spdlog::error("[vscpl2drv-template] Failed to load configuration file.");
    return rv;
  }

  // TODO: Open/connect to your device or service here, or do it in the
  //       worker thread if that suits your driver better.

  // Start the worker thread
  if (VSCP_ERROR_SUCCESS != start()) {
    spdlog::critical("[vscpl2drv-template] Failed to start driver.");
    spdlog::drop_all();
    return VSCP_ERROR_INIT_FAIL;
  }

  // Everything is OK
  m_bQuit = false;

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// close
//

int
CTemplateDrv::close(void)
{
  // Do nothing if already terminated
  if (m_bQuit) {
    spdlog::drop_all();
    return VSCP_ERROR_SUCCESS;
  }

  // Stop the worker thread
  stop();

  // TODO: Close/disconnect from your device or service here if you opened
  //       it in open().

  spdlog::drop_all();
  spdlog::shutdown();

  return VSCP_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// doLoadConfig
//

int
CTemplateDrv::doLoadConfig(std::string &path)
{
  // Take away possible whitespace
  vscp_trim(path);

  // If first character of path is "{" we expect path to be a JSON
  // configuration string for us to parse
  if ('{' == path.at(0)) {
    spdlog::info("Configuration appears to be a JSON string.");
    try {
      m_j_config = json::parse(path);
    }
    catch (json::parse_error) {
      spdlog::critical("Failed to parse JSON configuration.");
      return VSCP_ERROR_PARSING;
    }
    catch (std::exception &e) {
      spdlog::critical("Failed to parse configuration. Exception='{}'", e.what());
      return VSCP_ERROR_PARSING;
    }
    catch (...) {
      spdlog::critical("Failed to parse configuration due to unknown error.");
      return VSCP_ERROR_PARSING;
    }
  }
  // If path is a file read it
  else if (vscp_fileExists(path)) {
    spdlog::info("Configuration appears to be a file.");
    try {
      std::ifstream in(path, std::ifstream::in);
      in >> m_j_config;
    }
    catch (json::parse_error) {
      spdlog::critical("Failed to parse JSON configuration.");
      return VSCP_ERROR_PARSING;
    }
    catch (std::exception &e) {
      spdlog::critical("Failed to parse configuration. Exception='{}'", e.what());
      return VSCP_ERROR_PARSING;
    }
    catch (...) {
      spdlog::critical("Failed to parse configuration due to unknown error.");
      return VSCP_ERROR_PARSING;
    }
  }
  else {
    // Configuration file does not exist
    return VSCP_ERROR_PARAMETER;
  }

  // Enable extra debug output
  if (m_j_config.contains("debug")) {
    try {
      m_bDebug = m_j_config["debug"].get<bool>();
    }
    catch (const std::exception &ex) {
      spdlog::error("ReadConfig: Failed to read 'debug' Error='{0}'", ex.what());
    }
    catch (...) {
      spdlog::error("ReadConfig: Failed to read 'debug' due to unknown error.");
    }
  }
  else {
    spdlog::warn("ReadConfig: Failed to read 'debug' Defaults will be used.");
  }

  // write
  if (m_j_config.contains("write")) {
    try {
      m_bWriteEnable = m_j_config["write"].get<bool>();
    }
    catch (const std::exception &ex) {
      spdlog::error("ReadConfig: Failed to read 'write' Error='{0}'", ex.what());
    }
    catch (...) {
      spdlog::error("ReadConfig: Failed to read 'write' due to unknown error.");
    }
  }
  else {
    spdlog::warn("ReadConfig: Failed to read 'write' Defaults will be used.");
  }

  // Receive filter
  if (m_j_config.contains("rx-filter")) {
    try {
      std::string filter = m_j_config["rx-filter"].get<std::string>();
      if (!vscp_readFilterFromString(&m_rxfilter, filter.c_str())) {
        spdlog::error("ReadConfig: Failed to parse 'rx-filter' filter='{}'", filter);
      }
    }
    catch (const std::exception &ex) {
      spdlog::error("ReadConfig: Failed to read 'rx-filter' Error='{}'", ex.what());
    }
    catch (...) {
      spdlog::error("ReadConfig: Failed to read 'rx-filter' due to unknown error.");
    }
  }
  else {
    spdlog::warn("ReadConfig: Failed to read 'rx-filter' Defaults will be used.");
  }

  // Max queue size
  if (m_j_config.contains("max-queue-size")) {
    try {
      m_maxQueueSize = m_j_config["max-queue-size"].get<uint16_t>();
    }
    catch (const std::exception &ex) {
      spdlog::error("ReadConfig: Failed to read 'max-queue-size' Error='{}'", ex.what());
    }
    catch (...) {
      spdlog::error("ReadConfig: Failed to read 'max-queue-size' due to unknown error.");
    }
  }

  // TODO: Read your own configuration values here following the same
  //       pattern as above.

  ///////////////////////////////////////////////////////////////////////////
  //                             Logging
  ///////////////////////////////////////////////////////////////////////////

  if (m_j_config.contains("logging") && m_j_config["logging"].is_object()) {

    json j = m_j_config["logging"];

    // Logging: file-enable-log
    if (j.contains("file-enable-log")) {
      try {
        m_bEnableFileLog = j["file-enable-log"].get<bool>();
      }
      catch (const std::exception &ex) {
        spdlog::error("ReadConfig: Failed to read 'file-enable-log' Error='{}'", ex.what());
      }
      catch (...) {
        spdlog::error("ReadConfig: Failed to read 'file-enable-log' due to unknown error.");
      }
    }

    // Logging: file-log-level
    if (j.contains("file-log-level")) {
      std::string str;
      try {
        str = j["file-log-level"].get<std::string>();
      }
      catch (const std::exception &ex) {
        spdlog::error("ReadConfig: Failed to read 'file-log-level' Error='{0}'", ex.what());
      }
      catch (...) {
        spdlog::error("ReadConfig: Failed to read 'file-log-level' due to unknown error.");
      }
      vscp_makeLower(str);
      if (std::string::npos != str.find("off")) {
        m_fileLogLevel = spdlog::level::off;
      }
      else if (std::string::npos != str.find("critical")) {
        m_fileLogLevel = spdlog::level::critical;
      }
      else if (std::string::npos != str.find("err")) {
        m_fileLogLevel = spdlog::level::err;
      }
      else if (std::string::npos != str.find("warn")) {
        m_fileLogLevel = spdlog::level::warn;
      }
      else if (std::string::npos != str.find("info")) {
        m_fileLogLevel = spdlog::level::info;
      }
      else if (std::string::npos != str.find("debug")) {
        m_fileLogLevel = spdlog::level::debug;
      }
      else if (std::string::npos != str.find("trace")) {
        m_fileLogLevel = spdlog::level::trace;
      }
      else {
        spdlog::error("ReadConfig: LOGGING 'file-log-level' has invalid value [{}]. Default value used.", str);
      }
    }

    // Logging: file-pattern
    if (j.contains("file-pattern")) {
      try {
        m_fileLogPattern = j["file-pattern"].get<std::string>();
      }
      catch (const std::exception &ex) {
        spdlog::error("ReadConfig: Failed to read 'file-pattern' Error='{}'", ex.what());
      }
      catch (...) {
        spdlog::error("ReadConfig: Failed to read 'file-pattern' due to unknown error.");
      }
    }

    // Logging: file-path
    if (j.contains("file-path")) {
      try {
        m_path_to_log_file = j["file-path"].get<std::string>();
      }
      catch (const std::exception &ex) {
        spdlog::error("ReadConfig: Failed to read 'file-path' Error='{}'", ex.what());
      }
      catch (...) {
        spdlog::error("ReadConfig: Failed to read 'file-path' due to unknown error.");
      }
    }

    // Logging: file-max-size
    if (j.contains("file-max-size")) {
      try {
        m_max_log_size = j["file-max-size"].get<uint32_t>();
      }
      catch (const std::exception &ex) {
        spdlog::error("ReadConfig: Failed to read 'file-max-size' Error='{}'", ex.what());
      }
      catch (...) {
        spdlog::error("ReadConfig: Failed to read 'file-max-size' due to unknown error.");
      }
    }

    // Logging: file-max-files
    if (j.contains("file-max-files")) {
      try {
        m_max_log_files = j["file-max-files"].get<uint16_t>();
      }
      catch (const std::exception &ex) {
        spdlog::error("ReadConfig: Failed to read 'file-max-files' Error='{}'", ex.what());
      }
      catch (...) {
        spdlog::error("ReadConfig: Failed to read 'file-max-files' due to unknown error.");
      }
    }

    // Logging: console-enable-log
    if (j.contains("console-enable-log")) {
      try {
        m_bConsoleLogEnable = j["console-enable-log"].get<bool>();
      }
      catch (const std::exception &ex) {
        spdlog::error("ReadConfig: Failed to read 'console-enable-log' Error='{}'", ex.what());
      }
      catch (...) {
        spdlog::error("ReadConfig: Failed to read 'console-enable-log' due to unknown error.");
      }
    }

    // Logging: console-log-level
    if (j.contains("console-log-level")) {
      std::string str;
      try {
        str = j["console-log-level"].get<std::string>();
      }
      catch (const std::exception &ex) {
        spdlog::error("ReadConfig: Failed to read 'console-log-level' Error='{0}'", ex.what());
      }
      catch (...) {
        spdlog::error("ReadConfig: Failed to read 'console-log-level' due to unknown error.");
      }
      vscp_makeLower(str);
      if (std::string::npos != str.find("off")) {
        m_consoleLogLevel = spdlog::level::off;
      }
      else if (std::string::npos != str.find("critical")) {
        m_consoleLogLevel = spdlog::level::critical;
      }
      else if (std::string::npos != str.find("err")) {
        m_consoleLogLevel = spdlog::level::err;
      }
      else if (std::string::npos != str.find("warn")) {
        m_consoleLogLevel = spdlog::level::warn;
      }
      else if (std::string::npos != str.find("info")) {
        m_consoleLogLevel = spdlog::level::info;
      }
      else if (std::string::npos != str.find("debug")) {
        m_consoleLogLevel = spdlog::level::debug;
      }
      else if (std::string::npos != str.find("trace")) {
        m_consoleLogLevel = spdlog::level::trace;
      }
      else {
        spdlog::error("ReadConfig: LOGGING 'console-log-level' has invalid value [{}]. Default value used.", str);
      }
    }

    // Logging: console-pattern
    if (j.contains("console-pattern")) {
      try {
        m_consoleLogPattern = j["console-pattern"].get<std::string>();
      }
      catch (const std::exception &ex) {
        spdlog::error("ReadConfig: Failed to read 'console-pattern' Error='{}'", ex.what());
      }
      catch (...) {
        spdlog::error("ReadConfig: Failed to read 'console-pattern' due to unknown error.");
      }
    }

  } // Logging
  else {
    spdlog::info("ReadConfig: No logging has been setup.");
  }

  ///////////////////////////////////////////////////////////////////////////
  //                          Setup logger
  ///////////////////////////////////////////////////////////////////////////

  // Console log
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

  if (m_bConsoleLogEnable) {
    console_sink->set_level(m_consoleLogLevel);
    console_sink->set_pattern(m_consoleLogPattern);
  }
  else {
    // If disabled set to off
    console_sink->set_level(spdlog::level::off);
  }

  auto rotating_file_sink =
    std::make_shared<spdlog::sinks::rotating_file_sink_mt>(m_path_to_log_file.c_str(), m_max_log_size, m_max_log_files);

  if (m_bEnableFileLog) {
    rotating_file_sink->set_level(m_fileLogLevel);
    rotating_file_sink->set_pattern(m_fileLogPattern);
  }
  else {
    // If disabled set to off
    rotating_file_sink->set_level(spdlog::level::off);
  }

  std::vector<spdlog::sink_ptr> sinks{ console_sink, rotating_file_sink };
  auto logger = std::make_shared<spdlog::async_logger>("logger",
                                                       sinks.begin(),
                                                       sinks.end(),
                                                       spdlog::thread_pool(),
                                                       spdlog::async_overflow_policy::block);
  spdlog::register_logger(logger);
  spdlog::set_default_logger(logger);

  spdlog::debug("Read configuration.");

  return VSCP_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// doSaveConfig
//

int
CTemplateDrv::doSaveConfig(void)
{
  if (m_j_config.value("write", false)) {
    // TODO: Write (possibly updated) configuration back to m_path here.
  }
  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// start
//

int
CTemplateDrv::start(void)
{
  spdlog::debug("[vscpl2drv-template] Starting worker thread...");

  m_bQuit = false;

#ifdef WIN32
  m_workerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) workerThread, this, 0, NULL);
  if (m_workerThread == NULL) {
    spdlog::error("[vscpl2drv-template] Unable to start the worker thread.");
    return VSCP_ERROR_ERROR;
  }
#else
  if (pthread_create(&m_workerThread, NULL, workerThread, this)) {
    spdlog::error("[vscpl2drv-template] Unable to start the worker thread.");
    return VSCP_ERROR_ERROR;
  }
#endif

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// stop
//

int
CTemplateDrv::stop(void)
{
  // Tell the thread it's time to quit
  m_bQuit = true;

  spdlog::debug("[vscpl2drv-template] Terminating worker thread.");

// Wait for thread to finish and clean up
#ifdef WIN32
  if (m_workerThread == NULL) { // Check if thread was created
    // Not started
    return VSCP_ERROR_SUCCESS;
  }
  WaitForSingleObject(m_workerThread, INFINITE);
  CloseHandle(m_workerThread);
  m_workerThread = NULL; // Reset thread handle
#else
  if (!m_workerThread) { // Check if thread was created
    // Not started
    return VSCP_ERROR_SUCCESS;
  }
  pthread_join(m_workerThread, NULL);
  m_workerThread = 0; // Reset thread ID
#endif

  spdlog::debug("[vscpl2drv-template] Terminated worker thread.");

  return VSCP_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// restart
//

int
CTemplateDrv::restart(void)
{
  if (VSCP_ERROR_SUCCESS != stop()) {
    spdlog::warn("[vscpl2drv-template] Failed to stop driver.");
  }

  if (VSCP_ERROR_SUCCESS != start()) {
    spdlog::warn("[vscpl2drv-template] Failed to start driver.");
  }

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////
// addEvent2SendQueue
//
//  Add event to send queue (host --> driver --> device)
//

int
CTemplateDrv::addEvent2SendQueue(const vscpEvent *pEvent)
{
  pthread_mutex_lock(&m_mutexSendQueue);
  if (m_maxQueueSize && (m_sendQueue.size() >= m_maxQueueSize)) {
    pthread_mutex_unlock(&m_mutexSendQueue);
    spdlog::warn("[vscpl2drv-template] Send queue full, dropping event.");
    return VSCP_ERROR_FIFO_FULL;
  }
  m_sendQueue.push_back((vscpEvent *) pEvent);
#ifdef WIN32
  ReleaseSemaphore(m_semSendQueue, 1, NULL);
#else
  sem_post(&m_semSendQueue);
#endif
  pthread_mutex_unlock(&m_mutexSendQueue);
  return VSCP_ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
// addEvent2ReceiveQueue
//
//  Add event to receive queue (device --> driver --> host)
//

int
CTemplateDrv::addEvent2ReceiveQueue(const vscpEvent *pEvent)
{
  pthread_mutex_lock(&m_mutexReceiveQueue);
  if (m_maxQueueSize && (m_receiveQueue.size() >= m_maxQueueSize)) {
    pthread_mutex_unlock(&m_mutexReceiveQueue);
    spdlog::warn("[vscpl2drv-template] Receive queue full, dropping event.");
    return VSCP_ERROR_FIFO_FULL;
  }
  m_receiveQueue.push_back((vscpEvent *) pEvent);
  pthread_mutex_unlock(&m_mutexReceiveQueue);
#ifdef WIN32
  ReleaseSemaphore(m_semReceiveQueue, 1, NULL);
#else
  sem_post(&m_semReceiveQueue);
#endif
  return VSCP_ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
// addEvent2ReceiveQueue
//

int
CTemplateDrv::addEvent2ReceiveQueue(vscpEventEx &ex)
{
  vscpEvent *pev = new vscpEvent();
  if (NULL == pev) {
    spdlog::error("[vscpl2drv-template] addEvent2ReceiveQueue - Unable to allocate event storage.");
    return VSCP_ERROR_MEMORY;
  }

  pev->pdata = NULL;

  if (!vscp_convertEventExToEvent(pev, &ex)) {
    spdlog::error("[vscpl2drv-template] Failed to convert event from ex to ev.");
    vscp_deleteEvent(pev);
    return VSCP_ERROR_ERROR;
  }

  if (vscp_doLevel2Filter(pev, &m_rxfilter)) {
    return addEvent2ReceiveQueue(pev);
  }
  else {
    vscp_deleteEvent(pev);
  }

  return VSCP_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//                           Worker thread
///////////////////////////////////////////////////////////////////////////////

//
// The worker thread is the heart of the driver. It is responsible for
// transferring events between the host application and the device/service
// this driver interfaces.
//
//  * Events from the host shows up in the send queue (m_sendQueue). Deliver
//    them to your device.
//
//  * Data from your device should be converted to VSCP events and handed
//    to the host with addEvent2ReceiveQueue().
//

#ifdef WIN32
DWORD WINAPI
workerThread(LPVOID pData)
#else
void *
workerThread(void *pData)
#endif
{
  CTemplateDrv *pDrv = (CTemplateDrv *) pData;
  if (NULL == pDrv) {
    spdlog::error("[vscpl2drv-template] workerThread: Invalid CTemplateDrv pointer.");
#ifdef WIN32
    return 0;
#else
    return NULL;
#endif
  }

  spdlog::info("[vscpl2drv-template] Worker thread started.");

  // TODO: If your driver connects to a device or service, open the
  //       connection here (and reconnect when it is lost).

  while (!pDrv->m_bQuit) {

    //--------------------------------------------------------------------
    // Wait (max 100 ms) for an event from the host to send to the device
    //--------------------------------------------------------------------

    bool bEventAvailable = false;

#ifdef WIN32
    if (WaitForSingleObject(pDrv->m_semSendQueue, 100) == WAIT_OBJECT_0) {
      bEventAvailable = true;
    }
#elif defined(__linux__)
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
      continue;
    }
    ts.tv_nsec += 100000000; // 100 ms
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_sec++;
      ts.tv_nsec -= 1000000000;
    }
    if (sem_timedwait(&pDrv->m_semSendQueue, &ts) == 0) {
      bEventAvailable = true;
    }
#else
    // macOS and other Unix systems - use polling approach
    for (int i = 0; i < 10; i++) { // Poll for 100 ms total (10 * 10 ms)
      if (sem_trywait(&pDrv->m_semSendQueue) == 0) {
        bEventAvailable = true;
        break;
      }
      usleep(10000); // Sleep 10 ms
    }
#endif

    if (bEventAvailable && !pDrv->m_bQuit) {

      // Get the event from the send queue
      vscpEvent *pEvent = NULL;
      pthread_mutex_lock(&pDrv->m_mutexSendQueue);
      if (!pDrv->m_sendQueue.empty()) {
        pEvent = pDrv->m_sendQueue.front();
        pDrv->m_sendQueue.pop_front();
      }
      pthread_mutex_unlock(&pDrv->m_mutexSendQueue);

      if (NULL == pEvent) {
        continue;
      }

      // ----------------------------------------------------------------
      // TODO: Here is where you send the event to your device/service.
      //
      // The template driver just logs the event and, as a demonstration,
      // loops it back to the host application (remove the loopback for a
      // real driver).
      // ----------------------------------------------------------------

      if (pDrv->m_bDebug) {
        std::string str;
        vscp_convertEventToString(str, pEvent);
        spdlog::debug("[vscpl2drv-template] Event to device: {}", str);
      }

      // DEMO: loop event back to the host (remove in a real driver)
      if (VSCP_ERROR_SUCCESS != pDrv->addEvent2ReceiveQueue(pEvent)) {
        vscp_deleteEvent_v2(&pEvent);
      }
    }

    // --------------------------------------------------------------------
    // TODO: Here is where you poll your device/service for data and
    //       convert it to VSCP events. Example:
    //
    //   vscpEventEx ex;
    //   memset(&ex, 0, sizeof(ex));
    //   ex.vscp_class = VSCP_CLASS1_MEASUREMENT;
    //   ex.vscp_type  = VSCP_TYPE_MEASUREMENT_TEMPERATURE;
    //   memcpy(ex.GUID, pDrv->m_guid.getGUID(), 16);
    //   // ... set data/datasize, timestamp etc ...
    //   pDrv->addEvent2ReceiveQueue(ex);
    // --------------------------------------------------------------------
  }

  // TODO: Close the connection to your device/service here if you opened
  //       it in this thread.

  spdlog::info("[vscpl2drv-template] Worker thread terminated.");

#ifdef WIN32
  return 0;
#else
  return NULL;
#endif
}
