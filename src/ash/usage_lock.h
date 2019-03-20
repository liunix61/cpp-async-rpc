/// \file
/// \brief Reference-counting construct to for quiescent-state cleanup.
///
/// \copyright
///   Copyright 2018 by Google Inc. All Rights Reserved.
///
/// \copyright
///   Licensed under the Apache License, Version 2.0 (the "License"); you may
///   not use this channel except in compliance with the License. You may obtain
///   a copy of the License at
///
/// \copyright
///   http://www.apache.org/licenses/LICENSE-2.0
///
/// \copyright
///   Unless required by applicable law or agreed to in writing, software
///   distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
///   WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
///   License for the specific language governing permissions and limitations
///   under the License.

#ifndef ASH_USAGE_LOCK_H_
#define ASH_USAGE_LOCK_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string_view>
#include "ash/errors.h"

namespace ash {

template <typename T, typename E>
class usage_lock {
 public:
  explicit usage_lock(const char* exception_message = "")
      : message_(exception_message) {}

  void arm(T* ptr) {
    std::scoped_lock lock(mu_);
    ptr_.reset(ptr, [this](T*) {
      {
        std::scoped_lock lock(mu_);
        done_ = true;
      }
      done_cond_.notify_all();
    });
    done_ = false;
  }

  void drop() {
    std::unique_lock lock(mu_);
    ptr_.reset();
    done_cond_.wait(lock, [this]() { return done_; });
  }

  auto get() {
    auto res = ptr_;
    if (!res) {
      throw E(message_);
    }
    return res;
  }

  auto get_or_null() { return ptr_; }

 private:
  const char* const message_;
  std::shared_ptr<T> ptr_;
  std::mutex mu_;
  std::condition_variable done_cond_;
  bool done_ = true;
};

}  // namespace ash

#endif  // ASH_USAGE_LOCK_H_
