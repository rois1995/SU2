/*!
 * \file AllocationProbe.cpp
 * \brief Replacement of the global operator new/delete counting the requested bytes (see AllocationProbe.hpp).
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AllocationProbe.hpp"
#include "../../../Common/include/toolboxes/allocation_toolbox.hpp"

#if defined(__GLIBC__) && !defined(SU2_PROBE_PORTABLE_ONLY)
#include <malloc.h>
#endif

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <new>

namespace {

std::atomic<size_t> live{0}, peak{0}, liveUsable{0}, peakUsable{0}, calls{0};

size_t UsableBytes(void* pointer) {
#if defined(__GLIBC__) && !defined(SU2_PROBE_PORTABLE_ONLY)
  return malloc_usable_size(pointer);
#else
  (void)pointer;
  return 0;
#endif
}

void RaisePeak(std::atomic<size_t>& max, size_t value) {
  size_t current = max.load(std::memory_order_relaxed);
  while (value > current && !max.compare_exchange_weak(current, value, std::memory_order_relaxed)) {
  }
}

/*--- Block layout: [raw ... | size_t requested | size_t header | user data], header = max(16, alignment) bytes
 *    before the user pointer, so the user pointer keeps the requested alignment. ---*/
void* Allocate(size_t bytes, size_t alignment) {
  if (alignment < 16) alignment = 16;
  const size_t header = alignment;
  if (bytes > std::numeric_limits<size_t>::max() - header - (alignment - 1)) return nullptr;
  void* raw = MemoryAllocation::aligned_alloc<char>(alignment, bytes + header);
  if (raw == nullptr) return nullptr;
  char* user = static_cast<char*>(raw) + header;
  reinterpret_cast<size_t*>(user)[-1] = header;
  reinterpret_cast<size_t*>(user)[-2] = bytes;
  const size_t usable = UsableBytes(raw);
  RaisePeak(peak, live.fetch_add(bytes, std::memory_order_relaxed) + bytes);
  RaisePeak(peakUsable, liveUsable.fetch_add(usable, std::memory_order_relaxed) + usable);
  calls.fetch_add(1, std::memory_order_relaxed);
  return user;
}

void Release(void* pointer) {
  if (pointer == nullptr) return;
  char* user = static_cast<char*>(pointer);
  const size_t header = reinterpret_cast<size_t*>(user)[-1];
  const size_t bytes = reinterpret_cast<size_t*>(user)[-2];
  void* raw = user - header;
  live.fetch_sub(bytes, std::memory_order_relaxed);
  liveUsable.fetch_sub(UsableBytes(raw), std::memory_order_relaxed);
  MemoryAllocation::aligned_free(raw);
}

void* AllocateOrThrow(size_t bytes, size_t alignment) {
  void* pointer = Allocate(bytes, alignment);
  if (pointer == nullptr) throw std::bad_alloc();
  return pointer;
}

}  // namespace

namespace alloc_probe {
size_t Live() { return live.load(); }
size_t Peak() { return peak.load(); }
void ResetPeak() {
  peak.store(live.load());
  peakUsable.store(liveUsable.load());
}
size_t LiveUsable() { return liveUsable.load(); }
size_t PeakUsable() { return peakUsable.load(); }
bool HasUsable() {
#if defined(__GLIBC__) && !defined(SU2_PROBE_PORTABLE_ONLY)
  return true;
#else
  return false;
#endif
}
size_t Calls() { return calls.load(); }
}  // namespace alloc_probe

/*--- Every replaceable form of the global allocation functions. ---*/
void* operator new(std::size_t n) { return AllocateOrThrow(n, alignof(std::max_align_t)); }
void* operator new[](std::size_t n) { return AllocateOrThrow(n, alignof(std::max_align_t)); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return Allocate(n, alignof(std::max_align_t)); }
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept { return Allocate(n, alignof(std::max_align_t)); }
void* operator new(std::size_t n, std::align_val_t a) { return AllocateOrThrow(n, static_cast<size_t>(a)); }
void* operator new[](std::size_t n, std::align_val_t a) { return AllocateOrThrow(n, static_cast<size_t>(a)); }
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  return Allocate(n, static_cast<size_t>(a));
}
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  return Allocate(n, static_cast<size_t>(a));
}
void operator delete(void* p) noexcept { Release(p); }
void operator delete[](void* p) noexcept { Release(p); }
void operator delete(void* p, std::size_t) noexcept { Release(p); }
void operator delete[](void* p, std::size_t) noexcept { Release(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { Release(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { Release(p); }
void operator delete(void* p, std::align_val_t) noexcept { Release(p); }
void operator delete[](void* p, std::align_val_t) noexcept { Release(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { Release(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { Release(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { Release(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { Release(p); }
