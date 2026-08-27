//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements the Fortran type system.
///
//===----------------------------------------------------------------------===//
#include "TypeSystemFortran.h"
#include "FortranTypes.h"

#include "lldb/Core/DumpDataExtractor.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Host/StreamFile.h"
#include "lldb/Symbol/SymbolFile.h"
#include "lldb/Target/Target.h"
#include "lldb/ValueObject/ValueObject.h"

#include "Plugins/SymbolFile/DWARF/DWARFASTParserFortran.h"

using namespace lldb;
using namespace lldb_private;
using namespace llvm;
using namespace lldb_private::plugin::dwarf;
using namespace lldb_private::plugin::fortran;

LLDB_PLUGIN_DEFINE(TypeSystemFortran)

/// Used to determine if TypeSystem supports the language passed in
/// CreateInstance
static bool IsLanguageSupported(lldb::LanguageType language) {
  if (language == lldb::LanguageType::eLanguageTypeFortran77 ||
      language == lldb::LanguageType::eLanguageTypeFortran90 ||
      language == lldb::LanguageType::eLanguageTypeFortran95 ||
      language == lldb::LanguageType::eLanguageTypeFortran03 ||
      language == lldb::LanguageType::eLanguageTypeFortran08 ||
      language == lldb::LanguageType::eLanguageTypeFortran18)
    return true;

  return false;
}

static bool DumpComplex(Stream &s, const lldb_private::DataExtractor &data,
                        lldb::offset_t &offset, size_t data_byte_size) {
  if (sizeof(float) * 2 == data_byte_size) {
    float f32_1 = data.GetFloat(&offset);
    float f32_2 = data.GetFloat(&offset);

    s.Printf("(%g, %g)", f32_1, f32_2);
    return true;
  } else if (sizeof(double) * 2 == data_byte_size) {
    double d64_1 = data.GetDouble(&offset);
    double d64_2 = data.GetDouble(&offset);

    s.Printf("(%lg, %lg)", d64_1, d64_2);
    return true;
  } else if (sizeof(long double) * 2 == data_byte_size) {
    long double ld64_1 = data.GetLongDouble(&offset);
    long double ld64_2 = data.GetLongDouble(&offset);
    s.Printf("(%Lg, %Lg)", ld64_1, ld64_2);
    return true;
  } else {
    s.Printf("error: unsupported byte size (%" PRIu64
             ") for complex float format",
             (uint64_t)data_byte_size);
    return false;
  }
}

char TypeSystemFortran::ID;

TypeSystemFortran::~TypeSystemFortran() = default;
TypeSystemFortran::TypeSystemFortran() = default;

