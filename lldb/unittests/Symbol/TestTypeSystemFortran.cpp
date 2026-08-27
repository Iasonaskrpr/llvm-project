//===-- TestTypeSystemFortran.cpp -----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Plugins/TypeSystem/Fortran/FortranTypes.h"
#include "Plugins/TypeSystem/Fortran/TypeSystemFortran.h"
#include "TestingSupport/SubsystemRAII.h"
#include "lldb/Core/Declaration.h"
#include "lldb/Host/FileSystem.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/lldb-enumerations.h"
#include "llvm/Testing/Support/Error.h"
#include "gtest/gtest.h"

using namespace lldb;
using namespace lldb_private;
using namespace lldb_private::plugin::fortran;

class TypeSystemFortranHolder {
  std::shared_ptr<TypeSystemFortran> m_ast;

public:
  TypeSystemFortranHolder() : m_ast(std::make_shared<TypeSystemFortran>()) {}
  TypeSystemFortran *GetAST() const { return m_ast.get(); }
};

class TestTypeSystemFortran : public testing::Test {
public:
  SubsystemRAII<FileSystem, HostInfo> subsystems;

  void SetUp() override {
    m_holder = std::make_unique<TypeSystemFortranHolder>();
    m_ast = m_holder->GetAST();
  }

  void TearDown() override {
    m_ast = nullptr;
    m_holder.reset();
  }

protected:
  TypeSystemFortran *m_ast = nullptr;
  std::unique_ptr<TypeSystemFortranHolder> m_holder;
};

TEST_F(TestTypeSystemFortran, TestBaseTypes) {
  CompilerType logical_type = m_ast->CreateBaseType(llvm::dwarf::DW_ATE_boolean,
                                                    32, ConstString("Logical"));
  EXPECT_TRUE(logical_type.IsValid());
  auto bitsize_or_err = logical_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 32U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(logical_type.GetOpaqueQualType()),
            eBasicTypeBool);

  CompilerType int8_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 8, ConstString());
  EXPECT_TRUE(int8_type.IsValid());
  bitsize_or_err = int8_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 8U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(int8_type.GetOpaqueQualType()),
            eBasicTypeSignedChar);

  CompilerType int16_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 16, ConstString());
  EXPECT_TRUE(int16_type.IsValid());
  bitsize_or_err = int16_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 16U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(int16_type.GetOpaqueQualType()),
            eBasicTypeShort);

  CompilerType int32_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 32, ConstString());
  EXPECT_TRUE(int32_type.IsValid());
  bitsize_or_err = int32_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 32U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(int32_type.GetOpaqueQualType()),
            eBasicTypeInt);

  CompilerType int64_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 64, ConstString());
  EXPECT_TRUE(int64_type.IsValid());
  bitsize_or_err = int64_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 64U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(int64_type.GetOpaqueQualType()),
            eBasicTypeLongLong);

  CompilerType int128_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 128, ConstString());
  EXPECT_TRUE(int128_type.IsValid());
  bitsize_or_err = int128_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 128U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(int128_type.GetOpaqueQualType()),
            eBasicTypeInt128);

  CompilerType real16_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_float, 16, ConstString());
  EXPECT_TRUE(real16_type.IsValid());
  bitsize_or_err = real16_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 16U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(real16_type.GetOpaqueQualType()),
            eBasicTypeHalf);

  CompilerType real32_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_float, 32, ConstString());
  EXPECT_TRUE(real32_type.IsValid());
  bitsize_or_err = real32_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 32U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(real32_type.GetOpaqueQualType()),
            eBasicTypeFloat);

  CompilerType real64_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_float, 64, ConstString());
  EXPECT_TRUE(real64_type.IsValid());
  bitsize_or_err = real64_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 64U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(real64_type.GetOpaqueQualType()),
            eBasicTypeDouble);

  CompilerType real128_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_float, 128, ConstString());
  EXPECT_TRUE(real128_type.IsValid());
  bitsize_or_err = real128_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 128U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(real128_type.GetOpaqueQualType()),
            eBasicTypeFloat128);

  CompilerType complex64_type = m_ast->CreateBaseType(
      llvm::dwarf::DW_ATE_complex_float, 64, ConstString());
  EXPECT_TRUE(complex64_type.IsValid());
  bitsize_or_err = complex64_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 64U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(complex64_type.GetOpaqueQualType()),
            eBasicTypeFloatComplex);

  CompilerType complex128_type = m_ast->CreateBaseType(
      llvm::dwarf::DW_ATE_complex_float, 128, ConstString());
  EXPECT_TRUE(complex128_type.IsValid());
  bitsize_or_err = complex128_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 128U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(complex128_type.GetOpaqueQualType()),
            eBasicTypeDoubleComplex);

  CompilerType complex256_type = m_ast->CreateBaseType(
      llvm::dwarf::DW_ATE_complex_float, 256, ConstString());
  EXPECT_TRUE(complex256_type.IsValid());
  bitsize_or_err = complex256_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 256U);
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(complex256_type.GetOpaqueQualType()),
            eBasicTypeLongDoubleComplex);

  CompilerType invalid_int =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 42, ConstString());
  EXPECT_EQ(m_ast->GetBasicTypeEnumeration(invalid_int.GetOpaqueQualType()),
            eBasicTypeInvalid);
}

