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
#include "lldb/Target/Language.h"
#include "lldb/Target/Target.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/ValueObject/ValueObject.h"

#include "Plugins/SymbolFile/DWARF/DWARFASTParserFortran.h"

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

static ConstString CreateArrayTypeName(const CompilerType &element_type,
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

  if (Language::LanguageIsFortran(language)) {
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

bool TypeSystemFortran::IsArrayType(lldb::opaque_compiler_type_t type,
                                    CompilerType *element_type, uint64_t *size,
                                    bool *is_incomplete) {
  if (element_type)
    element_type->Clear();
  if (size)
    *size = 0;
  if (is_incomplete)
    *is_incomplete = false;

  FortranType *super_type = static_cast<FortranType *>(type);
  if (!super_type)
    return false;

  if (super_type->GetKind() != FortranType::KIND_ARRAY)
    return false;

  FortranArray *array_type = static_cast<FortranArray *>(super_type);

  if (!array_type)
    return false;

  if (element_type)
    *element_type = array_type->GetElementType();
  // TODO: If it isn't we have to evaluate the DWARFExpressionList
  if (!array_type->IsDynamic() && size)
    *size = array_type->GetTotalElements();

  return true;
}

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
  return Language::LanguageIsFortran(language);
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
  case FortranType::KIND_ARRAY:
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
  case FortranType::KIND_ARRAY:
    return eTypeIsArray | eTypeHasChildren;
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
  auto new_type_up = std::make_unique<FortranType>(kind, bitsize, name);

  FortranType *fortran_type = m_basic_types.getOrInsert(new_type_up.get());
  if (fortran_type == new_type_up.get())
    m_types.push_back(std::move(new_type_up));

  return CompilerType(weak_from_this(), (void *)fortran_type);
}

CompilerType TypeSystemFortran::CreateFortranFunction(
    ConstString name, const SmallVectorImpl<CompilerType> &parameters,
    const SmallVectorImpl<StringRef> &parameter_names,
    CompilerType return_type) {
  auto new_type_up = std::make_unique<FortranFunction>(
      name, parameters, parameter_names, return_type);
  FortranFunction *fortran_function =
      m_functions.getOrInsert(new_type_up.get());

  if (fortran_function == new_type_up.get())
    m_types.push_back(std::move(new_type_up));

  return CompilerType(weak_from_this(), (void *)fortran_function);
}

CompilerType TypeSystemFortran::CreateArrayType(FortranArrayMetadata array_info,
                                                uint64_t total_array_size,
                                                uint64_t total_elements) {
  // Assumed-rank types can be scalar, with a rank of 0, meaning they are
  // technically scalars. If this is the case we do not need to do anything.
  if (array_info.is_scalar)
    return array_info.element_type;
  size_t rank = array_info.dimensions.size();
  llvm::SmallVector<ArrayShape, 2> array_shapes;
  ConstString type_name;
  for (size_t idx = 0; idx < rank; ++idx) {
    ArrayShape shape;
    ArrayBound lb;
    ArrayBound ub;
    ArrayBound::Category bound_category;
    int64_t dim_elements = -1;

    if (std::holds_alternative<std::monostate>(
            array_info.dimensions[idx].byte_stride)) {
      auto byte_stride_or_err = array_info.element_type.GetByteSize(nullptr);
      if (!byte_stride_or_err) {
        LLDB_LOG_ERROR(GetLog(LLDBLog::Types), byte_stride_or_err.takeError(),
                       "{0}");
        return CompilerType();
      }
      shape.SetByteStride(*byte_stride_or_err);
    }

    else if (std::holds_alternative<int64_t>(
                 array_info.dimensions[idx].byte_stride))
      shape.SetByteStride(
          std::get<int64_t>(array_info.dimensions[idx].byte_stride));
    // If the elements for this dimension are unknown it is either colon or star
    // Star can only appear as the last bound

    if (!std::holds_alternative<int64_t>(
            array_info.dimensions[idx].element_count)) {
      bound_category = ArrayBound::Category::Colon;

      if (array_info.is_star && idx == rank - 1)
        bound_category = ArrayBound::Category::Star;
      shape.SetElementCount(0);
    } else {
      bound_category = ArrayBound::Category::Explicit;
      if (std::holds_alternative<int64_t>(
              array_info.dimensions[idx].element_count))
        dim_elements =
            std::get<int64_t>(array_info.dimensions[idx].element_count);
      shape.SetElementCount(dim_elements);
    }

    lb.SetCategory(bound_category);
    ub.SetCategory(bound_category);

    if (std::holds_alternative<int64_t>(
            array_info.dimensions[idx].lower_bound)) {
      int64_t lbound =
          std::get<int64_t>(array_info.dimensions[idx].lower_bound);
      lb.SetBound(lbound);
      if (dim_elements != -1)
        ub.SetBound(lbound + dim_elements - 1);
      else
        ub.SetBound(-1);
    }

    else if (std::holds_alternative<std::monostate>(
                 array_info.dimensions[idx].lower_bound)) {
      lb.SetBound(1);
      ub.SetBound(dim_elements);
    }

    if (std::holds_alternative<int64_t>(array_info.dimensions[idx].upper_bound))
      ub.SetBound(std::get<int64_t>(array_info.dimensions[idx].upper_bound));

    shape.SetLowerBound(lb);
    shape.SetUpperBound(ub);

    if (std::holds_alternative<DWARFExpressionList>(
            array_info.dimensions[idx].upper_bound))
      shape.SetUpperBoundExpression(std::get<DWARFExpressionList>(
          array_info.dimensions[idx].upper_bound));
    else if (std::holds_alternative<DWARFDIE>(
                 array_info.dimensions[idx].upper_bound))
      shape.SetUpperBoundDIE(
          std::get<DWARFDIE>(array_info.dimensions[idx].upper_bound));

    if (std::holds_alternative<DWARFExpressionList>(
            array_info.dimensions[idx].lower_bound))
      shape.SetLowerBoundExpression(std::get<DWARFExpressionList>(
          array_info.dimensions[idx].lower_bound));
    else if (std::holds_alternative<DWARFDIE>(
                 array_info.dimensions[idx].lower_bound))
      shape.SetLowerBoundDIE(
          std::get<DWARFDIE>(array_info.dimensions[idx].lower_bound));

    if (std::holds_alternative<DWARFExpressionList>(
            array_info.dimensions[idx].element_count))
      shape.SetElementCountExpression(std::get<DWARFExpressionList>(
          array_info.dimensions[idx].element_count));
    else if (std::holds_alternative<DWARFDIE>(
                 array_info.dimensions[idx].element_count))
      shape.SetElementCountDIE(
          std::get<DWARFDIE>(array_info.dimensions[idx].element_count));

    if (std::holds_alternative<DWARFExpressionList>(
            array_info.dimensions[idx].byte_stride))
      shape.SetByteStrideExpression(std::get<DWARFExpressionList>(
          array_info.dimensions[idx].byte_stride));
    else if (std::holds_alternative<DWARFDIE>(
                 array_info.dimensions[idx].byte_stride))
      shape.SetByteStrideDIE(
          std::get<DWARFDIE>(array_info.dimensions[idx].byte_stride));

    array_shapes.push_back(shape);
  }
  ConstString array_type_name = CreateArrayTypeName(
      array_info.element_type, array_shapes, array_info.is_allocatable,
      array_info.is_star, array_info.is_assumed_rank);

  auto new_type_up = std::make_unique<FortranArray>(
      array_info.element_type, array_shapes, array_type_name, total_array_size,
      array_info.is_allocatable, array_info.is_dynamic, array_info.is_star,
      array_info.is_auto, array_info.is_assumed_rank, total_elements,
      array_info.allocated_exp, array_info.data_location_exp,
      array_info.rank_exp);

  FortranArray *array_type = m_arrays.getOrInsert(new_type_up.get());

  if (array_type == new_type_up.get())
    m_types.push_back(std::move(new_type_up));

  return CompilerType(weak_from_this(), (void *)array_type);
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

  CompilerType pointee_type(weak_from_this(), type);
  uint32_t address_bitsize = GetAddressByteSize() * 8;

  auto new_type_up = std::make_unique<FortranPointer>(
      address_bitsize, pointee_type.GetTypeName(), pointee_type);

  FortranPointer *fortran_type = m_pointers.getOrInsert(new_type_up.get());

  if (fortran_type == new_type_up.get())
    m_types.push_back(std::move(new_type_up));

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
  case FortranType::KIND_ARRAY: {
    FortranArray *fortran_array = static_cast<FortranArray *>(fortran_type);

    if (!fortran_array)
      return createStringError(
          inconvertibleErrorCode(),
          "Couldn't get number of children, bad Fortran type.");

    // Fetch the number of elements
    if (!fortran_array->IsDynamic()) {
      if (fortran_array->GetDimensions().empty() || fortran_array->IsStar())
        return 0;
      return fortran_array->GetDimensions().front().GetElementCount();
    }
    return 0;
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
    return CompilerType();

  auto get_exe_scope = [&exe_ctx]() {
    return exe_ctx ? exe_ctx->GetBestExecutionContextScope() : nullptr;
  };
  FortranType *super_type = static_cast<FortranType *>(type);
  switch (super_type->GetKind()) {
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

  case FortranType::KIND_ARRAY: {
    FortranArray *fortran_type = static_cast<FortranArray *>(super_type);

    if (!fortran_type)
      return CompilerType();

    if (fortran_type->IsDynamic() || fortran_type->IsAuto())
      return CompilerType();

    llvm::ArrayRef<ArrayShape> old_dimensions = fortran_type->GetDimensions();
    ArrayShape first_dimension = old_dimensions.front();
    int64_t lb = first_dimension.GetLowerBound().GetBound();
    uint64_t num_elements;
    // In Fortran indices can be negative, but lldb defaults to using unsigned
    // by casting the index to a signed integer we can access elements with
    // negative indices.
    if (!fortran_type->IsStar()) {
      num_elements = old_dimensions.front().GetElementCount();
      if (idx >= num_elements || num_elements == 0)
        return CompilerType();
    }
    int32_t real_idx = idx + lb;
    child_name = llvm::formatv("[{0}]", real_idx);
    if (old_dimensions.size() > 1) {

      llvm::SmallVector<ArrayShape, 2> new_dimensions(
          old_dimensions.begin() + 1, old_dimensions.end());

      ArrayShape old_first_dimension = old_dimensions.front();
      uint64_t new_byte_stride;
      if (old_first_dimension.GetByteStride() != 0)
        new_byte_stride = old_first_dimension.GetElementCount() *
                          old_first_dimension.GetByteStride();
      else
        new_byte_stride = old_first_dimension.GetElementCount() *
                          fortran_type->GetElementByteSize();

      new_dimensions.front().SetByteStride(new_byte_stride);
      bool is_star = fortran_type->IsStar();
      bool is_allocatable = fortran_type->IsAllocatable();
      uint64_t new_total_elements = fortran_type->GetTotalElements() /
                                    old_first_dimension.GetElementCount();
      if (old_dimensions.front().GetByteStride() != 0)
        child_byte_offset = idx * old_dimensions.front().GetByteStride();
      else
        child_byte_offset = idx * fortran_type->GetElementByteSize();

      uint64_t last_dim_stride = new_dimensions.back().GetByteStride();

      if (last_dim_stride != 0) {
        child_byte_size =
            new_dimensions.back().GetElementCount() * last_dim_stride;
      } else {
        child_byte_size =
            new_total_elements * fortran_type->GetElementByteSize();
      }
      ConstString type_name = CreateArrayTypeName(
          fortran_type->GetElementType(), new_dimensions, is_allocatable,
          is_star, fortran_type->IsAssumedRank());

      auto new_type_up = std::make_unique<FortranArray>(
          fortran_type->GetElementType(), new_dimensions, type_name,
          child_byte_size, false, false, fortran_type->IsStar(),
          fortran_type->IsAuto(), false, new_total_elements,
          DWARFExpressionList(), DWARFExpressionList(), DWARFExpressionList());

      FortranArray *array_type = m_arrays.getOrInsert(new_type_up.get());

      if (array_type == new_type_up.get())
        m_types.push_back(std::move(new_type_up));

      return CompilerType(weak_from_this(), (void *)array_type);
    } else {
      child_byte_offset = idx * old_dimensions.front().GetByteStride();
      child_byte_size = fortran_type->GetElementByteSize();
      return fortran_type->GetElementType();
    }
  } break;
  default:
    return CompilerType();
  }
  return CompilerType();
}

void TypeSystemFortran::RegisterSyntheticArrayType(user_id_t valobj_id,
                                                   opaque_compiler_type_t type,
                                                   CompilerType array_type) {
  m_synthetic_array_types.insert_or_assign({valobj_id, type}, array_type);
}

CompilerType
TypeSystemFortran::GetExplicitArrayType(opaque_compiler_type_t type,
                                        user_id_t valobj_id) {
  auto possible_type = m_synthetic_array_types.find({valobj_id, type});

  if (possible_type == m_synthetic_array_types.end())
    return CompilerType();

  return possible_type->getSecond();
}

CompilerType
TypeSystemFortran::GetArrayElementType(lldb::opaque_compiler_type_t type,
                                       ExecutionContextScope *exe_scope) {
  if (!type)
    return CompilerType();
  FortranType *fortran_type = static_cast<FortranType *>(type);
  if (!fortran_type)
    return CompilerType();
  if (fortran_type->GetKind() != FortranType::KIND_ARRAY)
    return CompilerType();
  FortranArray *array_type = static_cast<FortranArray *>(fortran_type);
  return array_type->GetElementType();
}

int64_t TypeSystemFortran::GetArrayLowerBound(opaque_compiler_type_t type) {
  if (!type)
    return 0;
  FortranType *fortran_type = static_cast<FortranType *>(type);
  if (!fortran_type)
    return 0;
  if (fortran_type->GetKind() != FortranType::KIND_ARRAY)
    return 0;
  FortranArray *array_type = static_cast<FortranArray *>(fortran_type);
  if (!array_type->GetDimensions().front().GetLowerBound().IsBoundKnown())
    return 0;
  return array_type->GetDimensions().front().GetLowerBound().GetBound();
}

int64_t TypeSystemFortran::GetArrayByteStride(opaque_compiler_type_t type) {
  if (!type)
    return 0;
  FortranType *fortran_type = static_cast<FortranType *>(type);
  if (!fortran_type)
    return 0;
  if (fortran_type->GetKind() != FortranType::KIND_ARRAY)
    return 0;
  FortranArray *array_type = static_cast<FortranArray *>(fortran_type);
  return array_type->GetDimensions().front().GetByteStride();
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