void TypeSystemFortran::Initialize() {
  PluginManager::RegisterPlugin(
      GetPluginNameStatic(), "fortran AST context plug-in", CreateInstance,
      GetSupportedLanguagesForTypes(), GetSupportedLanguagesForExpressions());
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

  if (IsLanguageSupported(language)) {
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
  return TypeSystemSP();
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
  int kind = static_cast<FortranType *>(type)->GetKind();
  if (kind == FortranType::KIND_REAL)
    return true;
  return false;
}

bool TypeSystemFortran::IsFunctionType(opaque_compiler_type_t type) {
  if (!type)
    return false;
  int kind = static_cast<FortranType *>(type)->GetKind();

  if (kind == FortranType::KIND_FUNCTION)
    return true;
  return false;
}

size_t TypeSystemFortran::GetNumberOfFunctionArguments(
    lldb::opaque_compiler_type_t type) {
  if (!type)
    return 0;
  FortranType *fortran_type = static_cast<FortranType *>(type);
  int kind = fortran_type->GetKind();
  if (kind != FortranType::KIND_FUNCTION)
    return 0;
  FortranFunction *fortran_function =
      static_cast<FortranFunction *>(fortran_type);
  return fortran_function->GetNumberOfParameters();
}

CompilerType
TypeSystemFortran::GetFunctionArgumentAtIndex(lldb::opaque_compiler_type_t type,
                                              const size_t index) {
  if (!type)
    return CompilerType();
  FortranType *fortran_type = static_cast<FortranType *>(type);
  int kind = fortran_type->GetKind();
  if (kind != FortranType::KIND_FUNCTION)
    return CompilerType();
  FortranFunction *fortran_function =
      static_cast<FortranFunction *>(fortran_type);
  auto parameters = fortran_function->GetParameters();
  if (index >= parameters.size())
    return CompilerType();

  return parameters[index];
}

bool TypeSystemFortran::IsFunctionPointerType(
    lldb::opaque_compiler_type_t type) {
  if (!type)
    return false;

  if (!IsPointerType(type, nullptr))
    return false;

  FortranPointer *fortran_ptr = static_cast<FortranPointer *>(type);
  if (!IsFunctionType(fortran_ptr->GetPointeeType().GetOpaqueQualType()))
    return false;
  return true;
}

bool TypeSystemFortran::IsIntegerType(opaque_compiler_type_t type,
                                      bool &is_signed) {
  if (!type)
    return false;
  FortranType *fortran_type = static_cast<FortranType *>(type);
  if (fortran_type->GetKind() == FortranType::KIND_INTEGER) {
    is_signed = true;
    return true;
  }
  return false;
}

bool TypeSystemFortran::IsPointerType(lldb::opaque_compiler_type_t type,
                                      CompilerType *pointee_type) {
  if (!type)
    return false;

  FortranType *fortran_type = static_cast<FortranType *>(type);
  if (fortran_type->GetKind() != FortranType::KIND_POINTER)
    return false;

  FortranPointer *fortran_ptr = static_cast<FortranPointer *>(fortran_type);
  if (pointee_type)
    *pointee_type = fortran_ptr->GetPointeeType();
  return true;
}

bool TypeSystemFortran::SupportsLanguage(lldb::LanguageType language) {
  return IsLanguageSupported(language);
}

/// Returns the type name upper-cased to follow Fortran's general style
ConstString TypeSystemFortran::GetTypeName(opaque_compiler_type_t type,
                                           bool BaseOnly) {
  if (!type)
    return ConstString();
  FortranType *fortran_type = static_cast<FortranType *>(type);
  switch (fortran_type->GetKind()) {
  case FortranType::KIND_INTEGER:
  case FortranType::KIND_LOGICAL:
  case FortranType::KIND_REAL:
  case FortranType::KIND_COMPLEX:
  case FortranType::KIND_FUNCTION:
  case FortranType::KIND_POINTER:
    return fortran_type->GetName();
  default:
    return ConstString("Unsupported");
  }
}

uint32_t
TypeSystemFortran::GetTypeInfo(opaque_compiler_type_t type,
                               CompilerType *pointee_or_element_compiler_type) {
  if (!type)
    return 0;
  FortranType *fortran_type = static_cast<FortranType *>(type);
  uint32_t builtin_type_flags = 0;
  int type_kind = fortran_type->GetKind();

  switch (type_kind) {
  case FortranType::KIND_REAL:
  case FortranType::KIND_INTEGER:
  case FortranType::KIND_LOGICAL:
  case FortranType::KIND_COMPLEX:
    builtin_type_flags = eTypeIsBuiltIn | eTypeHasValue | eTypeIsScalar;
    if (type_kind == FortranType::KIND_INTEGER)
      builtin_type_flags |= eTypeIsInteger | eTypeIsSigned;
    if (type_kind == FortranType::KIND_REAL)
      builtin_type_flags |= eTypeIsFloat;
    if (type_kind == FortranType::KIND_COMPLEX)
      builtin_type_flags |= eTypeIsComplex;
    break;
  case FortranType::KIND_FUNCTION:
    return eTypeIsFuncPrototype;
  case FortranType::KIND_POINTER:
    return eTypeHasChildren | eTypeIsPointer | eTypeHasValue;
  default:
    break;
  }
  return builtin_type_flags;
}

CompilerType TypeSystemFortran::CreateBaseType(uint32_t dwarf_encoding,
                                               uint64_t bitsize,
                                               ConstString name) {
  int underlying_kind;
  switch (dwarf_encoding) {
  case dwarf::DW_ATE_boolean:
    if (bitsize == 32)
      name.SetCString("LOGICAL");
    underlying_kind = FortranType::KIND_LOGICAL;
    break;
  case dwarf::DW_ATE_float:
    if (bitsize == 32)
      name.SetCString("REAL");
    underlying_kind = FortranType::KIND_REAL;
    break;
  case dwarf::DW_ATE_signed:
    if (bitsize == 32)
      name.SetCString("INTEGER");
    underlying_kind = FortranType::KIND_INTEGER;
    break;
  case dwarf::DW_ATE_complex_float:
    if (bitsize == 64)
      name.SetCString("COMPLEX");
    underlying_kind = FortranType::KIND_COMPLEX;
    break;
  default:
    return CompilerType();
  }
  return GetOrCreateFortranBaseType(underlying_kind, bitsize, name);
}

/// Returns the type assosciated with the kind and bitsize, or creates it
/// if it is not in the map
CompilerType TypeSystemFortran::GetOrCreateFortranBaseType(int kind,
                                                           uint64_t bitsize,
                                                           ConstString name) {
  llvm::FoldingSetNodeID id;
  FortranType::Profile(id, kind, bitsize);
  void *insert_pos = nullptr;
  FortranType *fortran_type = m_basic_types.FindNodeOrInsertPos(id, insert_pos);
  if (fortran_type)
    return CompilerType(weak_from_this(), (void *)fortran_type);
  auto new_type_up = std::make_unique<FortranType>(kind, bitsize, name);
  fortran_type = new_type_up.get();

  m_types.push_back(std::move(new_type_up));
  m_basic_types.InsertNode(fortran_type, insert_pos);
  return CompilerType(weak_from_this(), (void *)fortran_type);
}

CompilerType TypeSystemFortran::CreateFortranFunction(
    ConstString name, const SmallVectorImpl<CompilerType> &parameters,
    const SmallVectorImpl<StringRef> &parameter_names,
    CompilerType return_type) {
  llvm::FoldingSetNodeID id;
  FortranFunction::Profile(id, name, parameters, return_type);
  void *insert_pos = nullptr;
  FortranFunction *fortran_function =
      m_functions.FindNodeOrInsertPos(id, insert_pos);
  if (fortran_function)
    return CompilerType(weak_from_this(), (void *)fortran_function);
  auto new_type_up = std::make_unique<FortranFunction>(
      name, parameters, parameter_names, return_type);
  fortran_function = new_type_up.get();

  m_functions.InsertNode(fortran_function, insert_pos);
  m_types.push_back(std::move(new_type_up));

  return CompilerType(weak_from_this(), (void *)fortran_function);
}

lldb::TypeClass
TypeSystemFortran::GetTypeClass(lldb::opaque_compiler_type_t type) {
  if (!type)
    return lldb::eTypeClassInvalid;

  return lldb::eTypeClassBuiltin;
}

CompilerType
TypeSystemFortran::GetCanonicalType(lldb::opaque_compiler_type_t type) {
  if (!type)
    return CompilerType();
  return CompilerType(weak_from_this(), type);
}

int TypeSystemFortran::GetFunctionArgumentCount(
    lldb::opaque_compiler_type_t type) {
  if (!type)
    return -1;
  FortranType *fortran_type = static_cast<FortranType *>(type);
  int kind = fortran_type->GetKind();
  if (kind != FortranType::KIND_FUNCTION)
    return -1;
  FortranFunction *fortran_function =
      static_cast<FortranFunction *>(fortran_type);
  return fortran_function->GetNumberOfParameters();
}

CompilerType
TypeSystemFortran::GetFunctionReturnType(lldb::opaque_compiler_type_t type) {
  if (!type)
    return CompilerType();

  FortranType *fortran_type = static_cast<FortranType *>(type);
  if (fortran_type->GetKind() != FortranType::KIND_FUNCTION)
    return CompilerType();

  FortranFunction *fortran_function =
      static_cast<FortranFunction *>(fortran_type);

  CompilerType return_type = fortran_function->GetReturnType();
  if (!return_type.IsValid())
    return CompilerType();

  return return_type;
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
  CompilerType pointee_type(weak_from_this(), type);
  FortranPointer::Profile(id, pointee_type);
  void *insert_pos = nullptr;
  FortranPointer *fortran_type = m_pointers.FindNodeOrInsertPos(id, insert_pos);
  if (fortran_type)
    return CompilerType(weak_from_this(), (void *)fortran_type);

  uint32_t address_bitsize = GetAddressByteSize() * 8;
  auto new_type_up = std::make_unique<FortranPointer>(
      address_bitsize, pointee_type.GetTypeName(), pointee_type);
  fortran_type = new_type_up.get();

  m_types.push_back(std::move(new_type_up));
  m_pointers.InsertNode(fortran_type, insert_pos);
  return CompilerType(weak_from_this(), (void *)fortran_type);
}

Expected<uint64_t>
TypeSystemFortran::GetBitSize(opaque_compiler_type_t type,
                              ExecutionContextScope *exe_scope) {
  if (!type)
    return 0;
  FortranType *fortran_type = static_cast<FortranType *>(type);
  return fortran_type->GetBitSize();
}

Encoding TypeSystemFortran::GetEncoding(opaque_compiler_type_t type) {
  if (!type)
    return eEncodingInvalid;
  FortranType *fortran_type = static_cast<FortranType *>(type);
  switch (fortran_type->GetKind()) {
  case FortranType::KIND_COMPLEX:
  case FortranType::KIND_REAL:
    return eEncodingIEEE754;
  case FortranType::KIND_INTEGER:
    return eEncodingSint;
  case FortranType::KIND_POINTER:
  case FortranType::KIND_LOGICAL:
    return eEncodingUint;
  default:
    return eEncodingInvalid;
  }
}

Format TypeSystemFortran::GetFormat(opaque_compiler_type_t type) {
  if (!type)
    return eFormatDefault;
  FortranType *fortran_type = static_cast<FortranType *>(type);
  switch (fortran_type->GetKind()) {
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

  FortranType *fortran_type = static_cast<FortranType *>(type);
  switch (fortran_type->GetKind()) {
  case FortranType::KIND_POINTER: {
    if (IsFunctionPointerType(type))
      return 0;
    return 1;
  }
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
  case FortranType::KIND_LOGICAL:
    return eBasicTypeBool;
  case FortranType::KIND_COMPLEX:
    switch (fortran_type->GetBitSize()) {
    case 64:
      return eBasicTypeFloatComplex;
    case 128:
      return eBasicTypeDoubleComplex;
    case 256:
      return eBasicTypeLongDoubleComplex;
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
  FortranType *fortran_type = static_cast<FortranType *>(type);
  switch (fortran_type->GetKind()) {
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
  } break;
  default:
    return CompilerType();
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

  FortranType *fortran_type = static_cast<FortranType *>(type);
  int type_kind = fortran_type->GetKind();
  DataExtractor format_data;
  switch (type_kind) {
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
    // For Complex we print the value exactly how Fortran prints it
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
  FortranType *fortran_type = static_cast<FortranType *>(type);

  switch (fortran_type->GetKind()) {
  case FortranType::KIND_COMPLEX:
  case FortranType::KIND_FUNCTION:
  case FortranType::KIND_INTEGER:
  case FortranType::KIND_LOGICAL:
  case FortranType::KIND_REAL:
    s << fortran_type->GetName();
    break;
  default:
    break;
  }
}

CompilerType TypeSystemFortran::GetBasicTypeFromAST(BasicType basic_type) {
  switch (basic_type) {
  case eBasicTypeInt:
    return GetOrCreateFortranBaseType(FortranType::KIND_INTEGER, 32,
                                      ConstString("INTEGER"));
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
  case eBasicTypeLongDoubleComplex:
    return GetOrCreateFortranBaseType(FortranType::KIND_COMPLEX, 256,
                                      ConstString("COMPLEX(KIND=16)"));
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
  case eEncodingIEEE754:
    return GetOrCreateFortranBaseType(FortranType::KIND_REAL, bit_size,
                                      ConstString("REAL"));
  default:
    return CompilerType();
  }
}