TEST_F(TestTypeSystemFortran, TestEncodingAndFormat) {
  CompilerType logical_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_boolean, 32, ConstString());
  CompilerType int_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 32, ConstString());
  CompilerType real_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_float, 32, ConstString());
  CompilerType complex_type = m_ast->CreateBaseType(
      llvm::dwarf::DW_ATE_complex_float, 64, ConstString());

  EXPECT_EQ(logical_type.GetEncoding(), eEncodingUint);
  EXPECT_EQ(int_type.GetEncoding(), eEncodingSint);
  EXPECT_EQ(real_type.GetEncoding(), eEncodingIEEE754);
  EXPECT_EQ(complex_type.GetEncoding(), eEncodingIEEE754);

  EXPECT_EQ(logical_type.GetFormat(), eFormatBoolean);
  EXPECT_EQ(int_type.GetFormat(), eFormatDecimal);
  EXPECT_EQ(real_type.GetFormat(), eFormatFloat);
  EXPECT_EQ(complex_type.GetFormat(), eFormatComplex);
}

TEST_F(TestTypeSystemFortran, TestTypeClassifications) {
  CompilerType logical_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_boolean, 32, ConstString());
  CompilerType int_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 32, ConstString());
  CompilerType real_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_float, 32, ConstString());
  CompilerType complex_type = m_ast->CreateBaseType(
      llvm::dwarf::DW_ATE_complex_float, 64, ConstString());

  bool is_signed = false;

  EXPECT_TRUE(int_type.IsIntegerType(is_signed));
  EXPECT_TRUE(is_signed);
  EXPECT_FALSE(logical_type.IsIntegerType(is_signed));
  EXPECT_FALSE(real_type.IsIntegerType(is_signed));
  EXPECT_FALSE(complex_type.IsIntegerType(is_signed));

  EXPECT_TRUE(real_type.IsFloatingPointType());
  EXPECT_FALSE(int_type.IsFloatingPointType());
  EXPECT_FALSE(logical_type.IsFloatingPointType());
  EXPECT_FALSE(complex_type.IsFloatingPointType());
}

