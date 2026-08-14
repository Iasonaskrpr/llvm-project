"""
Tests that arrays that take function arguments as parameters work correctly.
"""

import lldb
import lldbsuite.test.lldbutil as lldbutil
from lldbsuite.test.lldbtest import *


class FortranTestAutomaticArrays(TestBase):

    def setUp(self):
        super().setUp()
        self.build()
        self.main_source_file = lldb.SBFileSpec("automatic_array.f90")

    def test_single_dim_automatic_arrays(self):
        """Test frame variable works on 1d automatic arrays."""
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

    def test_multi_dim_automatic_arrays(self):
        """Test frame variable works on multi-dimensional automatic arrays with custom bounds."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here 2", self.main_source_file
        )
        self.expect(
            "frame variable arr",
            substrs=[
                "(INTEGER(:, :, :)) arr = {",
                "[3] = {",
                "[0] = ([1] = 301, [2] = 302, [3] = 303, [4] = 304, [5] = 305)",
                "[4] = ([1] = 341, [2] = 342, [3] = 343, [4] = 344, [5] = 345)",
                "}",
                "[12] = {",
                "[0] = ([1] = 1201, [2] = 1202, [3] = 1203, [4] = 1204, [5] = 1205)",
                "[4] = ([1] = 1241, [2] = 1242, [3] = 1243, [4] = 1244, [5] = 1245)",
                "}",
                "}",
            ],
        )
        self.expect(
            "frame variable arr[5]",
            substrs=[
                "(INTEGER(0:4, 5)) arr[5] = {",
                "[0] = ([1] = 501, [2] = 502, [3] = 503, [4] = 504, [5] = 505)",
                "[1] = ([1] = 511, [2] = 512, [3] = 513, [4] = 514, [5] = 515)",
                "[2] = ([1] = 521, [2] = 522, [3] = 523, [4] = 524, [5] = 525)",
                "[3] = ([1] = 531, [2] = 532, [3] = 533, [4] = 534, [5] = 535)",
                "[4] = ([1] = 541, [2] = 542, [3] = 543, [4] = 544, [5] = 545)",
                "}",
            ],
        )

        self.expect(
            "frame variable arr[8][4][4]",
            substrs=[
                "(INTEGER) arr[8][4][4] = 844",
            ],
        )
        self.expect(
            "frame variable arr[12][2][3]",
            substrs=[
                "(INTEGER) arr[12][2][3] = 1223",
            ],
        )
