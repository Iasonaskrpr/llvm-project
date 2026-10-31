//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TypeSystemFortran.h"
#include "FortranTypes.h"

#include "Plugins/SymbolFile/DWARF/DWARFASTParserFortran.h"

#include "lldb/Core/DumpDataExtractor.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Symbol/SymbolFile.h"
#include "lldb/Target/Language.h"
#include "lldb/Target/Target.h"

using namespace lldb;
using namespace lldb_private;
using namespace llvm;
using namespace lldb_private::plugin::dwarf;
using namespace lldb_private::plugin::fortran;

LLDB_PLUGIN_DEFINE(TypeSystemFortran)

static bool DumpComplex(Stream &s, const lldb_private::DataExtractor &data,
                        lldb::offset_t &offset, size_t data_byte_size) {
  if (sizeof(float) * 2 == data_byte_size) {
    float f32_1 = data.GetFloat(&offset);
    float f32_2 = data.GetFloat(&offset);

    s.Printf("(%g, %g)", f32_1, f32_2);
  } else if (sizeof(double) * 2 == data_byte_size) {
    double d64_1 = data.GetDouble(&offset);
    double d64_2 = data.GetDouble(&offset);

    s.Printf("(%lg, %lg)", d64_1, d64_2);
  } else if (sizeof(long double) * 2 == data_byte_size) {
    long double ld64_1 = data.GetLongDouble(&offset);
    long double ld64_2 = data.GetLongDouble(&offset);
    s.Printf("(%Lg, %Lg)", ld64_1, ld64_2);
  } else {
    s.Printf("error: unsupported byte size (%lu"
             ") for complex float format",
             data_byte_size);
    return false;
  }
  return true;
}

char TypeSystemFortran::ID;

TypeSystemFortran::~TypeSystemFortran() = default;
TypeSystemFortran::TypeSystemFortran() = default;

void TypeSystemFortran::Initialize() {
  PluginManager::RegisterPlugin(GetPluginNameStatic(), "fortran plug-in",
                                CreateInstance, GetSupportedLanguagesForTypes(),
                                GetSupportedLanguagesForExpressions());
}

void TypeSystemFortran::Terminate() {
  PluginManager::UnregisterPlugin(CreateInstance);
}

plugin::dwarf::DWARFASTParser *TypeSystemFortran::GetDWARFParser() {
  if (!m_dwarf_ast_parser_up)
    m_dwarf_ast_parser_up = std::make_unique<DWARFASTParserFortran>(*this);
  return m_dwarf_ast_parser_up.get();
}

TypeSystemSP TypeSystemFortran::CreateInstance(LanguageType language,
                                               Module *module, Target *target) {
  if (!Language::LanguageIsFortran(language))
    return TypeSystemSP();

  auto type_system_sp = std::make_shared<TypeSystemFortran>();

  // Get the byte order from the target or module and store it
  if (target)
    type_system_sp->SetByteOrder(target->GetArchitecture().GetByteOrder());
  else if (module)
    type_system_sp->SetByteOrder(module->GetArchitecture().GetByteOrder());

  return type_system_sp;
}

LanguageSet TypeSystemFortran::GetSupportedLanguagesForTypes() {
  LanguageSet languages;
  languages.Insert(eLanguageTypeFortran77);
  languages.Insert(eLanguageTypeFortran90);
  languages.Insert(eLanguageTypeFortran95);
  languages.Insert(eLanguageTypeFortran03);
  languages.Insert(eLanguageTypeFortran08);
  languages.Insert(eLanguageTypeFortran18);
  return languages;
}

LanguageSet TypeSystemFortran::GetSupportedLanguagesForExpressions() {
  return GetSupportedLanguagesForTypes();
}

#ifndef NDEBUG
bool TypeSystemFortran::Verify(lldb::opaque_compiler_type_t type) {
  return !type || llvm::isa<FortranType>(static_cast<FortranType *>(type));
}
#endif