TEST_F(TestTypeSystemFortran, TestGetTypeInfo) {
  CompilerType int_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 32, ConstString());
  CompilerType real_type =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_float, 32, ConstString());
  CompilerType complex_type = m_ast->CreateBaseType(
      llvm::dwarf::DW_ATE_complex_float, 64, ConstString());
  llvm::SmallVector<CompilerType, 0> parameters;
  llvm::SmallVector<llvm::StringRef, 0> parameter_names;
  CompilerType function_type = m_ast->CreateFortranFunction(
      ConstString("test_func"), parameters, parameter_names, complex_type);
  uint32_t int_flags = int_type.GetTypeInfo();
  EXPECT_TRUE(int_flags & eTypeIsBuiltIn);
  EXPECT_TRUE(int_flags & eTypeHasValue);
  EXPECT_TRUE(int_flags & eTypeIsScalar);
  EXPECT_TRUE(int_flags & eTypeIsInteger);
  EXPECT_TRUE(int_flags & eTypeIsSigned);

  uint32_t real_flags = real_type.GetTypeInfo();
  EXPECT_TRUE(real_flags & eTypeIsBuiltIn);
  EXPECT_TRUE(real_flags & eTypeHasValue);
  EXPECT_TRUE(real_flags & eTypeIsScalar);
  EXPECT_TRUE(real_flags & eTypeIsFloat);

  uint32_t complex_flags = complex_type.GetTypeInfo();
  EXPECT_TRUE(complex_flags & eTypeIsBuiltIn);
  EXPECT_TRUE(complex_flags & eTypeHasValue);
  EXPECT_TRUE(complex_flags & eTypeIsComplex);
  EXPECT_TRUE(complex_flags & eTypeIsScalar);

  uint32_t function_flags = function_type.GetTypeInfo();
  EXPECT_TRUE(function_flags & eTypeIsFuncPrototype);
}

TEST_F(TestTypeSystemFortran, TestTypeNameGeneration) {
  CompilerType logical32 =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_boolean, 32, ConstString());
  CompilerType int32 =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 32, ConstString());
  CompilerType real32 =
      m_ast->CreateBaseType(llvm::dwarf::DW_ATE_float, 32, ConstString());
  CompilerType complex64 = m_ast->CreateBaseType(
      llvm::dwarf::DW_ATE_complex_float, 64, ConstString());

  EXPECT_STREQ(logical32.GetTypeName().GetCString(), "LOGICAL");
  EXPECT_STREQ(int32.GetTypeName().GetCString(), "INTEGER");
  EXPECT_STREQ(real32.GetTypeName().GetCString(), "REAL");
  EXPECT_STREQ(complex64.GetTypeName().GetCString(), "COMPLEX");
}

TEST_F(TestTypeSystemFortran, TestFoldingSetDeduplication) {
  CompilerType int1 = m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 32,
                                            ConstString("INTEGER"));

  CompilerType int2 = m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed, 32,
                                            ConstString("INTEGER"));

  EXPECT_EQ(int1.GetOpaqueQualType(), int2.GetOpaqueQualType());
}

TEST_F(TestTypeSystemFortran, TestGetBasicTypeFromAST) {
  CompilerType int_type = m_ast->GetBasicTypeFromAST(eBasicTypeInt);
  EXPECT_TRUE(int_type.IsValid());
  EXPECT_STREQ(int_type.GetTypeName().GetCString(), "INTEGER");

  auto bitsize_or_err = int_type.GetBitSize(nullptr);
  ASSERT_THAT_EXPECTED(bitsize_or_err, llvm::Succeeded());
  EXPECT_EQ(*bitsize_or_err, 32U);

  CompilerType complex_type =
      m_ast->GetBasicTypeFromAST(eBasicTypeDoubleComplex);
  EXPECT_TRUE(complex_type.IsValid());
  EXPECT_STREQ(complex_type.GetTypeName().GetCString(), "COMPLEX(KIND=8)");
}

