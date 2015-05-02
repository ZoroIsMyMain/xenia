/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_HIR_VALUE_H_
#define XENIA_HIR_VALUE_H_

#include "poly/arena.h"
#include "poly/assert.h"
#include "poly/vec128.h"
#include "xenia/cpu/backend/machine_info.h"
#include "xenia/cpu/hir/opcodes.h"

namespace xe {
namespace cpu {
namespace hir {

class Instr;

using vec128_t = poly::vec128_t;

enum TypeName {
  // Many tables rely on this ordering.
  INT8_TYPE = 0,
  INT16_TYPE = 1,
  INT32_TYPE = 2,
  INT64_TYPE = 3,
  FLOAT32_TYPE = 4,
  FLOAT64_TYPE = 5,
  VEC128_TYPE = 6,
  MAX_TYPENAME,
};

inline size_t GetTypeSize(TypeName type_name) {
  switch (type_name) {
    case INT8_TYPE:
      return 1;
    case INT16_TYPE:
      return 2;
    case INT32_TYPE:
      return 4;
    case INT64_TYPE:
      return 8;
    case FLOAT32_TYPE:
      return 4;
    case FLOAT64_TYPE:
      return 8;
    case VEC128_TYPE:
      return 16;
    default:
      assert_unhandled_case(type_name);
      return 0;
  }
}

enum ValueFlags {
  VALUE_IS_CONSTANT = (1 << 1),
  VALUE_IS_ALLOCATED = (1 << 2),  // Used by backends. Do not set.
};

struct RegAssignment {
  const backend::MachineInfo::RegisterSet* set;
  int32_t index;
};

class Value {
 public:
  typedef struct Use_s {
    Instr* instr;
    Use_s* prev;
    Use_s* next;
  } Use;
  typedef union {
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    vec128_t v128;
  } ConstantValue;

 public:
  uint32_t ordinal;
  TypeName type;

  uint32_t flags;
  RegAssignment reg;
  ConstantValue constant;

  Instr* def;
  Use* use_head;
  // NOTE: for performance reasons this is not maintained during construction.
  Instr* last_use;
  Value* local_slot;

  // TODO(benvanik): remove to shrink size.
  void* tag;

  Use* AddUse(poly::Arena* arena, Instr* instr);
  void RemoveUse(Use* use);

  int8_t get_constant(int8_t) const { return constant.i8; }
  int16_t get_constant(int16_t) const { return constant.i16; }
  int32_t get_constant(int32_t) const { return constant.i32; }
  int64_t get_constant(int64_t) const { return constant.i64; }
  float get_constant(float) const { return constant.f32; }
  double get_constant(double) const { return constant.f64; }
  vec128_t get_constant(vec128_t&) const { return constant.v128; }

  void set_zero(TypeName type) {
    this->type = type;
    flags |= VALUE_IS_CONSTANT;
    constant.v128.low = constant.v128.high = 0;
  }
  void set_constant(int8_t value) {
    type = INT8_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.i8 = value;
  }
  void set_constant(uint8_t value) {
    type = INT8_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.i8 = value;
  }
  void set_constant(int16_t value) {
    type = INT16_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.i16 = value;
  }
  void set_constant(uint16_t value) {
    type = INT16_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.i16 = value;
  }
  void set_constant(int32_t value) {
    type = INT32_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.i32 = value;
  }
  void set_constant(uint32_t value) {
    type = INT32_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.i32 = value;
  }
  void set_constant(int64_t value) {
    type = INT64_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.i64 = value;
  }
  void set_constant(uint64_t value) {
    type = INT64_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.i64 = value;
  }
  void set_constant(float value) {
    type = FLOAT32_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.f32 = value;
  }
  void set_constant(double value) {
    type = FLOAT64_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.f64 = value;
  }
  void set_constant(const vec128_t& value) {
    type = VEC128_TYPE;
    flags |= VALUE_IS_CONSTANT;
    constant.v128 = value;
  }
  void set_from(const Value* other) {
    type = other->type;
    flags = other->flags;
    constant.v128 = other->constant.v128;
  }