bool TypeSystemFortran::IsFloatingPointType(opaque_compiler_type_t type) {
  if (!type)
    return false;

  int kind = static_cast<FortranType *>(type)->GetKind();
  if (kind == FortranType::KIND_REAL || kind == FortranType::KIND_COMPLEX)
    return true;

  return false;
}

bool TypeSystemFortran::IsFunctionType(opaque_compiler_type_t type) {
  if (!type)
    return false;

  return static_cast<FortranType *>(type)->GetKind() ==
         FortranType::KIND_FUNCTION;
}

size_t TypeSystemFortran::GetNumberOfFunctionArguments(
    lldb::opaque_compiler_type_t type) {
  FortranFunction *function_type = FortranFunction::GetFunctionType(type);
  if (!function_type)
    return 0;

  return function_type->GetNumParams();
}

CompilerType
TypeSystemFortran::GetFunctionArgumentAtIndex(lldb::opaque_compiler_type_t type,
                                              const size_t index) {
  FortranFunction *function_type = FortranFunction::GetFunctionType(type);
  if (!function_type)
    return CompilerType();

  llvm::ArrayRef<lldb_private::CompilerType> param_types =
      function_type->GetParamTypes();
  if (index >= param_types.size())
    return CompilerType();

  return param_types[index];
}

bool TypeSystemFortran::IsIntegerType(opaque_compiler_type_t type,
                                      bool &is_signed) {
  if (!type)
    return false;

  int kind = static_cast<FortranType *>(type)->GetKind();
  if (kind == FortranType::KIND_INTEGER || kind == FortranType::KIND_UNSIGNED) {
    is_signed = (kind == FortranType::KIND_INTEGER);
    return true;
  }

  return false;
}

bool TypeSystemFortran::SupportsLanguage(lldb::LanguageType language) {
  return Language::LanguageIsFortran(language);
}

/// Returns the type name upper-cased to follow Fortran's general style.
ConstString TypeSystemFortran::GetTypeName(opaque_compiler_type_t type,
                                           bool base_only) {
  if (!type)
    return ConstString();

  FortranType *fortran_type = static_cast<FortranType *>(type);
  switch (fortran_type->GetKind()) {
  case FortranType::KIND_INTEGER:
  case FortranType::KIND_LOGICAL:
  case FortranType::KIND_REAL:
  case FortranType::KIND_COMPLEX:
  case FortranType::KIND_UNSIGNED:
    return fortran_type->GetName();
  case FortranType::KIND_FUNCTION:
    return static_cast<FortranFunction *>(fortran_type)->GetSignature();
  case FortranType::KIND_UNKNOWN:
    return ConstString("Unsupported");
  }
  return ConstString("Unsupported");
}

uint32_t
TypeSystemFortran::GetTypeInfo(opaque_compiler_type_t type,
                               CompilerType *pointee_or_element_compiler_type) {
  if (!type)
    return 0;

  FortranType *base_type = static_cast<FortranType *>(type);
  uint32_t builtin_type_flags = eTypeIsBuiltIn | eTypeHasValue | eTypeIsScalar;

  switch (base_type->GetKind()) {
  case FortranType::KIND_REAL:
    return builtin_type_flags | eTypeIsFloat;
  case FortranType::KIND_INTEGER:
    return builtin_type_flags | eTypeIsInteger | eTypeIsSigned;
  case FortranType::KIND_LOGICAL:
    return builtin_type_flags | eTypeIsFloat;
  case FortranType::KIND_COMPLEX:
    return builtin_type_flags | eTypeIsComplex;
  case FortranType::KIND_FUNCTION:
    return eTypeIsFuncPrototype;
  default:
    break;
  }

  return 0;
}

