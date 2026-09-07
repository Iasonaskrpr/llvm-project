"""
Tests that assumed-rank arrays display correctly and the
array descriptor is read properly, as well as rank 0 arrays
are handled correctly.
"""

import lldb
import lldbsuite.test.lldbutil as lldbutil
from lldbsuite.test.lldbtest import *


class FortranTestAssumedRankArrays(TestBase):

    def setUp(self):
        super().setUp()
        self.build()
        self.main_source_file = lldb.SBFileSpec("assumed_rank.f90")

    def test_assumed_rank_arrays(self):
        """Tests that assumed-rank arrays of various ranks display properly."""
        target, process, thread, bkpt = lldbutil.run_to_source_breakpoint(
            self, "! Break here", self.main_source_file
        )

        self.expect(
            "frame variable scalar_val",
            substrs=[
                "(INTEGER(..)) scalar_val = (value = 42)",
            ],
        )

        self.expect(
            "frame variable vec",
            substrs=[
                "(INTEGER(..)) vec = ([1] = 10, [2] = 20, [3] = 30, [4] = 40)",
            ],
        )

        self.expect(
            "frame variable matrix",
            substrs=[
                "(INTEGER(..)) matrix = {",
                "[1] = ([1] = 1, [2] = 3, [3] = 5)",
                "[2] = ([1] = 2, [2] = 4, [3] = 6)",
                "}",
            ],
        )

        self.expect(
            "frame variable tensor[2]",
            substrs=[
                "(INTEGER(2, 2)) tensor[2] = {",
                "[1] = ([1] = 99, [2] = 11)",
                "[2] = ([1] = 99, [2] = 99)",
                "}",
            ],
        )
        self.expect(
            "frame variable tensor[2][1][2]",
            substrs=[
                "(INTEGER) tensor[2][1][2] = 11",
            ],
        )