TEST_F(TestTypeSystemFortran, TestFortranFunctions) {
  llvm::SmallVector<CompilerType, 2> parameters;
  llvm::SmallVector<llvm::StringRef, 2> parameter_names;
  CompilerType logical_type = m_ast->CreateBaseType(llvm::dwarf::DW_ATE_boolean,
                                                    32, ConstString("LOGICAL"));
  CompilerType int32_type = m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed,
                                                  32, ConstString("INTEGER"));
  parameters.push_back(logical_type);
  parameters.push_back(int32_type);
  parameter_names.push_back("arg_1");
  parameter_names.push_back("arg_2");

  CompilerType function_type = m_ast->CreateFortranFunction(
      ConstString("test_func"), parameters, parameter_names, int32_type);

  CompilerType first_arg = function_type.GetFunctionArgumentAtIndex(0);
  CompilerType second_arg = function_type.GetFunctionArgumentAtIndex(1);
  CompilerType invalid_arg = function_type.GetFunctionArgumentAtIndex(2);
  CompilerType return_type = function_type.GetFunctionReturnType();

  EXPECT_TRUE(function_type.IsFunctionType());
  EXPECT_STREQ(function_type.GetTypeName().GetCString(),
               "INTEGER test_func(LOGICAL arg_1, INTEGER arg_2)");
  EXPECT_EQ(function_type.GetNumberOfFunctionArguments(), 2);
  EXPECT_EQ(function_type.GetFunctionArgumentCount(), 2);

  EXPECT_EQ(first_arg.GetBasicTypeEnumeration(), eBasicTypeBool);
  EXPECT_TRUE(second_arg.IsInteger());
  EXPECT_FALSE(invalid_arg.IsValid());
  EXPECT_TRUE(return_type.IsInteger());

  // For non-function types this should return -1
  EXPECT_EQ(return_type.GetFunctionArgumentCount(), -1);
  EXPECT_FALSE(return_type.IsFunctionType());

  CompilerType invalid_return_type;
  CompilerType subroutine_type = m_ast->CreateFortranFunction(
      ConstString("my_sub"), parameters, parameter_names, invalid_return_type);

  EXPECT_STREQ(subroutine_type.GetTypeName().GetCString(),
               "my_sub(LOGICAL arg_1, INTEGER arg_2)");
  EXPECT_FALSE(subroutine_type.GetFunctionReturnType().IsValid());

  llvm::SmallVector<CompilerType, 0> no_args;
  llvm::SmallVector<llvm::StringRef, 0> no_args_names;
  CompilerType void_func_type = m_ast->CreateFortranFunction(
      ConstString("do_nothing"), no_args, no_args_names, invalid_return_type);

  EXPECT_STREQ(void_func_type.GetTypeName().GetCString(), "do_nothing()");
  EXPECT_EQ(void_func_type.GetFunctionArgumentCount(), 0);
  EXPECT_EQ(void_func_type.GetNumberOfFunctionArguments(), 0);
}