CompilerType
TypeSystemFortran::CreateBaseType(llvm::dwarf::TypeKind dwarf_encoding,
                                  uint64_t bitsize, ConstString name) {
  int underlying_kind = FortranType::KIND_UNKNOWN;
  const char *default_name = nullptr;

  switch (dwarf_encoding) {
  case dwarf::DW_ATE_boolean:
    underlying_kind = FortranType::KIND_LOGICAL;
    default_name = "LOGICAL";
    break;
  case dwarf::DW_ATE_float:
    underlying_kind = FortranType::KIND_REAL;
    default_name = "REAL";
    break;
  case dwarf::DW_ATE_signed:
  case dwarf::DW_ATE_signed_char:
    underlying_kind = FortranType::KIND_INTEGER;
    default_name = "INTEGER";
    break;
  // Flang allows for unsigned types if the program is compiled with -funsigned.
  case dwarf::DW_ATE_unsigned:
    underlying_kind = FortranType::KIND_UNSIGNED;
    default_name = "UNSIGNED";
    break;
  case dwarf::DW_ATE_complex_float:
    underlying_kind = FortranType::KIND_COMPLEX;
    default_name = "COMPLEX";
    break;
  default:
    return CompilerType();
  }
  /// DWARFASTParserFortran provides the names it gets from DWARF, but in case
  /// it doesn't we fallback to the default name.
  if (name.IsEmpty() && default_name)
    name.SetCString(default_name);

  return GetOrCreateFortranBaseType(underlying_kind, bitsize, name);
}

/// Creates the type if it has not been created before, otherwise it gets it
/// from the type map.
CompilerType TypeSystemFortran::GetOrCreateFortranBaseType(int kind,
                                                           uint64_t bitsize,
                                                           ConstString name) {
  FoldingSetNodeID id;
  FortranType::Profile(id, kind, bitsize);
  FoldingSetInsertToken token;
  FortranType *candidate_type = m_basic_types.lookup(id, token);
  if (candidate_type)
    return CompilerType(weak_from_this(), (void *)candidate_type);

  auto new_type_up = std::make_unique<FortranType>(kind, bitsize, name);
  candidate_type = new_type_up.get();
  m_basic_types.insert(candidate_type, token);
  m_types.push_back(std::move(new_type_up));
  return CompilerType(weak_from_this(), (void *)candidate_type);
}

CompilerType TypeSystemFortran::CreateFortranFunction(
    ConstString name, const SmallVectorImpl<CompilerType> &parameters,
    const SmallVectorImpl<StringRef> &parameter_names,
    CompilerType return_type) {
  llvm::FoldingSetNodeID id;
  FortranFunction::Profile(id, name, parameters, return_type);
  FoldingSetInsertToken token;

  FortranFunction *function_type = m_functions.lookup(id, token);
  if (function_type)
    return CompilerType(weak_from_this(), (void *)function_type);

  auto new_type_up = std::make_unique<FortranFunction>(
      name, parameters, parameter_names, return_type);
  function_type = new_type_up.get();

  m_functions.insert(function_type, token);
  m_types.push_back(std::move(new_type_up));

  return CompilerType(weak_from_this(), (void *)function_type);
}

lldb::TypeClass
TypeSystemFortran::GetTypeClass(lldb::opaque_compiler_type_t type) {
  return type ? lldb::eTypeClassBuiltin : lldb::eTypeClassInvalid;
}

CompilerType
TypeSystemFortran::GetCanonicalType(lldb::opaque_compiler_type_t type) {
  if (!type)
    return CompilerType();
  return CompilerType(weak_from_this(), type);
}

int TypeSystemFortran::GetFunctionArgumentCount(
    lldb::opaque_compiler_type_t type) {
  FortranFunction *function_type = FortranFunction::GetFunctionType(type);
  if (!function_type)
    return -1;

  return function_type->GetNumParams();
}

CompilerType
TypeSystemFortran::GetFunctionReturnType(lldb::opaque_compiler_type_t type) {
  FortranFunction *function_type = FortranFunction::GetFunctionType(type);
  if (!function_type)
    return CompilerType();

  CompilerType return_type = function_type->GetReturnType();

  return return_type.IsValid() ? return_type : CompilerType();
}

