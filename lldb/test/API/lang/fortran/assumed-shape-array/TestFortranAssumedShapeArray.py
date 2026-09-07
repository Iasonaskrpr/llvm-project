"""
Tests that arrays with assumed-shape display correctly and the
array descriptor passed is read properly.
"""

import lldb
import lldbsuite.test.lldbutil as lldbutil
from lldbsuite.test.lldbtest import *


class FortranTestAssumedShapeArrays(TestBase):

    def setUp(self):
        super().setUp()
        self.build()
        self.main_source_file = lldb.SBFileSpec("assumed_shape_array.f90")

    def test_single_dim_assumed_shape_arrays(self):
        """Test that single dimension assumed-shape arrays display correctly."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here 1", self.main_source_file
        )
        self.expect(
            "frame variable arr",
            substrs=[
                "(INTEGER(:)) arr = ([1] = 10, [2] = 20, [3] = 30, [4] = 40, [5] = 50)",
            ],
        )

        self.expect(
            "frame variable arr[1]",
            substrs=[
                "(INTEGER) arr[1] = 10",
            ],
        )
        self.expect(
            "frame variable arr[3]",
            substrs=[
                "(INTEGER) arr[3] = 30",
            ],
        )

    def test_custom_bounds_assumed_shape_arrays(self):
        """Test that single dimension assumed-shape arrays with custom bounds
        display correctly."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here 2", self.main_source_file
        )
        self.expect(
            "frame variable arr",
            substrs=[
                "(INTEGER(:)) arr = ([0] = 10, [1] = 20, [2] = 30, [3] = 40, [4] = 50)",
            ],
        )

        self.expect(
            "frame variable arr[0]",
            substrs=[
                "(INTEGER) arr[0] = 10",
            ],
        )
        self.expect(
            "frame variable arr[3]",
            substrs=[
                "(INTEGER) arr[3] = 40",
            ],
        )

    def test_multi_dim_assumed_shape_arrays(self):
        """Test that multi dimensional sliced assumed-shape arrays
        display correctly."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here 3", self.main_source_file
        )
        self.expect(
            "frame variable arr",
            substrs=[
                "(INTEGER(:, :)) arr = {",
                "[1] = ([1] = 11, [2] = 12)",
                "[2] = ([1] = 31, [2] = 32)",
                "}",
            ],
        )

        self.expect(
            "frame variable arr[1]",
            substrs=[
                "(INTEGER(2)) arr[1] = ([1] = 11, [2] = 12)",
            ],
        )
        self.expect(
            "frame variable arr[1][1]",
            substrs=[
                "(INTEGER) arr[1][1] = 11",
            ],
        )
        self.expect(
            "frame variable arr[2][1]",
            substrs=[
                "(INTEGER) arr[2][1] = 31",
            ],
        )
        self.expect(
            "frame variable arr[2][2]",
            substrs=[
                "(INTEGER) arr[2][2] = 32",
            ],
        )
