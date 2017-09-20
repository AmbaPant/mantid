# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import (ReflectometryILLPreprocess)
import numpy.testing
from testhelpers import (assertRaisesNothing, create_algorithm)
import unittest

class ReflectometryILLPreprocessTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def testDefaultRunExecutesSuccessfully(self):
        outWSName = 'outWS'
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'OutputWorkspace': outWSName,
            'rethrow': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)

    def testFlatBackgroundSubtraction(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        args = {
            'OutputWorkspace': inWSName,
            'Function': 'Flat background',
            'NumBanks': 1,
        }
        alg = create_algorithm('CreateSampleWorkspace', **args)
        alg.execute()
        # Add a peak to the sample workspace.
        ws = mtd[inWSName]
        ys = ws.dataY(49)
        ys += 10.0
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'ForegroundCentre': 50,
            'FluxNormalisation': 'Normalisation OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 100)
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            if i != 49:
                numpy.testing.assert_equal(ys, 0)
            else:
                numpy.testing.assert_equal(ys, 10)

    def testForegroundBackgroundRanges(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        args = {
            'OutputWorkspace': inWSName,
            'Function': 'Flat background',
            'NumBanks': 1,
        }
        alg = create_algorithm('CreateSampleWorkspace', **args)
        alg.execute()
        ws = mtd[inWSName]
        # Add special background fitting zones around the exclude zones.
        upperBkgIndices = [26]
        for i in upperBkgIndices:
            ys = ws.dataY(i)
            ys += 5.0
        # Add negative 'exclude zone' around the peak.
        upperExclusionIndices = [27, 28]
        for i in upperExclusionIndices:
            ys = ws.dataY(i)
            ys -= 1000.0
        # Add a peak to the sample workspace.
        foregroundIndices = [29, 30, 31]
        for i in foregroundIndices:
            ys = ws.dataY(i)
            ys += 1000.0
        # The second exclusion zone is wider.
        lowerExclusionIndices = [32, 33, 34]
        for i in lowerExclusionIndices:
            ys = ws.dataY(i)
            ys -= 1000.0
        # The second fittin zone is wider.
        lowerBkgIndices = [35, 36]
        for i in lowerBkgIndices:
            ys = ws.dataY(i)
            ys += 5.0
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'SumOutput': 'Summation OFF',
            'ForegroundCentre': 31,
            'ForegroundHalfWidth': 1,
            'LowerBackgroundOffset': len(lowerExclusionIndices),
            'LowerBackgroundWidth': len(lowerBkgIndices),
            'UpperBackgroundOffset': len(upperExclusionIndices),
            'UpperBackgroundWidth': len(upperBkgIndices),
            'FluxNormalisation': 'Normalisation OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 100)
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            if i in upperBkgIndices:
                numpy.testing.assert_equal(ys, 0)
            elif i in upperExclusionIndices:
                numpy.testing.assert_equal(ys, -1005)
            elif i in foregroundIndices:
                numpy.testing.assert_equal(ys, 995)
            elif i in lowerExclusionIndices:
                numpy.testing.assert_equal(ys, -1005)
            elif i in lowerBkgIndices:
                numpy.testing.assert_equal(ys, 0)
            else:
                numpy.testing.assert_equal(ys, -5)

if __name__ == "__main__":
    unittest.main()
