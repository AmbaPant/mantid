from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import (AddSampleLog, CloneWorkspace, CreateWorkspace,
                              DeleteWorkspace, LoadEmptyInstrument,
                              MaskDetectors, mtd, RemoveMaskedSpectra)
import numpy
import numpy.testing
from scipy import constants
from testhelpers import run_algorithm
import unittest


def _timeOfFlight(energy, length):
    """Calculate the time-flight from energy and flight length."""
    velocity = numpy.sqrt(2 * energy * 1e-3 * constants.e / constants.m_n)
    return length / velocity * 1e6


def _wavelength(energy):
    """Calculate the wavelength from energy."""
    velocity = numpy.sqrt(2 * energy * 1e-3 * constants.e / constants.m_n)
    return constants.h / (velocity * constants.m_n) * 1e10


def _gaussian(x, height, x0, sigma):
    """Return a point in the gaussian curve."""
    x = x - x0
    sigma2 = 2 * sigma * sigma
    return height * numpy.exp(- x * x / sigma2)


def _fillTemplateWorkspace(templateWS):
    """Fill a workspace with somewhat sane data."""
    nHistograms = templateWS.getNumberHistograms()
    E_i = 23.0
    nBins = 128
    binWidth = 2.63
    elasticIndex = int(nBins / 3)
    monitorElasticIndex = int(nBins / 2)
    xs = numpy.empty(nHistograms*(nBins+1))
    ys = numpy.empty(nHistograms*nBins)
    es = numpy.empty(nHistograms*nBins)
    instrument = templateWS.getInstrument()
    sample = instrument.getSample()
    l1 = sample.getDistance(instrument.getSource())
    l2 = float(instrument.getStringParameter('l2')[0])
    tofElastic = _timeOfFlight(E_i, l1+l2)
    tofBegin = tofElastic - elasticIndex * binWidth
    monitor = instrument.getDetector(0)
    monitorSampleDistance = sample.getDistance(monitor)
    tofElasticMonitor = tofBegin + monitorElasticIndex * binWidth
    tofMonitorDetector = _timeOfFlight(E_i, monitorSampleDistance+l2)
    elasticPeakSigma = nBins * binWidth * 0.03
    elasticPeakHeight = 1723.0
    bkg = 2
    bkgMonitor = 1

    def fillBins(histogramIndex, elasticTOF, elasticPeakHeight, bkg):
        xIndexOffset = histogramIndex*(nBins+1)
        yIndexOffset = histogramIndex*nBins
        xs[xIndexOffset] = tofBegin - binWidth / 2
        for binIndex in range(nBins):
            x = tofBegin + binIndex * binWidth
            xs[xIndexOffset+binIndex+1] = x + binWidth / 2
            y = round(_gaussian(x, elasticPeakHeight, elasticTOF,
                                elasticPeakSigma)) + bkg
            ys[yIndexOffset+binIndex] = y
            es[yIndexOffset+binIndex] = numpy.sqrt(y)

    fillBins(0, tofElasticMonitor, 1623 * elasticPeakHeight, bkgMonitor)
    for histogramIndex in range(1, nHistograms):
        trueL2 = sample.getDistance(templateWS.getDetector(histogramIndex))
        trueTOF = _timeOfFlight(E_i, l1+trueL2)
        fillBins(histogramIndex, trueTOF, elasticPeakHeight, bkg)
    ws = CreateWorkspace(DataX=xs,
                         DataY=ys,
                         DataE=es,
                         NSpec=nHistograms,
                         ParentWorkspace=templateWS)
    ws.getAxis(0).setUnit('TOF')
    AddSampleLog(Workspace=ws,
                 LogName='Ei',
                 LogText=str(E_i),
                 LogType='Number',
                 NumberType='Double')
    AddSampleLog(Workspace=ws,
                 LogName='wavelength',
                 LogText=str(float(_wavelength(E_i))),
                 LogType='Number',
                 NumberType='Double')
    pulseInterval = \
        tofMonitorDetector + (monitorElasticIndex - elasticIndex) * binWidth
    AddSampleLog(Workspace=ws,
                 LogName='pulse_interval',
                 LogText=str(float(pulseInterval * 1e-6)),
                 LogType='Number',
                 NumberType='Double')
    AddSampleLog(Workspace=ws,
                 LogName='Detector.elasticpeak',
                 LogText=str(elasticIndex),
                 LogType='Number',
                 NumberType='Int')
    return ws


