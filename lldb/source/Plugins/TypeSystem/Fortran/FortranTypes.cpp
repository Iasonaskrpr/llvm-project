//===-- FortranTypes.cpp ----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FortranTypes.h"

using namespace lldb_private;

namespace lldb_private {
namespace plugin {
namespace fortran {

FortranType::~FortranType() = default;

FortranFunction::FortranFunction(
    ConstString func_name,
    const llvm::SmallVectorImpl<CompilerType> &parameters)
    : FortranType(FortranType::KIND_FUNCTION, 0, func_name) {
  m_parameters.assign(parameters.begin(), parameters.end());
}

FortranArray::FortranArray(CompilerType element_type,
                           const llvm::SmallVectorImpl<ArrayShape> &dimensions,
                           ConstString array_type_name,
                           uint64_t total_array_size, bool is_allocatable,
                           bool is_dynamic, bool is_star, bool is_auto,
                           bool is_assumed_rank, uint64_t total_elements,
                           DWARFExpressionList allocated_exp,
                           DWARFExpressionList data_location_exp,
                           DWARFExpressionList rank_exp)
    : FortranType(TypeKind::KIND_ARRAY, total_array_size, array_type_name),
      m_element_type(element_type),
      m_dimensions(dimensions.begin(), dimensions.end()),
      m_is_allocatable(is_allocatable), m_is_dynamic(is_dynamic),
      m_is_star(is_star), m_is_auto(is_auto),
      m_is_assumed_rank(is_assumed_rank), m_total_elements(total_elements),
      m_allocated_exp(allocated_exp), m_data_location_exp(data_location_exp),
      m_rank_exp(rank_exp) {}

uint64_t FortranArray::GetElementByteSize() const {
  auto byte_size_or_err = m_element_type.GetByteSize(nullptr);
  // TODO: Change this to returning an error, and change return type to
  // expected<uint64_t>
  if (!byte_size_or_err)
    return 0;
  return *byte_size_or_err;
}

ConstString CreateArrayTypeName(const CompilerType &element_type,
                                const llvm::ArrayRef<ArrayShape> shapes,
                                bool is_allocatable, bool is_star,
                                bool is_assumed_rank) {

  std::string name_buffer;
  llvm::raw_string_ostream name_stream(name_buffer);

  name_stream << element_type.GetTypeName().AsCString(nullptr) << "(";
  size_t rank = shapes.size();
  if (is_assumed_rank) {
    name_stream << "..)";
    name_stream.flush();
    return ConstString(name_buffer.c_str());
  }

  for (size_t idx = 0; idx < rank; ++idx) {
    if (idx > 0)
      name_stream << ", ";

    const ArrayBound &lb = shapes[idx].GetLowerBound();
    const ArrayBound &ub = shapes[idx].GetUpperBound();

    if (is_star && idx == rank - 1) {
      if (lb.IsExplicit() && lb.GetBound() != 1)
        name_stream << lb.GetBound() << ":";
      name_stream << "*";
    } else if (ub.IsColon()) {
      // Unknown bound elements
      name_stream << ":";
    } else if (ub.IsExplicit()) {
      // Explicit bounds
      if (lb.GetBound() != 1) {
        name_stream << lb.GetBound() << ":";
      }
      name_stream << ub.GetBound();
    }
  }

  name_stream << ")";

  if (is_allocatable) {
    name_stream << ", allocatable";
  }

  name_stream.flush();
  return ConstString(name_buffer.c_str());
}

} // namespace fortran
} // namespace plugin
} // namespace lldb_private