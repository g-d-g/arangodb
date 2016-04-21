////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
////////////////////////////////////////////////////////////////////////////////

#include "DispatcherFeature.h"

#include "Dispatcher/Dispatcher.h"
#include "ProgramOptions/ProgramOptions.h"
#include "ProgramOptions/Section.h"
#include "Scheduler/SchedulerFeature.h"
#include "V8Server/V8DealerFeature.h"
#include "V8Server/v8-dispatcher.h"

using namespace arangodb;
using namespace arangodb::application_features;
using namespace arangodb::basics;
using namespace arangodb::options;
using namespace arangodb::rest;

Dispatcher* DispatcherFeature::DISPATCHER = nullptr;

DispatcherFeature::DispatcherFeature(
    application_features::ApplicationServer* server)
    : ApplicationFeature(server, "Dispatcher"),
      _nrStandardThreads(0),
      _nrAqlThreads(0),
      _queueSize(16384),
      _dispatcher(nullptr) {
  setOptional(true);
  requiresElevatedPrivileges(false);
  startsAfter("Database");
  startsAfter("FileDescriptors");
  startsAfter("Logger");
  startsAfter("Scheduler");
  startsAfter("WorkMonitor");
}

DispatcherFeature::~DispatcherFeature() {
  delete _dispatcher;
}

void DispatcherFeature::collectOptions(
    std::shared_ptr<ProgramOptions> options) {
  LOG_TOPIC(TRACE, Logger::STARTUP) << name() << "::collectOptions";

  options->addSection("server", "Server features");

  options->addOption("--server.threads",
                     "number of threads for basic operations",
                     new UInt64Parameter(&_nrStandardThreads));

  options->addHiddenOption("--server.aql-threads",
                           "number of threads for basic operations",
                           new UInt64Parameter(&_nrAqlThreads));

  options->addHiddenOption("--server.maximal-queue-size",
                           "maximum queue length for asynchronous operations",
                           new UInt64Parameter(&_queueSize));
}

void DispatcherFeature::validateOptions(std::shared_ptr<ProgramOptions>) {
  LOG_TOPIC(TRACE, Logger::STARTUP) << name() << "::validateOptions";

  if (_nrStandardThreads == 0) {
    size_t n = TRI_numberProcessors();

    if (n <= 4) {
      _nrStandardThreads = n - 1;
    } else {
      _nrStandardThreads = n - 2;
    }
  }

  if (_nrAqlThreads == 0) {
    _nrAqlThreads = _nrStandardThreads;
  }

  if (_queueSize <= 128) {
    LOG(FATAL)
        << "invalid value for `--server.maximal-queue-size', need at least 128";
    FATAL_ERROR_EXIT();
  }
}

void DispatcherFeature::prepare() {
  V8DealerFeature* dealer = dynamic_cast<V8DealerFeature*>(
      ApplicationServer::lookupFeature("V8Dealer"));

  if (dealer != nullptr) {
    dealer->defineDouble("DISPATCHER_THREADS", _nrStandardThreads);
  }
}

void DispatcherFeature::start() {
  LOG_TOPIC(TRACE, Logger::STARTUP) << name() << "::start";

  buildDispatcher();
  buildStandardQueue();

  V8DealerFeature* dealer = dynamic_cast<V8DealerFeature*>(
      ApplicationServer::lookupFeature("V8Dealer"));

  if (dealer != nullptr) {
    dealer->defineContextUpdate(
        [](v8::Isolate* isolate, v8::Handle<v8::Context> context, size_t) {
          TRI_InitV8Dispatcher(isolate, context);
        },
        nullptr);
  }
}

void DispatcherFeature::beginShutdown() {
  LOG_TOPIC(TRACE, Logger::STARTUP) << name() << "::beginShutdown";

  _dispatcher->beginShutdown();
}

void DispatcherFeature::stop() {
  LOG_TOPIC(TRACE, Logger::STARTUP) << name() << "::stop";

  _dispatcher->shutdown();

  DISPATCHER = nullptr;
}

void DispatcherFeature::buildDispatcher() {
  _dispatcher = new Dispatcher(SchedulerFeature::SCHEDULER);
  DISPATCHER = _dispatcher;
}

void DispatcherFeature::buildStandardQueue() {
  LOG_TOPIC(DEBUG, Logger::STARTUP) << "setting up a standard queue with "
                                    << _nrStandardThreads << " threads";

  _dispatcher->addStandardQueue(_nrStandardThreads, _queueSize);
}

void DispatcherFeature::buildAqlQueue() {
  LOG_TOPIC(DEBUG, Logger::STARTUP) << "setting up the AQL standard queue with "
                                    << _nrAqlThreads << " threads";

  _dispatcher->addAQLQueue(_nrAqlThreads, _queueSize);
}

void DispatcherFeature::setProcessorAffinity(std::vector<size_t> const& cores) {
#ifdef TRI_HAVE_THREAD_AFFINITY
  _dispatcher->setProcessorAffinity(Dispatcher::STANDARD_QUEUE, cores);
#endif
}