class DirectILLReductionTest(unittest.TestCase):

    _TEMPLATE_IN5_WS_NAME = '__IN5templateWS'

    def setUp(self):
        self._testIN5WS = CloneWorkspace(InputWorkspace=self._in5WStemplate,
                                         OutputWorkspace='__IN5testWS')

    @classmethod
    def setUpClass(cls):
        cls._in5WStemplate = \
            LoadEmptyInstrument(InstrumentName='IN5',
                                OutputWorkspace=cls._TEMPLATE_IN5_WS_NAME)
        mask = list()
        for i in range(513):
            if i % 10 != 0:
                mask.append(i)
        MaskDetectors(Workspace=cls._in5WStemplate, DetectorList=mask)
        MaskDetectors(Workspace=cls._in5WStemplate, StartWorkspaceIndex=512)
        cls._in5WStemplate = \
            RemoveMaskedSpectra(InputWorkspace=cls._in5WStemplate,
                                OutputWorkspace=cls._in5WStemplate)
        cls._in5WStemplate = _fillTemplateWorkspace(cls._in5WStemplate)

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def test_component_mask(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'MaskedComponents': 'tube_1',  # Mask workspace indices 0-31
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        outWS = mtd[outWSName]
        self.assertTrue(self._checkAlgorithmsInHistory(outWS, 'MaskDetectors'))
        nHistograms = outWS.getNumberHistograms()
        for i in range(int(nHistograms / 2)):
            self.assertTrue(outWS.getDetector(i).isMasked())
        for i in range(int(nHistograms / 2), nHistograms):
            self.assertFalse(outWS.getDetector(i).isMasked())
        DeleteWorkspace(outWSName)

    def test_det_diagnostics_bad_elastic_intensity(self):
        nHistograms = self._testIN5WS.getNumberHistograms()
        noPeakIndices = [1, int(nHistograms / 6)]
        highPeakIndices = [int(nHistograms / 3), nHistograms - 1]
        for i in noPeakIndices:
            ys = self._testIN5WS.dataY(i)
            ys *= 0.01
        for i in highPeakIndices:
            ys = self._testIN5WS.dataY(i)
            ys *= 5
        outWSName = 'outWS'
        diagnosticsWSName = 'diagnosticsWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'DetectorsAtL2': '12, 38',
            'ElasticPeakDiagnosticsLowThreshold': 0.1,
            'ElasticPeakDiagnosticsHighThreshold': 4.8,
            'OutputDiagnosticsWorkspace': diagnosticsWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        diagnosticsWS = mtd[diagnosticsWSName]
        self._checkDiagnosticsAlgorithmsInHistory(mtd[outWSName],
                                                  diagnosticsWS)
        self.assertEqual(diagnosticsWS.getNumberHistograms(),
                         nHistograms - 1)
        self.assertEqual(diagnosticsWS.blocksize(), 1)
        shouldBeMasked = noPeakIndices + highPeakIndices
        for i in range(diagnosticsWS.getNumberHistograms()):
            originalI = i + 1  # Monitor has been extracted.
            if originalI in shouldBeMasked:
                self.assertEqual(diagnosticsWS.readY(i)[0], 1.0,
                                 ('Detector at ws index {0} should be ' +
                                  'masked').format(i))
            else:
                self.assertEqual(diagnosticsWS.readY(i)[0], 0.0)
        DeleteWorkspace(outWSName)
        DeleteWorkspace(diagnosticsWS)

    def test_det_diagnostics_no_bad_detectors(self):
        outWSName = 'outWS'
        diagnosticsWSName = 'diagnostics'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'DetectorsAtL2': '12, 38',
            'OutputDiagnosticsWorkspace': diagnosticsWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        diagnosticsWS = mtd[diagnosticsWSName]
        self._checkDiagnosticsAlgorithmsInHistory(mtd[outWSName],
                                                  diagnosticsWS)
        self.assertEqual(diagnosticsWS.getNumberHistograms(),
                         self._testIN5WS.getNumberHistograms() - 1)
        self.assertEqual(diagnosticsWS.blocksize(), 1)
        for i in range(diagnosticsWS.getNumberHistograms()):
            self.assertEqual(diagnosticsWS.readY(i)[0], 0.0)
        DeleteWorkspace(outWSName)
        DeleteWorkspace(diagnosticsWS)

    def test_det_diagnostics_noisy_background(self):
        nHistograms = self._testIN5WS.getNumberHistograms()
        noisyWSIndices = [1, int(nHistograms / 5), nHistograms - 1]
        for i in noisyWSIndices:
            ys = self._testIN5WS.dataY(i)
            ys *= 2
        outWSName = 'outWS'
        diagnosticsWSName = 'diagnostics'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'DetectorsAtL2': '12, 38',
            'NoisyBkgDiagnosticsHighThreshold': 1.9,
            'OutputDiagnosticsWorkspace': diagnosticsWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        diagnosticsWS = mtd[diagnosticsWSName]
        self._checkDiagnosticsAlgorithmsInHistory(mtd[outWSName],
                                                  diagnosticsWS)
        self.assertEqual(diagnosticsWS.getNumberHistograms(),
                         nHistograms - 1)
        self.assertEqual(diagnosticsWS.blocksize(), 1)
        for i in range(diagnosticsWS.getNumberHistograms()):
            originalI = i + 1  # Monitor has been extracted.
            if originalI in noisyWSIndices:
                self.assertEqual(diagnosticsWS.readY(i)[0], 1.0)
            else:
                self.assertEqual(diagnosticsWS.readY(i)[0], 0.0)
        DeleteWorkspace(outWSName)
        DeleteWorkspace(diagnosticsWSName)

    def test_det_diagnostics_output_when_disabled(self):
        outWSName = 'outWS'
        diagnosticsWSName = 'diagnosticsWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'OutputDiagnosticsWorkspace': diagnosticsWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertFalse(mtd.doesExist(diagnosticsWSName))

    def test_empty_container_subtraction(self):
        outECWSName = 'outECWS'
        ecWS = CloneWorkspace(self._testIN5WS)  # Empty container ws.
        for i in range(ecWS.getNumberHistograms() - 1):
            ys = ecWS.dataY(i + 1)
            ys *= 0.1
        algProperties = {
            'InputWorkspace': ecWS,
            'OutputWorkspace': outECWSName,
            'Cleanup': 'Delete Intermediate Workspaces',
            'ReductionType': 'Empty Container',
            'Normalisation': 'No Normalisation',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'rethrow': True
        }
        try:
            run_algorithm('DirectILLReduction', **algProperties)
        except:
            self.fail('Algorithm threw an exception.')
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'Cleanup': 'Keep Intermediate Workspaces',
            'ReductionType': 'Vanadium',
            'IndexType': 'Workspace Index',
            'DetectorsAtL2': '12, 38',
            'Normalisation': 'No Normalisation',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'rethrow': True
        }
        try:
            run_algorithm('DirectILLReduction', **algProperties)
        except:
            self.fail('Algorithm threw an exception.')
        self.assertFalse(mtd.doesExist(outWSName + '_ecScaling'))
        self.assertFalse(mtd.doesExist(outWSName + '_scaled_EC'))
        self.assertFalse(mtd.doesExist(outWSName + '_EC_subtracted'))
        outECSubtractedWSName = 'outECSubtractedWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outECSubtractedWSName,
            'Cleanup': 'Keep Intermediate Workspaces',
            'ReductionType': 'Vanadium',
            'Normalisation': 'No Normalisation',
            'IndexType': 'Workspace Index',
            'DetectorsAtL2': '12, 38',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'EmptyContainerWorkspace': outECWSName,
            'EmptyContainerScalingFactor': 0.75,
            'rethrow': True
        }
        try:
            run_algorithm('DirectILLReduction', **algProperties)
        except:
            self.fail('Algorithm threw and exception')
        self.assertTrue(mtd.doesExist(outECSubtractedWSName + '_ecScaling'))
        self.assertTrue(mtd.doesExist(outECSubtractedWSName + '_scaled_EC'))
        self.assertTrue(mtd.doesExist(outECSubtractedWSName +
                                      '_EC_subtracted'))
        DeleteWorkspace(ecWS)
        DeleteWorkspace(outWSName)
        DeleteWorkspace(outECSubtractedWSName)
        DeleteWorkspace(outECWSName)

    def test_energy_rebinning_manual_mode(self):
        rebinningBegin = -2.3
        rebinningEnd = 4.2
        binWidth = 0.66
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'Reductiontype': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'Transposing': 'No Sample Output Transposing',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'IndexType': 'Workspace Index',
            'DetectorsAtL2': '12, 38',
            'EnergyRebinningMode': 'Manual Energy Rebinning',
            'EnergyRebinningParams': '{0}, {1}, {2}'.format(rebinningBegin,
                                                            binWidth,
                                                            rebinningEnd),
            'QBinningMode': 'Bin to Median 2Theta',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        ws = mtd[outWSName]
        self.assertTrue(self._checkAlgorithmsInHistory(ws, 'Rebin'))
        axis = ws.getAxis(0)
        self.assertTrue(axis.getUnit().name(), 'Energy transfer')
        for i in range(ws.getNumberHistograms()):
            xs = ws.readX(i)
            self.assertAlmostEqual(xs[0], rebinningBegin)
            self.assertAlmostEqual(xs[-1], rebinningEnd)
            for j in range(len(xs) - 2):
                # Skip the last bin which is smaller.
                self.assertAlmostEqual(xs[j+1] - xs[j], binWidth)

    def test_energy_rebinning_manul_mode_fails_without_params(self):
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'Reductiontype': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'Transposing': 'No Sample Output Transposing',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'EnergyRebinningMode': 'Manual Energy Rebinning',
            'QBinningMode': 'Bin to Median 2Theta',
            'child': True,
            'rethrow': True
        }
        self.assertRaises(RuntimeError, run_algorithm, 'DirectILLReduction',
                          **algProperties)

    def test_energy_rebinning_to_elastic_bin_width(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'Transposing': 'No Sample Output Transposing',
            'IndexType': 'Workspace Index',
            'DetectorsAtL2': '12, 38',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'EnergyRebinningMode': 'Rebin to Bin Width at Elastic Peak',
            'QBinningMode': 'Bin to Median 2Theta',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        ws = mtd[outWSName]
        self.assertTrue(self._checkAlgorithmsInHistory(ws, 'BinWidthAtX',
                                                       'Rebin'))
        axis = ws.getAxis(0)
        self.assertTrue(axis.getUnit().name(), 'Energy transfer')
        binWidth = ws.readX(0)[1] - ws.readX(0)[0]
        xs = ws.extractX()[:, :-1]  # The last bin is smaller, ignoring.
        numpy.testing.assert_almost_equal(numpy.diff(xs), binWidth, decimal=5)
        DeleteWorkspace(ws)

    def test_energy_rebinning_to_median_bin_width(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'Transposing': 'No Sample Output Transposing',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'IndexType': 'Workspace Index',
            'DetectorsAtL2': '12, 38',
            'Diagnostics': 'No Detector Diagnostics',
            'EnergyRebinningMode': 'Rebin to Median Bin Width',
            'QBinningMode': 'Bin to Median 2Theta',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        ws = mtd[outWSName]
        self.assertTrue(self._checkAlgorithmsInHistory(ws, 'MedianBinWidth',
                                                       'Rebin'))
        axis = ws.getAxis(0)
        self.assertTrue(axis.getUnit().name(), 'Energy transfer')
        binWidth = ws.readX(0)[1] - ws.readX(0)[0]
        xs = ws.extractX()[:, :-1]  # The last bin is smaller, ignoring.
        numpy.testing.assert_almost_equal(numpy.diff(xs), binWidth, decimal=5)
        DeleteWorkspace(ws)

    def test_final_sample_output_workspace(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'IndexType': 'Workspace Index',
            'DetectorsAtL2': '12, 38',
            'Diagnostics': 'No Detector Diagnostics',
            'QBinningMode': 'Bin to Median 2Theta',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        ws = mtd[outWSName]
        self.assertTrue(self._checkAlgorithmsInHistory(ws,
                                                       'ConvertToPointData',
                                                       'Transpose'))
        axis = ws.getAxis(0)
        self.assertEqual(axis.getUnit().name(), 'q')
        axis = ws.getAxis(1)
        self.assertEqual(axis.getUnit().name(), 'Energy transfer')
        DeleteWorkspace(ws)

    def test_input_ws_not_deleted(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'Cleanup': 'Delete Intermediate Workspaces',
            'ReductionType': 'Empty Container',
            'Normalisation': 'No Normalisation',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        try:
            self._testIN5WS.getNumberHistograms()
        except RuntimeError:
            self.fail('Workspace used as InputWorkspace has been deleted.')
        finally:
            DeleteWorkspace(outWSName)

    def test_q_rebinning_auto_mode(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'IndexType': 'Workspace Index',
            'DetectorsAtL2': '12, 38',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'EnergyRebinningMode': 'Rebin to Bin Width at Elastic Peak',
            'QBinningMode': 'Bin to Median 2Theta',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        ws = mtd[outWSName]
        axis = ws.getAxis(0)
        self.assertTrue(axis.getUnit().name(), 'Momentum transfer')
        self.assertAlmostEquals(ws.readX(0)[0], 0.0)
        binWidth = ws.readX(0)[1] - ws.readX(0)[0]
        xs = ws.extractX()[:, :-1]  # The last bin may be smaller, ignoring.
        numpy.testing.assert_almost_equal(numpy.diff(xs), binWidth, decimal=5)
        DeleteWorkspace(ws)

    def test_q_rebinning_manual_mode(self):
        outWSName = 'outWS'
        qMin = 0.1
        qMax = 1.3
        qStep = 0.042
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'IndexType': 'Workspace Index',
            'DetectorsAtL2': '12, 38',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'EnergyRebinningMode': 'Rebin to Bin Width at Elastic Peak',
            'QBinningMode': 'Manual q Binning',
            'QBinningParams': '{0}, {1}, {2}'.format(qMin, qStep, qMax),
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        ws = mtd[outWSName]
        axis = ws.getAxis(0)
        self.assertTrue(axis.getUnit().name(), 'Momentum transfer')
        self.assertAlmostEquals(ws.readX(0)[0], qMin)
        self.assertAlmostEquals(ws.readX(0)[-1], qMax)
        xs = ws.extractX()[:, :-1]  # The last bin may be smaller, ignoring.
        numpy.testing.assert_almost_equal(numpy.diff(xs), qStep, decimal=5)
        DeleteWorkspace(ws)

    def test_q_rebinning_manual_mode_fails_without_params(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'IndexType': 'Workspace Index',
            'DetectorsAtL2': '12, 38',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'EnergyRebinningMode': 'Rebin to Bin Width at Elastic Peak',
            'QRebinningMode': 'Manual q Rebinning',
            'rethrow': True
        }
        self.assertRaises(RuntimeError, run_algorithm, 'DirectILLReduction',
                          **algProperties)

    def test_theta_energy_output_workspace(self):
        outWSName = 'outWS'
        thetaEnergyWSName = 'out_S_of_theta_energy'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Sample',
            'Cleanup': 'Delete Intermediate Workspaces',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'IndexType': 'Workspace Index',
            'DetectorsAtL2': '12, 38',
            'Diagnostics': 'No Detector Diagnostics',
            'QBinningMode': 'Bin to Median 2Theta',
            'OutputSofThetaEnergyWorkspace': thetaEnergyWSName,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        ws = mtd[outWSName]
        self.assertTrue(self._checkAlgorithmsInHistory(ws,
                                                       'ConvertSpectrumAxis'))
        self.assertTrue(mtd.doesExist(thetaEnergyWSName))
        thetaEnergyWS = mtd[thetaEnergyWSName]
        axis = thetaEnergyWS.getAxis(0)
        self.assertEqual(axis.getUnit().name(), 'Energy transfer')
        axis = thetaEnergyWS.getAxis(1)
        self.assertEqual(axis.getUnit().name(), 'Scattering angle')
        DeleteWorkspace(ws)
        DeleteWorkspace(thetaEnergyWS)

    def test_user_mask(self):
        numHistograms = self._testIN5WS.getNumberHistograms()
        userMask = [0, int(3 * numHistograms / 5), numHistograms - 2]
        maskString = '{0},'.format(userMask[0])
        for i in userMask:
            maskString += str(i) + ','
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Empty Container',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'MaskedDetectors': maskString,
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        outWS = mtd[outWSName]
        self.assertTrue(self._checkAlgorithmsInHistory(outWS, 'MaskDetectors'))
        for i in range(outWS.getNumberHistograms()):
            if i in userMask:
                self.assertTrue(outWS.getDetector(i).isMasked())
            else:
                self.assertFalse(outWS.getDetector(i).isMasked())
        DeleteWorkspace(outWSName)

    def test_vanadium_workflow(self):
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': outWSName,
            'ReductionType': 'Vanadium',
            'IndexType': 'Workspace Index',
            'Monitor': '0',
            'DetectorsAtL2': '12, 38',
            'IncidentEnergyCalibration': 'No Incident Energy Calibration',
            'Diagnostics': 'No Detector Diagnostics',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        outWS = mtd[outWSName]
        self.assertTrue(self._checkAlgorithmsInHistory(outWS,
                        'ComputeCalibrationCoefVan'))
        for i in range(outWS.getNumberHistograms()):
            self.assertAlmostEqual(outWS.readY(i)[0], 0.000765, 5)
        DeleteWorkspace(outWSName)

    def _checkAlgorithmsInHistory(self, ws, *args):
        """Return true if algorithm names listed in *args are found in the
        workspace's history.
        """
        history = ws.getHistory()
        reductionHistory = history.getAlgorithmHistory(history.size() - 1)
        algHistories = reductionHistory.getChildHistories()
        algNames = [alg.name() for alg in algHistories]
        for algName in args:
            return algName in algNames

    def _checkDiagnosticsAlgorithmsInHistory(self, outWS, diagnosticsWS):
        """Assert if the outWS and diagnosticsWS contain the correct
        algorithms.
        """
        self.assertTrue(self._checkAlgorithmsInHistory(outWS,
                                                       'MedianDetectorTest',
                                                       'MaskDetectors'))
        self.assertTrue(self._checkAlgorithmsInHistory(diagnosticsWS,
                                                       'MedianDetectorTest',
                                                       'ClearMaskFlag',
                                                       'Plus'))

if __name__ == '__main__':
    unittest.main()
