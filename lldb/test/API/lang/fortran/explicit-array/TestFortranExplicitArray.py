"""
Tests that arrays with no runtime information display elements correctly,
with the proper bounds.
"""

import lldb
import lldbsuite.test.lldbutil as lldbutil
from lldbsuite.test.lldbtest import *


class FortranTestExplicitArrays(TestBase):

    def test_simple_explicit_arrays(self):
        """Test that explicit arrays with no custom bounds display properly."""
        self.build()
        self.main_source_file = lldb.SBFileSpec("explicit.f90")
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here", self.main_source_file
        )
        frame = thread.GetFrameAtIndex(0)

        self.expect_var_path(
            "arr_1d_default",
            type="INTEGER(10)",
            children=[
                ValueCheck(name="[1]", value="100"),
                ValueCheck(name="[2]", value="2"),
                ValueCheck(name="[3]", value="3"),
                ValueCheck(name="[4]", value="4"),
                ValueCheck(name="[5]", value="5"),
                ValueCheck(name="[6]", value="6"),
                ValueCheck(name="[7]", value="7"),
                ValueCheck(name="[8]", value="8"),
                ValueCheck(name="[9]", value="9"),
                ValueCheck(name="[10]", value="999"),
            ],
        )

        self.expect(
            "frame variable arr_2d_default",
            substrs=[
                "(LOGICAL(3, 3))",
                "arr_2d_default = {",
                "[1] = ([1] = true, [2] = true, [3] = false)",
                "[2] = ([1] = true, [2] = false, [3] = true)",
                "[3] = ([1] = false, [2] = true, [3] = false)",
                "}",
            ],
        )

        self.expect(
            "frame variable arr_2d_default[1]",
            substrs=[
                "(LOGICAL(3))",
                "arr_2d_default[1] = ([1] = true, [2] = true, [3] = false)",
            ],
        )
        self.expect(
            "frame variable arr_2d_default[1][2]",
            substrs=["(LOGICAL)", "arr_2d_default[1][2] = true"],
        )

        self.expect_var_path(
            "arr_2d_default[1]",
            type="LOGICAL(3)",
            children=[
                ValueCheck(name="[1]", value="true"),
                ValueCheck(name="[2]", value="true"),
                ValueCheck(name="[3]", value="false"),
            ],
        )

        self.expect_var_path("arr_7d", type="INTEGER(KIND=1)(2, 2, 2, 2, 2, 2, 2)")
        self.expect_var_path(
            "arr_7d[2][2][2][2][2][2][2]", type="INTEGER(KIND=1)", value="127"
        )
        self.expect_var_path(
            "arr_7d[1][1][1][1][1][1][1]", type="INTEGER(KIND=1)", value="1"
        )

    def test_stress_explicit_bounds(self):
        """Test that explicit arrays with custom bounds display properly."""
        self.build()
        self.main_source_file = lldb.SBFileSpec("explicit.f90")
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here", self.main_source_file
        )
        frame = thread.GetFrameAtIndex(0)

        self.expect_var_path(
            "arr_1d_custom",
            type="REAL(-5:5)",
            children=[
                ValueCheck(name="[-5]", value="-5.5"),
                ValueCheck(name="[-4]", value="-3.5"),
                ValueCheck(name="[-3]", value="-2.5"),
                ValueCheck(name="[-2]", value="-1.5"),
                ValueCheck(name="[-1]", value="-0.5"),
                ValueCheck(name="[0]", value="0"),
                ValueCheck(name="[1]", value="1.5"),
                ValueCheck(name="[2]", value="2.5"),
                ValueCheck(name="[3]", value="3.5"),
                ValueCheck(name="[4]", value="4.5"),
                ValueCheck(name="[5]", value="5.5"),
            ],
        )

        self.expect_var_path("arr_3d_mixed", type="REAL(KIND=8)(0:2, -3:-1, 4:5)")

        arr_3d_mixed = frame.FindVariable("arr_3d_mixed")
        first_element = (
            arr_3d_mixed.GetChildAtIndex(0).GetChildAtIndex(0).GetChildAtIndex(0)
        )

        self.assertTrue(first_element.IsValid())
        self.assertAlmostEqual(float(first_element.GetValue()), 3.14159, places=5)

        last_3d_element = (
            arr_3d_mixed.GetChildAtIndex(2).GetChildAtIndex(2).GetChildAtIndex(1)
        )
        self.assertTrue(last_3d_element.IsValid())
        self.assertAlmostEqual(float(last_3d_element.GetValue()), 2.71828, places=5)

        self.expect(
            "frame variable arr_3d_mixed[0][-2]",
            substrs=[
                "(REAL(KIND=8)(4:5))",
                "arr_3d_mixed[0][-2] = ([4] = 4.0999999999999996, [5] = 13.1)",
            ],
        )
        self.expect(
            "frame variable arr_3d_mixed[0][-2][5]",
            substrs=["(REAL(KIND=8))", "arr_3d_mixed[0][-2][5] = 13.1"],
        )