Expected<uint64_t>
TypeSystemFortran::GetBitSize(opaque_compiler_type_t type,
                              ExecutionContextScope *exe_scope) {
  if (!type)
    return 0;
  return static_cast<FortranType *>(type)->GetBitSize();
}

Encoding TypeSystemFortran::GetEncoding(opaque_compiler_type_t type) {
  if (!type)
    return eEncodingInvalid;

  switch (static_cast<FortranType *>(type)->GetKind()) {
  case FortranType::KIND_COMPLEX:
  case FortranType::KIND_REAL:
    return eEncodingIEEE754;
  case FortranType::KIND_INTEGER:
    return eEncodingSint;
  case FortranType::KIND_LOGICAL:
    return eEncodingUint;
  default:
    return eEncodingInvalid;
  }
}

Format TypeSystemFortran::GetFormat(opaque_compiler_type_t type) {
  if (!type)
    return eFormatDefault;

  switch (static_cast<FortranType *>(type)->GetKind()) {
  case FortranType::KIND_INTEGER:
    return eFormatDecimal;
  case FortranType::KIND_REAL:
    return eFormatFloat;
  case FortranType::KIND_LOGICAL:
    return eFormatBoolean;
  case FortranType::KIND_COMPLEX:
    return eFormatComplex;
  default:
    return eFormatDefault;
  }
}

BasicType
TypeSystemFortran::GetBasicTypeEnumeration(lldb::opaque_compiler_type_t type) {
  if (!type)
    return eBasicTypeInvalid;

  FortranType *fortran_type = static_cast<FortranType *>(type);
  switch (fortran_type->GetKind()) {
  case FortranType::KIND_INTEGER:
    switch (fortran_type->GetBitSize()) {
    case 8:
      return eBasicTypeSignedChar;
    case 16:
      return eBasicTypeShort;
    case 32:
      return eBasicTypeInt;
    case 64:
      return eBasicTypeLongLong;
    case 128:
      return eBasicTypeInt128;
    default:
      return eBasicTypeInvalid;
    }
  case FortranType::KIND_UNSIGNED:
    switch (fortran_type->GetBitSize()) {
    case 8:
      return eBasicTypeUnsignedChar;
    case 16:
      return eBasicTypeUnsignedShort;
    case 32:
      return eBasicTypeUnsignedInt;
    case 64:
      return eBasicTypeUnsignedLongLong;
    case 128:
      return eBasicTypeUnsignedInt128;
    default:
      return eBasicTypeInvalid;
    }
  case FortranType::KIND_LOGICAL:
    return eBasicTypeBool;
  case FortranType::KIND_COMPLEX:
    switch (fortran_type->GetBitSize()) {
    case 64:
      return eBasicTypeFloatComplex;
    case 128:
      return eBasicTypeDoubleComplex;
    default:
      return eBasicTypeInvalid;
    }
  case FortranType::KIND_REAL:
    switch (fortran_type->GetBitSize()) {
    case 16:
      return eBasicTypeHalf;
    case 32:
      return eBasicTypeFloat;
    case 64:
      return eBasicTypeDouble;
    case 128:
      return eBasicTypeFloat128;
    default:
      return eBasicTypeInvalid;
    }
  default:
    return eBasicTypeInvalid;
  }
}

bool TypeSystemFortran::DumpTypeValue(
    lldb::opaque_compiler_type_t type, Stream &s, lldb::Format format,
    const DataExtractor &data, lldb::offset_t data_offset,
    size_t data_byte_size, uint32_t bitfield_bit_size,
    uint32_t bitfield_bit_offset, ExecutionContextScope *exe_scope) {
  if (!type)
    return false;

  DataExtractor format_data;
  switch (static_cast<FortranType *>(type)->GetKind()) {
  case FortranType::KIND_INTEGER:
  case FortranType::KIND_REAL:
  case FortranType::KIND_LOGICAL:
    format_data.SetData(data, 0, data.GetByteSize());
    format_data.SetAddressByteSize(data.GetAddressByteSize());
    format_data.SetByteOrder(m_byte_order);

    return DumpDataExtractor(format_data, &s, data_offset, format,
                             data_byte_size, 1 /*item_count*/, UINT32_MAX,
                             LLDB_INVALID_ADDRESS, bitfield_bit_size,
                             bitfield_bit_offset, exe_scope);

  case FortranType::KIND_COMPLEX:
    // For Complex we print the value exactly how Fortran prints it.
    format_data.SetData(data, 0, data.GetByteSize());
    format_data.SetAddressByteSize(data.GetAddressByteSize());
    format_data.SetByteOrder(m_byte_order);
    return DumpComplex(s, data, data_offset, data_byte_size);

  default:
    Host::SystemLog(lldb::eSeverityError,
                    "Error: DumpTypeValue not handled yet.\n");
    return false;
  }
}

