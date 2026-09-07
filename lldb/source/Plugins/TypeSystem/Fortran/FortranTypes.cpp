//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements the classes that describe the Fortran types.
///
//===----------------------------------------------------------------------===//

#include "FortranTypes.h"

using namespace lldb_private;
using namespace lldb_private::plugin::fortran;

char FortranType::ID;

FortranType::~FortranType() = default;

static ConstString CreateFortranFunctionName(
    ConstString func_name,
    const llvm::SmallVectorImpl<CompilerType> &parameters,
    const llvm::SmallVectorImpl<llvm::StringRef> &parameter_names,
    CompilerType return_type) {
  std::string name_buffer;
  llvm::raw_string_ostream name_stream(name_buffer);

  if (return_type.IsValid())
    name_stream << return_type.GetTypeName().AsCString("") << " ";

  name_stream << func_name.AsCString("<unnamed function>") << "(";
  for (size_t idx = 0; idx < parameters.size(); idx++) {
    if (idx != 0)
      name_stream << ", ";
    name_stream << parameters[idx].GetTypeName().AsCString("") << " ";
    name_stream << parameter_names[idx].str();
  }
  name_stream << ")";
  name_stream.flush();
  return ConstString(name_buffer);
}

FortranFunction::FortranFunction(
    ConstString func_name,
    const llvm::SmallVectorImpl<CompilerType> &parameters,
    const llvm::SmallVectorImpl<llvm::StringRef> &parameter_names,
    CompilerType return_type)
    : FortranType(FortranType::KIND_FUNCTION, 0,
                  CreateFortranFunctionName(func_name, parameters,
                                            parameter_names, return_type)),
      m_parameters(parameters.begin(), parameters.end()),
      m_return_type(return_type) {}

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
