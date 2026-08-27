"""
Tests that pointers in fortran can dereference correctly and print the right values
"""

import lldb
import lldbsuite.test.lldbutil as lldbutil
from lldbsuite.test.lldbtest import *


class FortranTestPointers(TestBase):

    def test_fortran_pointers(self):
        """Tests if pointers return the correct name, kind and value."""
        self.build()
        self.main_source_file = lldb.SBFileSpec("pointers.f90")
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here", self.main_source_file
        )

        frame = thread.GetFrameAtIndex(0)

        self.expect("frame variable int_ptr", substrs=["(INTEGER *) _QFEint_ptr"])

        self.expect("frame variable *int_ptr", substrs=["(INTEGER) *int_ptr = 42"])

        self.expect("frame variable func_ptr", substrs=["(INTEGER <unnamed function>(INTEGER * ) *) _QFEfunc_ptr"])

        var_int_ptr = frame.FindVariable("int_ptr")
        self.assertTrue(var_int_ptr.IsValid())
        self.assertTrue(var_int_ptr.GetType().IsPointerType())

        # Test dereference
        deref = var_int_ptr.Dereference()
        self.assertTrue(deref.IsValid())
        self.assertEqual(deref.GetValueAsSigned(), 42)

        var_func_ptr = frame.FindVariable("func_ptr")
        self.assertTrue(var_func_ptr.IsValid())
        self.assertTrue(var_func_ptr.GetType().IsPointerType())

        # Check that address resolves to the square function
        func_addr = var_func_ptr.GetValueAsUnsigned()
        sb_addr = target.ResolveLoadAddress(func_addr)
        self.assertTrue(sb_addr.IsValid())
        self.assertIn("square", sb_addr.GetSymbol().GetName())