TEST_F(TestTypeSystemFortran, TestFortranPointers) {
  llvm::SmallVector<CompilerType, 0> no_args;
  llvm::SmallVector<llvm::StringRef, 0> no_args_names;
  CompilerType void_func_type = m_ast->CreateFortranFunction(
      ConstString("do_nothing"), no_args, no_args_names, CompilerType());
  
  CompilerType int32_type = m_ast->CreateBaseType(llvm::dwarf::DW_ATE_signed,
                                                  32, ConstString("INTEGER"));    

  // The type system relies on getting access to either module or target 
  // to set address size, which in this instance doesn't happen so we need 
  // to set the address  byte size ourselves.                                                
  m_ast->SetAddressByteSize(8);
  
  CompilerType func_ptr = m_ast->GetPointerType(void_func_type.GetOpaqueQualType());
  CompilerType int_ptr = m_ast->GetPointerType(int32_type.GetOpaqueQualType());

  EXPECT_TRUE(int_ptr.IsPointerType());
  EXPECT_TRUE(func_ptr.IsPointerType());
  EXPECT_FALSE(int32_type.IsPointerType());

  EXPECT_TRUE(func_ptr.IsFunctionPointerType());
  EXPECT_FALSE(int_ptr.IsFunctionPointerType());

  EXPECT_THAT_EXPECTED(int_ptr.GetNumChildren(false, nullptr), llvm::HasValue(1u));
  EXPECT_THAT_EXPECTED(func_ptr.GetNumChildren(false, nullptr), llvm::HasValue(0u));
  EXPECT_THAT_EXPECTED(int32_type.GetNumChildren(false, nullptr), llvm::HasValue(0u));

  EXPECT_STREQ(int_ptr.GetTypeName().AsCString(""), "INTEGER *");
  EXPECT_STREQ(func_ptr.GetTypeName().AsCString(""), "do_nothing() *");

  auto func_ptr_size_or_err = func_ptr.GetByteSize(nullptr);
  ASSERT_THAT_EXPECTED(func_ptr_size_or_err, llvm::Succeeded());
  
  auto int_ptr_size_or_err = int_ptr.GetByteSize(nullptr);
  ASSERT_THAT_EXPECTED(int_ptr_size_or_err, llvm::Succeeded()); 

  EXPECT_EQ(*func_ptr_size_or_err, 8U);
  EXPECT_EQ(*int_ptr_size_or_err, 8U);

  CompilerType int_pointee_type = int_ptr.GetPointeeType();
  CompilerType func_pointee_type = func_ptr.GetPointeeType();  

  EXPECT_TRUE(func_pointee_type.IsFunctionType());
  EXPECT_FALSE(func_pointee_type.IsFunctionPointerType());

  EXPECT_TRUE(int_pointee_type.IsInteger());
  EXPECT_FALSE(int_pointee_type.IsPointerType());

  std::string child_name;
  uint32_t child_byte_size = 0;
  int32_t child_byte_offset = 0;
  uint32_t child_bitfield_bit_size = 0;
  uint32_t child_bitfield_bit_offset = 0;
  bool child_is_base_class = false;
  bool child_is_deref_of_parent = false;
  uint64_t language_flags = 0;

  auto int_child_or_err = int_ptr.GetChildCompilerTypeAtIndex(
      nullptr, 0, false, true, false, child_name, child_byte_size,
      child_byte_offset, child_bitfield_bit_size, child_bitfield_bit_offset,
      child_is_base_class, child_is_deref_of_parent, nullptr, language_flags);

  ASSERT_THAT_EXPECTED(int_child_or_err, llvm::Succeeded());
  EXPECT_EQ(*int_child_or_err, int32_type);
  EXPECT_EQ(child_byte_size, 4U);
  EXPECT_EQ(child_byte_offset, 0);
  EXPECT_TRUE(child_is_deref_of_parent);
  EXPECT_FALSE(child_is_base_class);

  auto int_invalid_idx_err = int_ptr.GetChildCompilerTypeAtIndex(
      nullptr, 1, false, true, false, child_name, child_byte_size,
      child_byte_offset, child_bitfield_bit_size, child_bitfield_bit_offset,
      child_is_base_class, child_is_deref_of_parent, nullptr, language_flags);
  EXPECT_THAT_EXPECTED(int_invalid_idx_err, llvm::Failed());

  auto func_child_err = func_ptr.GetChildCompilerTypeAtIndex(
      nullptr, 0, false, true, false, child_name, child_byte_size,
      child_byte_offset, child_bitfield_bit_size, child_bitfield_bit_offset,
      child_is_base_class, child_is_deref_of_parent, nullptr, language_flags);
  EXPECT_THAT_EXPECTED(func_child_err, llvm::Failed());

  std::string deref_name;
  uint32_t deref_byte_size = 0;
  int32_t deref_byte_offset = 0;
  language_flags = 0;

  auto deref_int_or_err = int_ptr.GetDereferencedType(
      nullptr, deref_name, deref_byte_size, deref_byte_offset, nullptr,
      language_flags);
  ASSERT_THAT_EXPECTED(deref_int_or_err, llvm::Succeeded());
  EXPECT_EQ(*deref_int_or_err, int32_type);
  EXPECT_EQ(deref_byte_size, 4U);
  EXPECT_EQ(deref_byte_offset, 0);

  auto deref_func_err = func_ptr.GetDereferencedType(
      nullptr, deref_name, deref_byte_size, deref_byte_offset, nullptr,
      language_flags);
  EXPECT_THAT_EXPECTED(deref_func_err, llvm::Failed());

  auto deref_non_ptr_err = int32_type.GetDereferencedType(
      nullptr, deref_name, deref_byte_size, deref_byte_offset, nullptr,
      language_flags);
  EXPECT_THAT_EXPECTED(deref_non_ptr_err, llvm::Failed());
}