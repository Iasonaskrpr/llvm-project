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

static ConstString
CreateFortranFunctionName(ConstString func_name,
                          const llvm::SmallVectorImpl<CompilerType> &parameters,
                          CompilerType return_type) {
  std::string name_buffer;
  llvm::raw_string_ostream name_stream(name_buffer);

  if (return_type.IsValid())
    name_stream << return_type.GetTypeName().AsCString("") << " ";

  name_stream << func_name.AsCString("<unnamed function>") << "(";
  for (auto &parameter : parameters) {
    if (parameter != parameters.front())
      name_stream << ", ";
    name_stream << parameter.GetTypeName().AsCString("");
  }
  name_stream << ")";
  name_stream.flush();
  return ConstString(name_buffer);
}

FortranFunction::FortranFunction(
    ConstString func_name,
    const llvm::SmallVectorImpl<CompilerType> &parameters,
    CompilerType return_type)
    : FortranType(
          FortranType::KIND_FUNCTION, 0,
          CreateFortranFunctionName(func_name, parameters, return_type)),
      m_parameters(parameters.begin(), parameters.end()),
      m_return_type(return_type) {}