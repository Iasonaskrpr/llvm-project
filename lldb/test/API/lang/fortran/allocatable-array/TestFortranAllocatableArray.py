"""
Tests that allocatable arrays display correctly and the
array descriptor is read properly.
"""

import lldb
import lldbsuite.test.lldbutil as lldbutil
from lldbsuite.test.lldbtest import *


class FortranTestAllocatableArrays(TestBase):

    def setUp(self):
        super().setUp()
        self.build()
        self.main_source_file = lldb.SBFileSpec("allocatable.f90")

    def test_simple_allocatable_arrays(self):
        """Tests that single dimension allocatable arrays with no custom bounds
        or byte strides display properly."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here", self.main_source_file
        )

        self.expect(
            "frame variable arr_base",
            substrs=[
                "(INTEGER(:), allocatable)",
                "([1] = 10, [2] = 20, [3] = 30, [4] = 40, [5] = 50,",
                "[6] = 60, [7] = 70, [8] = 80, [9] = 90, [10] = 100)",
            ],
        )

    def test_multi_dim_allocatable_arrays(self):
        """Tests that multi dimensional allocatable arrays
        with no custom bounds or byte strides display properly."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here", self.main_source_file
        )
        frame = thread.GetFrameAtIndex(0)

        self.expect(
            "frame variable arr_3d_dynamic[-3]",
            substrs=[
                "(REAL(KIND=8)(5:7, -1:1), allocatable)",
                "arr_3d_dynamic[-3] = {",
                "[5] = ([-1] = 1.5, [0] = 19.5, [1] = 37.5)",
                "[6] = ([-1] = 7.5, [0] = 25.5, [1] = 43.5)",
                "[7] = ([-1] = 13.5, [0] = 31.5, [1] = 49.5)",
                "}",
            ],
        )

        dyn_element = frame.GetValueForVariablePath("arr_3d_dynamic[0][6][0]")
        self.assertTrue(dyn_element.IsValid())
        self.assertAlmostEqual(float(dyn_element.GetValue()), 2.71828, places=5)

        self.expect(
            "frame variable arr_7d_cursed[1][1]",
            error=True,
            substrs=["array index 1 is not valid"],
        )
        self.expect(
            "frame variable arr_7d_cursed[-11][-3]",
            error=True,
            substrs=["array index -11 is not valid"],
        )

        self.expect_var_path(
            "arr_7d_cursed[-1][1][-2][0][-3][2][-4]", type="INTEGER(KIND=1)", value="42"
        )
        self.expect_var_path(
            "arr_7d_cursed[0][2][-1][1][-2][3][-3]",
            type="INTEGER(KIND=1)",
            value="-128",
        )

        self.expect(
            "frame variable arr_7d_cursed[-1][-3]",
            substrs=[
                "(INTEGER(KIND=1)(-2:-1, 0:1, -3:-2, 2:3, -4:-3))",
                "arr_7d_cursed[-1][-3] = {",
                "[-2] = {",
                "[0] = {",
                "[-3] = {",
                "[2] = ([-4] = 42, [-3] = 64)",
                "[3] = ([-4] = 32, [-3] = 96)",
            ],
        )

    def test_stress_allocatable_bounds(self):
        """Tests that allocatable arrays that have not been allocated yet or
        have custom bounds are displayed correctly."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here", self.main_source_file
        )
        frame = thread.GetFrameAtIndex(0)

        arr_unallocated = frame.FindVariable("arr_unallocated")
        self.assertTrue(arr_unallocated.IsValid())
        self.assertEqual(arr_unallocated.GetNumChildren(), 0)
        self.expect(
            "frame variable arr_unallocated[1]",
            error=True,
            substrs=["array index 1 is not valid"],
        )

        arr_1d_zero_size = frame.FindVariable("arr_1d_zero_size")
        self.assertTrue(arr_1d_zero_size.IsValid())
        self.assertEqual(arr_1d_zero_size.GetNumChildren(), 0)
        self.expect(
            "frame variable arr_1d_zero_size[1]",
            error=True,
            substrs=["array index 1 is not valid"],
        )

        val_neg_10 = frame.GetValueForVariablePath("arr_1d_neg_bounds[-10]")
        self.assertTrue(val_neg_10.IsValid())
        self.assertAlmostEqual(float(val_neg_10.GetValue()), -999.9, places=4)

        val_neg_5 = frame.GetValueForVariablePath("arr_1d_neg_bounds[-5]")
        self.assertTrue(val_neg_5.IsValid())
        self.assertAlmostEqual(float(val_neg_5.GetValue()), 555.5, places=4)

        self.expect(
            "frame variable arr_2d_weird_bounds",
            substrs=[
                "[-2] = ([0] = false)",
                "[-1] = ([0] = true)",
                "[0] = ([0] = true)",
                "[1] = ([0] = true)",
                "[2] = ([0] = false)",
            ],
        )
        self.expect_var_path("arr_2d_weird_bounds[0][0]", type="LOGICAL", value="true")

    def test_stress_allocatable_strides(self):
        """Tests that allocatable arrays that have custom byte strides
        are displayed correctly."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here", self.main_source_file
        )

        self.expect(
            "frame variable arr_stride_2",
            substrs=[
                "(INTEGER(:))",
                "([1] = 10, [2] = 30, [3] = 50, [4] = 70, [5] = 90)",
            ],
        )
        self.expect(
            "frame variable arr_stride_reverse",
            substrs=[
                "(INTEGER(:))",
                "([1] = 100, [2] = 90, [3] = 80, [4] = 70, [5] = 60,",
                "[6] = 50, [7] = 40, [8] = 30, [9] = 20, [10] = 10)",
            ],
        )