  inline bool IsConstant() const { return !!(flags & VALUE_IS_CONSTANT); }
  bool IsConstantTrue() const {
    if (type == VEC128_TYPE) {
      assert_always();
    }
    return (flags & VALUE_IS_CONSTANT) && !!constant.i64;
  }
  bool IsConstantFalse() const {
    if (type == VEC128_TYPE) {
      assert_always();
    }
    return (flags & VALUE_IS_CONSTANT) && !constant.i64;
  }
  bool IsConstantZero() const {
    if (type == VEC128_TYPE) {
      return (flags & VALUE_IS_CONSTANT) && !constant.v128.low &&
             !constant.v128.high;
    }
    return (flags & VALUE_IS_CONSTANT) && !constant.i64;
  }
  bool IsConstantEQ(Value* other) const {
    if (type == VEC128_TYPE) {
      assert_always();
    }
    return (flags & VALUE_IS_CONSTANT) && (other->flags & VALUE_IS_CONSTANT) &&
           constant.i64 == other->constant.i64;
  }
  bool IsConstantNE(Value* other) const {
    if (type == VEC128_TYPE) {
      assert_always();
    }
    return (flags & VALUE_IS_CONSTANT) && (other->flags & VALUE_IS_CONSTANT) &&
           constant.i64 != other->constant.i64;
  }
  bool IsConstantSLT(Value* other) const {
    assert_true(flags & VALUE_IS_CONSTANT && other->flags & VALUE_IS_CONSTANT);
    switch (type) {
      case INT8_TYPE:
        return constant.i8 < other->constant.i8;
      case INT16_TYPE:
        return constant.i16 < other->constant.i16;
      case INT32_TYPE:
        return constant.i32 < other->constant.i32;
      case INT64_TYPE:
        return constant.i64 < other->constant.i64;
      case FLOAT32_TYPE:
        return constant.f32 < other->constant.f32;
      case FLOAT64_TYPE:
        return constant.f64 < other->constant.f64;
      default:
        assert_always();
        return false;
    }
  }
  bool IsConstantSLE(Value* other) const {
    assert_true(flags & VALUE_IS_CONSTANT && other->flags & VALUE_IS_CONSTANT);
    switch (type) {
      case INT8_TYPE:
        return constant.i8 <= other->constant.i8;
      case INT16_TYPE:
        return constant.i16 <= other->constant.i16;
      case INT32_TYPE:
        return constant.i32 <= other->constant.i32;
      case INT64_TYPE:
        return constant.i64 <= other->constant.i64;
      case FLOAT32_TYPE:
        return constant.f32 <= other->constant.f32;
      case FLOAT64_TYPE:
        return constant.f64 <= other->constant.f64;
      default:
        assert_always();
        return false;
    }
  }
  bool IsConstantSGT(Value* other) const {
    assert_true(flags & VALUE_IS_CONSTANT && other->flags & VALUE_IS_CONSTANT);
    switch (type) {
      case INT8_TYPE:
        return constant.i8 > other->constant.i8;
      case INT16_TYPE:
        return constant.i16 > other->constant.i16;
      case INT32_TYPE:
        return constant.i32 > other->constant.i32;
      case INT64_TYPE:
        return constant.i64 > other->constant.i64;
      case FLOAT32_TYPE:
        return constant.f32 > other->constant.f32;
      case FLOAT64_TYPE:
        return constant.f64 > other->constant.f64;
      default:
        assert_always();
        return false;
    }
  }
  bool IsConstantSGE(Value* other) const {
    assert_true(flags & VALUE_IS_CONSTANT && other->flags & VALUE_IS_CONSTANT);
    switch (type) {
      case INT8_TYPE:
        return constant.i8 >= other->constant.i8;
      case INT16_TYPE:
        return constant.i16 >= other->constant.i16;
      case INT32_TYPE:
        return constant.i32 >= other->constant.i32;
      case INT64_TYPE:
        return constant.i64 >= other->constant.i64;
      case FLOAT32_TYPE:
        return constant.f32 >= other->constant.f32;
      case FLOAT64_TYPE:
        return constant.f64 >= other->constant.f64;
      default:
        assert_always();
        return false;
    }
  }
  bool IsConstantULT(Value* other) const {
    assert_true(flags & VALUE_IS_CONSTANT && other->flags & VALUE_IS_CONSTANT);
    switch (type) {
      case INT8_TYPE:
        return (uint8_t)constant.i8 < (uint8_t)other->constant.i8;
      case INT16_TYPE:
        return (uint16_t)constant.i16 < (uint16_t)other->constant.i16;
      case INT32_TYPE:
        return (uint32_t)constant.i32 < (uint32_t)other->constant.i32;
      case INT64_TYPE:
        return (uint64_t)constant.i64 < (uint64_t)other->constant.i64;
      case FLOAT32_TYPE:
        return constant.f32 < other->constant.f32;
      case FLOAT64_TYPE:
        return constant.f64 < other->constant.f64;
      default:
        assert_always();
        return false;
    }
  }
  bool IsConstantULE(Value* other) const {
    assert_true(flags & VALUE_IS_CONSTANT && other->flags & VALUE_IS_CONSTANT);
    switch (type) {
      case INT8_TYPE:
        return (uint8_t)constant.i8 <= (uint8_t)other->constant.i8;
      case INT16_TYPE:
        return (uint16_t)constant.i16 <= (uint16_t)other->constant.i16;
      case INT32_TYPE:
        return (uint32_t)constant.i32 <= (uint32_t)other->constant.i32;
      case INT64_TYPE:
        return (uint64_t)constant.i64 <= (uint64_t)other->constant.i64;
      case FLOAT32_TYPE:
        return constant.f32 <= other->constant.f32;
      case FLOAT64_TYPE:
        return constant.f64 <= other->constant.f64;
      default:
        assert_always();
        return false;
    }
  }
  bool IsConstantUGT(Value* other) const {
    assert_true(flags & VALUE_IS_CONSTANT && other->flags & VALUE_IS_CONSTANT);
    switch (type) {
      case INT8_TYPE:
        return (uint8_t)constant.i8 > (uint8_t)other->constant.i8;
      case INT16_TYPE:
        return (uint16_t)constant.i16 > (uint16_t)other->constant.i16;
      case INT32_TYPE:
        return (uint32_t)constant.i32 > (uint32_t)other->constant.i32;
      case INT64_TYPE:
        return (uint64_t)constant.i64 > (uint64_t)other->constant.i64;
      case FLOAT32_TYPE:
        return constant.f32 > other->constant.f32;
      case FLOAT64_TYPE:
        return constant.f64 > other->constant.f64;
      default:
        assert_always();
        return false;
    }
  }
  bool IsConstantUGE(Value* other) const {
    assert_true(flags & VALUE_IS_CONSTANT && other->flags & VALUE_IS_CONSTANT);
    switch (type) {
      case INT8_TYPE:
        return (uint8_t)constant.i8 >= (uint8_t)other->constant.i8;
      case INT16_TYPE:
        return (uint16_t)constant.i16 >= (uint16_t)other->constant.i16;
      case INT32_TYPE:
        return (uint32_t)constant.i32 >= (uint32_t)other->constant.i32;
      case INT64_TYPE:
        return (uint64_t)constant.i64 >= (uint64_t)other->constant.i64;
      case FLOAT32_TYPE:
        return constant.f32 >= other->constant.f32;
      case FLOAT64_TYPE:
        return constant.f64 >= other->constant.f64;
      default:
        assert_always();
        return false;
    }
  }
  uint32_t AsUint32();
  uint64_t AsUint64();

  void Cast(TypeName target_type);
  void ZeroExtend(TypeName target_type);
  void SignExtend(TypeName target_type);
  void Truncate(TypeName target_type);
  void Convert(TypeName target_type, RoundMode round_mode);
  void Round(RoundMode round_mode);
  bool Add(Value* other);
  bool Sub(Value* other);
  void Mul(Value* other);
  void Div(Value* other);
  static void MulAdd(Value* dest, Value* value1, Value* value2, Value* value3);
  static void MulSub(Value* dest, Value* value1, Value* value2, Value* value3);
  void Neg();
  void Abs();
  void Sqrt();
  void RSqrt();
  void And(Value* other);
  void Or(Value* other);
  void Xor(Value* other);
  void Not();
  void Shl(Value* other);
  void Shr(Value* other);
  void Sha(Value* other);
  void ByteSwap();
  void CountLeadingZeros(const Value* other);
  bool Compare(Opcode opcode, Value* other);
};

}  // namespace hir
}  // namespace cpu
}  // namespace xe

#endif  // XENIA_HIR_VALUE_H_
