// Copyright 2022 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_MAGLEV_MAGLEV_REGALLOC_DATA_H_
#define V8_MAGLEV_MAGLEV_REGALLOC_DATA_H_

#include "src/base/pointer-with-payload.h"
#include "src/codegen/register.h"
#include "src/compiler/backend/instruction.h"

namespace v8 {
namespace internal {
namespace maglev {

class ValueNode;

#define COUNT(V) +1
static constexpr int kAllocatableGeneralRegisterCount =
    ALLOCATABLE_GENERAL_REGISTERS(COUNT);
#undef COUNT

constexpr uint8_t MapRegisterToIndex(Register r) {
  uint8_t count = 0;
#define EMIT_BRANCH(V)      \
  if (r == V) return count; \
  count++;
  ALLOCATABLE_GENERAL_REGISTERS(EMIT_BRANCH)
#undef EMIT_BRANCH
  // TODO(v8:7700): Re-enable UNREACHABLE once we figure out how to to avoid
  // the gcc error 'call to non-constexpr function'.
  // UNREACHABLE();
  return 255;
}

constexpr Register MapIndexToRegister(int i) {
  uint8_t count = 0;
#define EMIT_BRANCH(V)      \
  if (i == count) return V; \
  count++;
  ALLOCATABLE_GENERAL_REGISTERS(EMIT_BRANCH)
#undef EMIT_BRANCH
  // TODO(v8:7700): Re-enable UNREACHABLE once we figure out how to to avoid
  // the gcc error 'call to non-constexpr function'.
  // UNREACHABLE();
  return no_reg;
}

struct RegisterStateFlags {
  // TODO(v8:7700): Use the good old Flags mechanism.
  static constexpr int kIsMergeShift = 0;
  static constexpr int kIsInitializedShift = 1;

  const bool is_initialized = false;
  const bool is_merge = false;

  explicit constexpr operator uintptr_t() const {
    return (is_initialized ? 1 << kIsInitializedShift : 0) |
           (is_merge ? 1 << kIsMergeShift : 0);
  }
  constexpr explicit RegisterStateFlags(uintptr_t state)
      : is_initialized((state & (1 << kIsInitializedShift)) != 0),
        is_merge((state & (1 << kIsMergeShift)) != 0) {}
  constexpr RegisterStateFlags(bool is_initialized, bool is_merge)
      : is_initialized(is_initialized), is_merge(is_merge) {}
};
constexpr bool operator==(const RegisterStateFlags& left,
                          const RegisterStateFlags& right) {
  return left.is_initialized == right.is_initialized &&
         left.is_merge == right.is_merge;
}

typedef base::PointerWithPayload<void, RegisterStateFlags, 2> RegisterState;

struct RegisterMerge {
  compiler::AllocatedOperand* operands() {
    return reinterpret_cast<compiler::AllocatedOperand*>(this + 1);
  }
  compiler::AllocatedOperand& operand(size_t i) { return operands()[i]; }

  ValueNode* node;
};

inline bool LoadMergeState(RegisterState state, RegisterMerge** merge) {
  DCHECK(state.GetPayload().is_initialized);
  if (state.GetPayload().is_merge) {
    *merge = static_cast<RegisterMerge*>(state.GetPointer());
    return true;
  }
  *merge = nullptr;
  return false;
}

inline bool LoadMergeState(RegisterState state, ValueNode** node,
                           RegisterMerge** merge) {
  DCHECK(state.GetPayload().is_initialized);
  if (LoadMergeState(state, merge)) {
    *node = (*merge)->node;
    return true;
  }
  *node = static_cast<ValueNode*>(state.GetPointer());
  return false;
}

}  // namespace maglev
}  // namespace internal
}  // namespace v8

#endif  // V8_MAGLEV_MAGLEV_REGALLOC_DATA_H_
