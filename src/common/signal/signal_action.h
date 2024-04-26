/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <csignal>
#include <list>
#include <vector>

#include "src/common/base/base.h"
#include "src/common/signal/fatal_handler.h"
#include "src/common/system/system.h"

namespace px {

// This implementation is based on Envoy's signal handler.

class SignalAction : public NotCopyable {
 public:
  SignalAction()
      : guard_size_(px::system::Config::GetInstance().PageSizeBytes()),
        altstack_size_(DetermineAltStackSize()) {
    MapAndProtectStackMemory();
    InstallSigHandlers();
  }
  ~SignalAction() {
    RemoveSigHandlers();
    UnmapStackMemory();
  }
  /**
   * The actual signal handler function with prototype matching signal.h
   */
  static void SigHandler(int sig, siginfo_t* info, void* context);

  /**
   * Add this handler to the list of functions which will be called on fatal signal
   */
  static void RegisterFatalErrorHandler(const FatalErrorHandlerInterface& handler);

 private:
  /**
   * Allocate this many bytes on each side of the area used for alt stack.
   *
   * Set to system page size.
   *
   * The memory will be protected from read and write.
   */
  const size_t guard_size_;
  /**
   * Use this many bytes for the alternate signal handling stack.
   *
   * Initialized as a multiple of page size (although signalstack will
   * do alignment as needed).
   *
   * Additionally, two guard pages will be allocated to bookend the usable area.
   */
  const size_t altstack_size_;
  /**
   * Determine the number of bytes to allocate to altstack_size_.
   * Makes sure that it is a multiple of the system page size.
   */
  size_t DetermineAltStackSize() const {
    // The size of altstack_ should be at least 4 * page size or size MINSIGSTKSZ if it is
    // greater. This size needs to be a multiple of the system page size.
    const size_t page_size = px::system::Config::GetInstance().PageSizeBytes();
    const size_t min_altstack_size = 4 * page_size;
    const size_t sig_page_count = IntRoundUpDivide(static_cast<size_t>(MINSIGSTKSZ), page_size);
    const size_t sig_stack_size = sig_page_count * page_size;

    return std::max(min_altstack_size, sig_stack_size);
  }
  /**
   * Signal handlers will be installed for these signals which have a fatal outcome.
   */
  static constexpr int kFatalSignals[] = {SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV};
  /**
   * Return the memory size we actually map including two guard pages.
   */
  size_t MapSizeWithGuards() const { return altstack_size_ + guard_size_ * 2; }
  /**
   * Install all signal handlers and setup signal handling stack.
   */
  void InstallSigHandlers();

  /**
   * Use mmap to map anonymous memory for the alternative stack.
   *
   * GUARD_SIZE on either end of the memory will be marked PROT_NONE, protected
   * from all access.
   */
  void MapAndProtectStackMemory();
  /**
   * Unmap alternative stack memory.
   */
  void UnmapStackMemory();

  void RemoveSigHandlers();

  char* altstack_{};
  std::array<struct sigaction, sizeof(kFatalSignals) / sizeof(int)> previous_handlers_;
  stack_t previous_altstack_;
  std::list<const FatalErrorHandlerInterface*> fatal_error_handlers_;
};

}  // namespace px
