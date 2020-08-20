// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/aligned_memory.h"

#include "base/logging.h"
#include "build/build_config.h"

#if defined(OS_ANDROID)
#include <malloc.h>
#endif
extern "C" void wait_for_continue();
namespace base {

void* AlignedAlloc(size_t size, size_t alignment) {
  DCHECK_GT(size, 0U);
  DCHECK_EQ(alignment & (alignment - 1), 0U);
  DCHECK_EQ(alignment % sizeof(void*), 0U);
  void* ptr = nullptr;
#if defined(COMPILER_MSVC)
  ptr = _aligned_malloc(size, alignment);
// Android technically supports posix_memalign(), but does not expose it in
// the current version of the library headers used by Chrome.  Luckily,
// memalign() on Android returns pointers which can safely be used with
// free(), so we can use it instead.  Issue filed to document this:
// http://code.google.com/p/android/issues/detail?id=35391
#elif defined(OS_ANDROID)
  ptr = memalign(alignment, size);
#elif defined(OS_GENODE)
  /* XXX: alignment not implemented yet, result address must be compatible with 'free()' */
  ptr = malloc(size);
  if ((reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) != 0U) {
    fprintf(stderr, "Warning: AlignedAlloc(): could not fulfill alignment request of %zu bytes\n", alignment);
    wait_for_continue();
  }
#else
  if (int ret = posix_memalign(&ptr, alignment, size)) {
    DLOG(ERROR) << "posix_memalign() returned with error " << ret;
    ptr = nullptr;
  }
#endif
  // Since aligned allocations may fail for non-memory related reasons, force a
  // crash if we encounter a failed allocation; maintaining consistent behavior
  // with a normal allocation failure in Chrome.
  if (!ptr) {
    DLOG(ERROR) << "If you crashed here, your aligned allocation is incorrect: "
                << "size=" << size << ", alignment=" << alignment;
    CHECK(false);
  }
  // Sanity check alignment just to be safe.
  DCHECK_EQ(reinterpret_cast<uintptr_t>(ptr) & (alignment - 1), 0U);
  return ptr;
}

}  // namespace base
