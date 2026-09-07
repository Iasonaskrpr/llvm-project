"""
Tests that arrays with assumed-size (*) display correctly.
"""

import lldb
import lldbsuite.test.lldbutil as lldbutil
from lldbsuite.test.lldbtest import *


class FortranTestAssumedSizeArrays(TestBase):

    def setUp(self):
        super().setUp()
        self.build()
        self.main_source_file = lldb.SBFileSpec("assumed_size_array.f90")

    def test_single_dim_assumed_size_arrays(self):
        """Test frame variable works on 1d assumed-size arrays."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here 1", self.main_source_file
        )
        self.expect(
            "frame variable arr",
            substrs=[
                "(INTEGER(*)) arr =",
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

    def test_multi_dim_assumed_size_arrays(self):
        """Test frame variable works on multidimensional assumed-size arrays."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here 2", self.main_source_file
        )
        self.expect(
            "frame variable arr",
            substrs=[
                "(INTEGER(3, *)) arr =",
            ],
        )

        self.expect(
            "frame variable arr[1]",
            substrs=[
                "(INTEGER(*)) arr[1] =",
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
                "(INTEGER) arr[2][1] = 21",
            ],
        )
        self.expect(
            "frame variable arr[2][2]",
            substrs=[
                "(INTEGER) arr[2][2] = 22",
            ],
        )
        self.expect(
            "frame variable arr[3][2]",
            substrs=[
                "(INTEGER) arr[3][2] = 32",
            ],
        )

    def test_multi_dim_custom_bounds_assumed_size_arrays(self):
        """Test frame variable works on multidimensional assumed-size arrays with custom bounds."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here 3", self.main_source_file
        )
        self.expect(
            "frame variable arr",
            substrs=[
                "(INTEGER(-2:1, *)) arr =",
            ],
        )

        self.expect(
            "frame variable arr[-1]",
            substrs=[
                "(INTEGER(*)) arr[-1] =",
            ],
        )
        self.expect(
            "frame variable arr[-2][1]",
            substrs=[
                "(INTEGER) arr[-2][1] = 11",
            ],
        )
        self.expect(
            "frame variable arr[0][1]",
            substrs=[
                "(INTEGER) arr[0][1] = 31",
            ],
        )
        self.expect(
            "frame variable arr[1][2]",
            substrs=[
                "(INTEGER) arr[1][2] = 23",
            ],
        )
        self.expect(
            "frame variable arr[0][3]",
            substrs=[
                "(INTEGER) arr[0][3] = 24",
            ],
        )