void TypeSystemFortran::DumpTypeDescription(lldb::opaque_compiler_type_t type,
                                            lldb::DescriptionLevel level) {
  StreamFile s(stdout, false);
  DumpTypeDescription(type, s, level);
}

void TypeSystemFortran::DumpTypeDescription(lldb::opaque_compiler_type_t type,
                                            Stream &s,
                                            lldb::DescriptionLevel level) {
  if (!type)
    return;

  FortranType *base_type = static_cast<FortranType *>(type);

  switch (base_type->GetKind()) {
  case FortranType::KIND_COMPLEX:
  case FortranType::KIND_FUNCTION:
  case FortranType::KIND_INTEGER:
  case FortranType::KIND_LOGICAL:
  case FortranType::KIND_REAL:
    s << base_type->GetName();
    break;
  default:
    break;
  }
}

// TODO: When a program is compiled with flags like -fdefault-integer-8 or
// -fdefault-real-8, default KIND-less types have an 8-byte width instead of 4.
// Consider whether the producer flags of the program should be queried to
// determine the bitsize for eBasicTypeInt and eBasicTypeFloat.
CompilerType TypeSystemFortran::GetBasicTypeFromAST(BasicType basic_type) {
  switch (basic_type) {
  case eBasicTypeInt:
    return GetOrCreateFortranBaseType(FortranType::KIND_INTEGER, 32,
                                      ConstString("INTEGER"));
  case eBasicTypeUnsignedInt:
    return GetOrCreateFortranBaseType(FortranType::KIND_UNSIGNED, 32,
                                      ConstString("UNSIGNED"));
  case eBasicTypeFloat:
    return GetOrCreateFortranBaseType(FortranType::KIND_REAL, 32,
                                      ConstString("REAL"));
  case eBasicTypeDouble:
    return GetOrCreateFortranBaseType(FortranType::KIND_REAL, 64,
                                      ConstString("REAL(KIND=8)"));
  case eBasicTypeBool:
    return GetOrCreateFortranBaseType(FortranType::KIND_LOGICAL, 32,
                                      ConstString("LOGICAL"));
  case eBasicTypeFloatComplex:
    return GetOrCreateFortranBaseType(FortranType::KIND_COMPLEX, 64,
                                      ConstString("COMPLEX"));
  case eBasicTypeDoubleComplex:
    return GetOrCreateFortranBaseType(FortranType::KIND_COMPLEX, 128,
                                      ConstString("COMPLEX(KIND=8)"));
  default:
    return CompilerType();
  }
}

CompilerType
TypeSystemFortran::GetBuiltinTypeForEncodingAndBitSize(Encoding encoding,
                                                       size_t bit_size) {
  switch (encoding) {
  case eEncodingSint:
    return GetOrCreateFortranBaseType(FortranType::KIND_INTEGER, bit_size,
                                      ConstString("INTEGER"));
  case eEncodingUint:
    return GetOrCreateFortranBaseType(FortranType::KIND_UNSIGNED, bit_size,
                                      ConstString("UNSIGNED"));
  case eEncodingIEEE754:
    return GetOrCreateFortranBaseType(FortranType::KIND_REAL, bit_size,
                                      ConstString("REAL"));
  default:
    return CompilerType();
  }
}
