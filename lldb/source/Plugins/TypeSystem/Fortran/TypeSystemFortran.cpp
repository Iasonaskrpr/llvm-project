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
#include "lldb/ValueObject/ValueObject.h"

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
  if (target) {
    type_system_sp->SetByteOrder(target->GetArchitecture().GetByteOrder());
    type_system_sp->SetAddressByteSize(
        target->GetArchitecture().GetAddressByteSize());
  } else if (module) {
    type_system_sp->SetByteOrder(module->GetArchitecture().GetByteOrder());
    type_system_sp->SetAddressByteSize(
        module->GetArchitecture().GetAddressByteSize());
  }

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

bool TypeSystemFortran::IsFunctionPointerType(
    lldb::opaque_compiler_type_t type) {
  if (!type || !IsPointerType(type, nullptr))
    return false;

  FortranPointer *ptr_type = static_cast<FortranPointer *>(type);
  if (!IsFunctionType(ptr_type->GetPointeeType().GetOpaqueQualType()))
    return false;

  return true;
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

bool TypeSystemFortran::IsPointerType(lldb::opaque_compiler_type_t type,
                                      CompilerType *pointee_type) {
  if (!type ||
      static_cast<FortranType *>(type)->GetKind() != FortranType::KIND_POINTER)
    return false;

  FortranPointer *ptr_type = static_cast<FortranPointer *>(type);
  if (pointee_type)
    *pointee_type = ptr_type->GetPointeeType();

  return true;
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
  case FortranType::KIND_POINTER:
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
  case FortranType::KIND_POINTER:
    return eTypeHasChildren | eTypeIsPointer | eTypeHasValue;
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

CompilerType
TypeSystemFortran::GetPointeeType(lldb::opaque_compiler_type_t type) {
  CompilerType pointee_type;
  if (!type || !IsPointerType(type, &pointee_type))
    return CompilerType();

  return pointee_type;
}

CompilerType
TypeSystemFortran::GetPointerType(lldb::opaque_compiler_type_t type) {
  if (!type)
    return CompilerType();

  llvm::FoldingSetNodeID id;
  FoldingSetInsertToken token;

  CompilerType pointee_type(weak_from_this(), type);
  FortranPointer::Profile(id, pointee_type);
  FortranPointer *candidate_type = m_pointers.lookup(id, token);
  if (candidate_type)
    return CompilerType(weak_from_this(), (void *)candidate_type);

  uint32_t address_bitsize = GetAddressByteSize() * 8;
  auto new_type_up = std::make_unique<FortranPointer>(
      address_bitsize, pointee_type.GetTypeName(), pointee_type);
  candidate_type = new_type_up.get();

  m_types.push_back(std::move(new_type_up));
  m_pointers.insert(candidate_type, token);
  return CompilerType(weak_from_this(), (void *)candidate_type);
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
  case FortranType::KIND_POINTER:
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
  case FortranType::KIND_POINTER:
    return eFormatHex;
  default:
    return eFormatDefault;
  }
}

llvm::Expected<uint32_t>
TypeSystemFortran::GetNumChildren(lldb::opaque_compiler_type_t type,
                                  bool omit_empty_base_classes,
                                  const ExecutionContext *exe_ctx) {
  if (!type)
    return 0;

  switch (static_cast<FortranType *>(type)->GetKind()) {
  case FortranType::KIND_POINTER:
    return IsFunctionPointerType(type) ? 0 : 1;
  default:
    return 0;
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

llvm::Expected<CompilerType> TypeSystemFortran::GetDereferencedType(
    lldb::opaque_compiler_type_t type, ExecutionContext *exe_ctx,
    std::string &deref_name, uint32_t &deref_byte_size,
    int32_t &deref_byte_offset, ValueObject *valobj, uint64_t &language_flags) {
  if (!IsPointerType(type, nullptr))
    return llvm::createStringError("not a pointer type");

  uint32_t child_bitfield_bit_size = 0;
  uint32_t child_bitfield_bit_offset = 0;
  bool child_is_base_class;
  bool child_is_deref_of_parent;

  return GetChildCompilerTypeAtIndex(
      type, exe_ctx, 0, false, true, false, deref_name, deref_byte_size,
      deref_byte_offset, child_bitfield_bit_size, child_bitfield_bit_offset,
      child_is_base_class, child_is_deref_of_parent, valobj, language_flags);
}

llvm::Expected<CompilerType> TypeSystemFortran::GetChildCompilerTypeAtIndex(
    lldb::opaque_compiler_type_t type, ExecutionContext *exe_ctx, size_t idx,
    bool transparent_pointers, bool omit_empty_base_classes,
    bool ignore_array_bounds, std::string &child_name,
    uint32_t &child_byte_size, int32_t &child_byte_offset,
    uint32_t &child_bitfield_bit_size, uint32_t &child_bitfield_bit_offset,
    bool &child_is_base_class, bool &child_is_deref_of_parent,
    ValueObject *valobj, uint64_t &language_flags) {
  if (!type)
    return llvm::createStringError("invalid type");

  child_bitfield_bit_size = 0;
  child_bitfield_bit_offset = 0;
  child_is_base_class = false;
  language_flags = 0;

  auto num_children_or_err =
      GetNumChildren(type, omit_empty_base_classes, exe_ctx);
  if (!num_children_or_err)
    return num_children_or_err.takeError();

  const bool idx_is_valid = idx < *num_children_or_err;
  if (!idx_is_valid)
    return llvm::createStringError("invalid index");

  auto get_exe_scope = [&exe_ctx]() {
    return exe_ctx ? exe_ctx->GetBestExecutionContextScope() : nullptr;
  };

  switch (static_cast<FortranType *>(type)->GetKind()) {
  case FortranType::KIND_POINTER: {
    CompilerType pointee_type(GetPointeeType(type));

    if (transparent_pointers && pointee_type.IsAggregateType()) {
      child_is_deref_of_parent = false;
      bool tmp_child_is_deref_of_parent = false;
      return pointee_type.GetChildCompilerTypeAtIndex(
          exe_ctx, idx, transparent_pointers, omit_empty_base_classes,
          ignore_array_bounds, child_name, child_byte_size, child_byte_offset,
          child_bitfield_bit_size, child_bitfield_bit_offset,
          child_is_base_class, tmp_child_is_deref_of_parent, valobj,
          language_flags);
    }

    child_is_deref_of_parent = true;
    const char *parent_name = valobj ? valobj->GetName().GetCString() : nullptr;
    if (parent_name) {
      child_name.assign(1, '*');
      child_name += parent_name;
    }

    if (idx == 0 && pointee_type.GetCompleteType()) {
      auto size_or_err = pointee_type.GetByteSize(get_exe_scope());
      if (!size_or_err)
        return size_or_err.takeError();
      child_byte_size = *size_or_err;
      child_byte_offset = 0;
      return pointee_type;
    }
    break;
  }
  default:
    break;
  }
  return CompilerType();
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
  case FortranType::KIND_POINTER:
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
