"""
Tests that arrays with no runtime information display elements correctly, with the proper bounds 
"""

import lldb
import lldbsuite.test.lldbutil as lldbutil
from lldbsuite.test.lldbtest import *


class FortranTestComplex(TestBase):

    def test_fortran_explicit_arrays(self):
        """Tests that explicit arrays display the elements as expected"""
        self.build()
        self.main_source_file = lldb.SBFileSpec("explicit.f90")
        (target, process, thread, bkpt) = lldbutil.run_to_source_breakpoint(
            self, "! Break here", self.main_source_file
        )

        frame = thread.GetFrameAtIndex(0)

        # ---------------------------------------------------------
        # 1D Default Array
        # ---------------------------------------------------------
        # Method 2: Testing the entire array structure in one go using children
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
            ]
        )

        # ---------------------------------------------------------
        # 1D Custom Array (-5 to 5)
        # ---------------------------------------------------------
        # Method 2: Verifying the custom negative bounds directly via children
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
            ]
        )

        # ---------------------------------------------------------
        # 2D Default Array
        # ---------------------------------------------------------
        self.expect_var_path("arr_2d_default", type="LOGICAL(3, 3)")
        
        # Testing a 1D slice of the 2D array to prove nested synthetic bounds
        self.expect_var_path(
            "arr_2d_default[1]",
            type="LOGICAL(3)",
            children=[
                ValueCheck(name="[1]", value="true"),
                ValueCheck(name="[2]", value="true"),
                ValueCheck(name="[3]", value="false"),
            ]
        )

        # ---------------------------------------------------------
        # 3D Mixed Array
        # ---------------------------------------------------------
        self.expect_var_path("arr_3d_mixed", type="REAL(KIND=8)(0:2, -3:-1, 4:5)")
        
        arr_3d_mixed = frame.FindVariable("arr_3d_mixed")
        first_element = arr_3d_mixed.GetChildAtIndex(0).GetChildAtIndex(0).GetChildAtIndex(0)
        
        # Check value (using "in" to avoid strict float formatting mismatches)
        self.assertTrue(first_element.IsValid(), "Failed to fetch first_element")
        actual_val = float(first_element.GetValue())
        self.assertAlmostEqual(actual_val, 3.14159, places=5, 
                               msg=f"Expected ~3.14159, got {actual_val}")
        last_3d_element = arr_3d_mixed.GetChildAtIndex(2).GetChildAtIndex(2).GetChildAtIndex(1)
        self.assertTrue(last_3d_element.IsValid(), "Failed to fetch last 3d element")
        actual_last_val = float(last_3d_element.GetValue())
        self.assertAlmostEqual(actual_last_val, 2.71828, places=5, 
                               msg=f"Expected ~2.71828, got {actual_last_val}")
        # ---------------------------------------------------------
        # 7D Array 
        # ---------------------------------------------------------
        self.expect_var_path("arr_7d", type="INTEGER(KIND=1)(2, 2, 2, 2, 2, 2, 2)")
        
        # Method 1: Deep drill-down using standard path brackets
        # Fetches arr_7d(2, 2, 2, 2, 2, 2, 2) in a single clean string
        self.expect_var_path(
            "arr_7d[2][2][2][2][2][2][2]",
            type="INTEGER(KIND=1)",
            value="127"
        )
        self.expect_var_path(
            "arr_7d[1][1][1][1][1][1][1]",
            type="INTEGER(KIND=1)",
            value="1"
        )