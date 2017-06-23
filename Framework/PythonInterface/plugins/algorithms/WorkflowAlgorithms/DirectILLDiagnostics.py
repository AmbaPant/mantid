# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
import mantid
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator,
                        ITableWorkspaceProperty, MatrixWorkspaceProperty, mtd, Progress, PropertyMode, WorkspaceProperty,
                        WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction, FloatBoundedValidator, IntArrayBoundedValidator,
                           IntArrayProperty, StringArrayProperty, StringListValidator)
from mantid.simpleapi import (ClearMaskFlag, CloneWorkspace, CreateEmptyTableWorkspace, CreateWorkspace, Divide,
                              ExtractMask, Integration, LoadMask, MaskDetectors, MedianDetectorTest, Plus, SolidAngle)
import numpy
import os.path

_PLOT_TYPE_X = 1
_PLOT_TYPE_Y = 2


class _DiagnosticsSettings:
    """A class to hold settings for MedianDetectorTest."""

    def __init__(self, lowThreshold, highThreshold, significanceTest):
        """Initialize an instance of the class."""
        self.lowThreshold = lowThreshold
        self.highThreshold = highThreshold
        self.significanceTest = significanceTest


def _bkgDiagnostics(bkgWS, noisyBkgSettings, wsNames, algorithmLogging):
    """Diagnose noisy flat backgrounds and return a mask workspace."""
    noisyBkgWSName = wsNames.withSuffix('diagnostics_noisy_bkg')
    noisyBkgDiagnostics, nFailures = \
        MedianDetectorTest(InputWorkspace=bkgWS,
                           OutputWorkspace=noisyBkgWSName,
                           SignificanceTest=noisyBkgSettings.significanceTest,
                           LowThreshold=noisyBkgSettings.lowThreshold,
                           HighThreshold=noisyBkgSettings.highThreshold,
                           LowOutlier=0.0,
                           EnableLogging=algorithmLogging)
    ClearMaskFlag(Workspace=noisyBkgDiagnostics,
                  EnableLogging=algorithmLogging)
    return noisyBkgDiagnostics


def _createDiagnosticsReportTable(reportWSName, numberHistograms, algorithmLogging):
    """Return a table workspace for detector diagnostics reporting."""
    if mtd.doesExist(reportWSName):
        reportWS = mtd[reportWSName]
    else:
        reportWS = CreateEmptyTableWorkspace(OutputWorkspace=reportWSName,
                                             EnableLogging=algorithmLogging)
    existingColumnNames = reportWS.getColumnNames()
    if 'WorkspaceIndex' not in existingColumnNames:
        reportWS.addColumn('int', 'WorkspaceIndex', _PLOT_TYPE_X)
    reportWS.setRowCount(numberHistograms)
    for i in range(numberHistograms):
        reportWS.setCell('WorkspaceIndex', i, i)
    return reportWS


def _createMaskWS(ws, wsNames, algorithmLogging):
    """Return a single bin workspace with same number of histograms as ws."""
    n = ws.getNumberHistograms()
    xs = numpy.zeros(n)
    ys = numpy.zeros(n)
    maskWSName = wsNames.withSuffix('mask_template')
    maskWS = CreateWorkspace(OutputWorkspace=maskWSName,
                             DataX=xs,
                             DataY=ys,
                             NSpec=n,
                             ParentWorkspace=ws,
                             EnableLogging=algorithmLogging)
    return maskWS


def _elasticPeakDiagnostics(ws, peakSettings, wsNames, algorithmLogging):
    """Diagnose elastic peaks and return a mask workspace"""
    elasticPeakDiagnosticsWSName = wsNames.withSuffix('diagnostics_elastic_peak')
    elasticPeakDiagnostics, nFailures = \
        MedianDetectorTest(InputWorkspace=ws,
                           OutputWorkspace=elasticPeakDiagnosticsWSName,
                           SignificanceTest=peakSettings.significanceTest,
                           LowThreshold=peakSettings.lowThreshold,
                           HighThreshold=peakSettings.highThreshold,
                           EnableLogging=algorithmLogging)
    ClearMaskFlag(Workspace=elasticPeakDiagnostics,
                  EnableLogging=algorithmLogging)
    return elasticPeakDiagnostics


def _extractUserMask(ws, wsNames, algorithmLogging):
    """Extracts user specified mask from a workspace."""
    maskWSName = wsNames.withSuffix('user_mask_extracted')
    maskWS, detectorIDs = ExtractMask(InputWorkspace=ws,
                                      OutputWorkspace=maskWSName,
                                      EnableLogging=algorithmLogging)
    return maskWS


def _integrateBkgs(ws, eppWS, sigmaMultiplier, wsNames, wsCleanup, algorithmLogging):
    """Return a workspace integrated around flat background areas."""
    histogramCount = ws.getNumberHistograms()
    binMatrix = ws.extractX()
    leftBegins = binMatrix[:, 0]
    leftEnds = numpy.empty(histogramCount)
    rightBegins = numpy.empty(histogramCount)
    rightEnds = binMatrix[:, -1]
    for i in range(histogramCount):
        eppRow = eppWS.row(i)
        if eppRow['FitStatus'] != 'success':
            leftBegins[i] = 0
            leftEnds[i] = 0
            rightBegins[i] = 0
            rightEnds[i] = 0
            continue
        peakCentre = eppRow['PeakCentre']
        sigma = eppRow['Sigma']
        leftEnds[i] = peakCentre - sigmaMultiplier * sigma
        if leftBegins[i] > leftEnds[i]:
            leftBegins[i] = leftEnds[i]
        rightBegins[i] = peakCentre + sigmaMultiplier * sigma
        if rightBegins[i] > rightEnds[i]:
            rightBegins[i] = rightEnds[i]
    leftWSName =  wsNames.withSuffix('integrated_left_bkgs')
    leftWS = Integration(InputWorkspace=ws,
                         OutputWorkspace=leftWSName,
                         RangeLowerList=leftBegins,
                         RangeUpperList=leftEnds,
                         EnableLogging=algorithmLogging)
    rightWSName = wsNames.withSuffix('integrated_right_bkgs')
    rightWS = Integration(InputWorkspace=ws,
                          OutputWorkspace=rightWSName,
                          RangeLowerList=rightBegins,
                          RangeUpperList=rightEnds,
                          EnableLogging=algorithmLogging)
    sumWSName = wsNames.withSuffix('integrated_bkgs_sum')
    sumWS = Plus(LHSWorkspace=leftWS,
                 RHSWorkspace=rightWS,
                 OutputWorkspace=sumWSName,
                 EnableLogging=algorithmLogging)
    wsCleanup.cleanup(leftWS)
    wsCleanup.cleanup(rightWS)
    return sumWS


def _integrateElasticPeaks(ws, eppWS, sigmaMultiplier, wsNames, wsCleanup, algorithmLogging):
    """Return a workspace integrated around the elastic peak."""
    histogramCount = ws.getNumberHistograms()
    integrationBegins = numpy.empty(histogramCount)
    integrationEnds = numpy.empty(histogramCount)
    for i in range(histogramCount):
        eppRow = eppWS.row(i)
        if eppRow['FitStatus'] != 'success':
            integrationBegins[i] = 0
            integrationEnds[i] = 0
            continue
        peakCentre = eppRow['PeakCentre']
        sigma = eppRow['Sigma']
        integrationBegins[i] = peakCentre - sigmaMultiplier * sigma
        integrationEnds[i] = peakCentre + sigmaMultiplier * sigma
    integratedElasticPeaksWSName = \
        wsNames.withSuffix('integrated_elastic_peak')
    integratedElasticPeaksWS = \
        Integration(InputWorkspace=ws,
                    OutputWorkspace=integratedElasticPeaksWSName,
                    IncludePartialBins=True,
                    RangeLowerList=integrationBegins,
                    RangeUpperList=integrationEnds,
                    EnableLogging=algorithmLogging)
    solidAngleWSName = wsNames.withSuffix('detector_solid_angles')
    solidAngleWS = SolidAngle(InputWorkspace=ws,
                              OutputWorkspace=solidAngleWSName,
                              EnableLogging=algorithmLogging)
    solidAngleCorrectedElasticPeaksWSName = \
        wsNames.withSuffix('solid_angle_corrected_elastic_peak')
    solidAngleCorrectedElasticPeaksWS = \
        Divide(LHSWorkspace=integratedElasticPeaksWS,
               RHSWorkspace=solidAngleWS,
               OutputWorkspace=solidAngleCorrectedElasticPeaksWSName,
               EnableLogging=algorithmLogging)
    wsCleanup.cleanup(integratedElasticPeaksWS)
    wsCleanup.cleanup(solidAngleWS)
    return solidAngleCorrectedElasticPeaksWS


def _maskDiagnosedDetectors(ws, diagnosticsWS, wsNames, algorithmLogging):
    """Mask detectors according to diagnostics."""
    maskedWSName = wsNames.withSuffix('diagnostics_applied')
    maskedWS = CloneWorkspace(InputWorkspace=ws,
                              OutputWorkspace=maskedWSName,
                              EnableLogging=algorithmLogging)
    MaskDetectors(Workspace=maskedWS,
                  MaskedWorkspace=diagnosticsWS,
                  EnableLogging=algorithmLogging)
    return maskedWS


def _maskedListToStr(masked):
    """Return a string presentation of a list of spectrum numbers."""
    if not masked:
        return ''

    def addBlock(string, begin, end):
        if blockBegin == blockEnd:
            string += str(blockEnd) + ', '
        else:
            string += str(blockBegin) + '-' + str(blockEnd) + ', '
        return string

    string = str()
    blockBegin = None
    blockEnd = None
    maxMasked = max(masked)
    for i in sorted(masked):
        if blockBegin is None:
            blockBegin = i
            blockEnd = i
        if i == maxMasked:
            if i > blockEnd + 1:
                string = addBlock(string, blockBegin, blockEnd)
                string += str(i)
            else:
                string += str(blockBegin) + '-' + str(i)
            break
        if i > blockEnd + 1:
            string = addBlock(string, blockBegin, blockEnd)
            blockBegin = i
        blockEnd = i
    return string


def _reportBkgDiagnostics(reportWS, bkgWS, diagnosticsWS):
    """Return masked spectrum numbers and add background diagnostics information to a report workspace."""
    return _reportDiagnostics(reportWS, bkgWS, diagnosticsWS, 'FlatBkg', 'BkgDiagnosed')


def _reportDiagnostics(reportWS, dataWS, diagnosticsWS, dataColumn, diagnosedColumn):
    """Return masked spectrum numbers and add diagnostics information to a report workspace."""
    if reportWS is not None:
        existingColumnNames = reportWS.getColumnNames()
        if dataColumn not in existingColumnNames:
            reportWS.addColumn('double', dataColumn, _PLOT_TYPE_Y)
        if diagnosedColumn not in existingColumnNames:
            reportWS.addColumn('double', diagnosedColumn, _PLOT_TYPE_Y)
    maskedSpectra = list()
    for i in range(dataWS.getNumberHistograms()):
        diagnosed = int(diagnosticsWS.readY(i)[0])
        if reportWS is not None:
            y = dataWS.readY(i)[0]
            reportWS.setCell(dataColumn, i, y)
            reportWS.setCell(diagnosedColumn, i, diagnosed)
        if diagnosed != 0:
            maskedSpectra.append(dataWS.getSpectrum(i).getSpectrumNo())
    return maskedSpectra


def _reportPeakDiagnostics(reportWS, peakIntensityWS, diagnosticsWS):
    """Return masked spectrum numbers and add elastic peak diagnostics information to a report workspace."""
    return _reportDiagnostics(reportWS, peakIntensityWS, diagnosticsWS, 'ElasticIntensity', 'IntensityDiagnosed')


def _reportUserMask(reportWS, maskWS):
    """Return masked spectrum numbers and add user mask information to a report workspace."""
    if reportWS is not None:
        MASKED_COLUMN = 'UserMask'
        existingColumnNames = reportWS.getColumnNames()
        if MASKED_COLUMN not in existingColumnNames:
            reportWS.addColumn('double', MASKED_COLUMN, _PLOT_TYPE_Y)
    maskedSpectra = list()
    for i in range(maskWS.getNumberHistograms()):
        diagnosed = int(maskWS.readY(i)[0])
        if reportWS is not None:
            reportWS.setCell(MASKED_COLUMN, i, diagnosed)
        if diagnosed != 0:
            maskedSpectra.append(maskWS.getSpectrum(i).getSpectrumNo())
    return maskedSpectra


class DirectILLDiagnostics(DataProcessorAlgorithm):
    """A workflow algorithm for detector diagnostics."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return 'Workflow\\Inelastic'

    def name(self):
        """Return the algorithm's name."""
        return 'DirectILLDiagnostics'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Perform detector diagnostics and masking for the direct geometry TOF spectrometers at ILL.'

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Executes the data reduction workflow."""
        progress = Progress(self, 0.0, 1.0, 5)
        report = common.Report()
        subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsNames = common.NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        # Get input workspace.
        progress.report('Loading inputs')
        mainWS = self._inputWS(wsNames, wsCleanup, subalgLogging)

        maskWS = _createMaskWS(mainWS, wsNames, subalgLogging)

        # Apply hard mask.
        progress.report('Applying default mask')
        maskWS = self._applyDefaultMask(maskWS, wsNames, wsCleanup, subalgLogging)

        # Apply user mask.
        progress.report('Applying hard mask')
        maskWS = self._applyUserMask(maskWS, wsNames, wsCleanup, subalgLogging)

        # Detector diagnostics, if requested.
        progress.report('Diagnosing detectors')
        mainWS = self._detDiagnostics(mainWS, maskWS, wsNames, wsCleanup, report, subalgLogging)

        self._finalize(mainWS, wsCleanup, report)
        progress.report('Done')

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        greaterThanUnityFloat = FloatBoundedValidator(lower=1)
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator('TOF'))
        positiveFloat = FloatBoundedValidator(lower=0)
        positiveIntArray = IntArrayBoundedValidator()
        positiveIntArray.setLower(0)
        scalingFactor = FloatBoundedValidator(lower=0, upper=1)

        # Properties.
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            direction=Direction.Input),
            doc='Raw input workspace.')
        self.declareProperty(WorkspaceProperty(name=common.PROP_OUTPUT_WS,
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='A diagnostics mask workspace.')
        self.declareProperty(name=common.PROP_CLEANUP_MODE,
                             defaultValue=common.CLEANUP_ON,
                             validator=StringListValidator([
                                 common.CLEANUP_ON,
                                 common.CLEANUP_OFF]),
                             direction=Direction.Input,
                             doc='What to do with intermediate workspaces.')
        self.declareProperty(name=common.PROP_SUBALG_LOGGING,
                             defaultValue=common.SUBALG_LOGGING_OFF,
                             validator=StringListValidator([
                                 common.SUBALG_LOGGING_OFF,
                                 common.SUBALG_LOGGING_ON]),
                             direction=Direction.Input,
                             doc='Enable or disable subalgorithms to ' +
                                 'print in the logs.')
        self.declareProperty(ITableWorkspaceProperty(
            name=common.PROP_EPP_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Table workspace containing results from the FindEPP algorithm.')
        self.declareProperty(name=common.PROP_ELASTIC_PEAK_DIAGNOSTICS,
                             defaultValue=common.ELASTIC_PEAK_DIAGNOSTICS_ON,
                             validator=StringListValidator([
                                 common.ELASTIC_PEAK_DIAGNOSTICS_ON,
                                 common.ELASTIC_PEAK_DIAGNOSTICS_OFF]),
                             direction=Direction.Input,
                             doc='Enable or disable elastic peak diagnostics.')
        self.setPropertyGroup(common.PROP_ELASTIC_PEAK_DIAGNOSTICS,
                              common.PROPGROUP_PEAK_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER,
                             defaultValue=3.0,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc="Integration width of the elastic peak in multiples " +
                                 " of 'Sigma' in the EPP table.")
        self.setPropertyGroup(common.PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER,
                              common.PROPGROUP_PEAK_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=0.1,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit ' +
                                 'used in elastic peak diagnostics.')
        self.setPropertyGroup(common.PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD,
                              common.PROPGROUP_PEAK_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=3.0,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit ' +
                                 'used in elastic peak diagnostics.')
        self.setPropertyGroup(common.PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD,
                              common.PROPGROUP_PEAK_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=3.3,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Error bar multiplier for significance ' +
                                 'test in the elastic peak diagnostics.')
        self.setPropertyGroup(common.PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST,
                              common.PROPGROUP_PEAK_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS,
                             defaultValue=common.BKG_DIAGNOSTICS_AUTO,
                             validator=StringListValidator([
                                 common.BKG_DIAGNOSTICS_AUTO,
                                 common.BKG_DIAGNOSTICS_ON,
                                 common.BKG_DIAGNOSTICS_OFF]),
                             direction=Direction.Input,
                             doc='Control the background diagnostics.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS, common.PROPGROUP_BKG_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_SIGMA_MULTIPLIER,
                             defaultValue=10.0,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc="Width of the range excluded from background integration around " +
                                 "the elastic peaks in multiplies of 'Sigma' in the EPP table")
        self.setPropertyGroup(common.PROP_BKG_SIGMA_MULTIPLIER,
                              common.PROPGROUP_BKG_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD,
                             defaultValue=0.1,
                             validator=scalingFactor,
                             direction=Direction.Input,
                             doc='Multiplier for lower acceptance limit ' +
                                 'used in noisy background diagnostics.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD,
                              common.PROPGROUP_BKG_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD,
                             defaultValue=3.3,
                             validator=greaterThanUnityFloat,
                             direction=Direction.Input,
                             doc='Multiplier for higher acceptance limit ' +
                                 'used in noisy background diagnostics.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD,
                              common.PROPGROUP_BKG_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST,
                             defaultValue=3.3,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc='Error bar multiplier for significance ' +
                                 'test in the noisy background diagnostics.')
        self.setPropertyGroup(common.PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST,
                              common.PROPGROUP_BKG_DIAGNOSTICS)
        self.declareProperty(name=common.PROP_DEFAULT_MASK,
                             defaultValue=common.DEFAULT_MASK_ON,
                             validator=StringListValidator([
                                 common.DEFAULT_MASK_ON,
                                 common.DEFAULT_MASK_OFF]),
                             direction=Direction.Input,
                             doc='Enable or disable instrument specific default mask.')
        self.declareProperty(IntArrayProperty(name=common.PROP_USER_MASK,
                                              values='',
                                              validator=positiveIntArray,
                                              direction=Direction.Input),
                             doc='List of spectra to mask.')
        self.setPropertyGroup(common.PROP_USER_MASK, common.PROPGROUP_USER_MASK)
        self.declareProperty(
            StringArrayProperty(name=common.PROP_USER_MASK_COMPONENTS,
                                values='',
                                direction=Direction.Input),
            doc='List of instrument components to mask.')
        self.setPropertyGroup(common.PROP_USER_MASK_COMPONENTS, common.PROPGROUP_USER_MASK)
        # Rest of the output properties
        self.declareProperty(ITableWorkspaceProperty(
            name=common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS,
            defaultValue='',
            direction=Direction.Output,
            optional=PropertyMode.Optional),
            doc='Output table workspace for detector diagnostics reporting.')
        self.setPropertyGroup(common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS,
                              common.PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(name=common.PROP_OUTPUT_DIAGNOSTICS_REPORT,
                             defaultValue='',
                             direction=Direction.Output,
                             doc='Diagnostics report as a string.')
        self.setPropertyGroup(common.PROP_OUTPUT_DIAGNOSTICS_REPORT,
                              common.PROPGROUP_OPTIONAL_OUTPUT)

    def validateInputs(self):
        """Check for issues with user input."""
        issues = dict()
        if self.getProperty(common.PROP_EPP_WS).isDefault:
            if self.getProperty(common.PROP_ELASTIC_PEAK_DIAGNOSTICS).value == common.ELASTIC_PEAK_DIAGNOSTICS_ON:
                issues[common.PROP_EPP_WS] = 'An EPP table is needed for elastic peak diagnostics.'
            if self.getProperty(common.PROP_BKG_DIAGNOSTICS).value == common.BKG_DIAGNOSTICS_ON:
                issues[common.PROP_EPP_WS] = 'An EPP table is needed for background diagnostics.'
        return issues

    def _applyDefaultMask(self, maskWS, wsNames, wsCleanup, algorithmLogging):
        """Apply instrument specific default mask to a workspace."""
        option = self.getProperty(common.PROP_DEFAULT_MASK).value
        if option == common.DEFAULT_MASK_OFF:
            return maskWS
        instrumentName = maskWS.getInstrument().getName()
        if instrumentName == 'IN5':
            maskFile = os.path.join(mantid.config.getInstrumentDirectory(), 'masks' , 'IN5_Mask.xml')
            defaultMaskWSName = wsNames.withSuffix('default_mask')
            defaultMaskWS = LoadMask(Instrument='IN5',
                                     InputFile=maskFile,
                                     RefWorkspace=maskWS,
                                     OutputWorkspace=defaultMaskWSName,
                                     EnableLogging=algorithmLogging)
            MaskDetectors(Workspace=maskWS,
                          MaskedWorkspace=defaultMaskWS,
                          EnableLogging=algorithmLogging)
            wsCleanup.cleanup(defaultMaskWS)
        return maskWS

    def _applyUserMask(self, maskWS, wsNames, wsCleanup, algorithmLogging):
        """Apply user mask to a workspace."""
        userMask = self.getProperty(common.PROP_USER_MASK).value
        maskComponents = self.getProperty(common.PROP_USER_MASK_COMPONENTS).value
        MaskDetectors(Workspace=maskWS,
                      DetectorList=userMask,
                      ComponentList=maskComponents,
                      EnableLogging=algorithmLogging)
        return maskWS

    def _bkgDiagnosticsEnabled(self, mainWS, report):
        """Return true if background diagnostics are enabled, false otherwise."""
        bkgDiagnostics = self.getProperty(common.PROP_BKG_DIAGNOSTICS).value
        if bkgDiagnostics == common.BKG_DIAGNOSTICS_AUTO:
            instrument = mainWS.getInstrument()
            if instrument.hasParameter('enable_background_diagnostics'):
                enabled = instrument.getBoolParameter('enable_background_diagnostics')[0]
                if enabled:
                    report.notice(common.PROP_BKG_DIAGNOSTICS + ' set to '
                                  + common.BKG_DIAGNOSTICS_ON + ' by the IPF.')
                    return True
                else:
                    report.notice(common.PROP_BKG_DIAGNOSTICS + ' set to '
                                  + common.BKG_DIAGNOSTICS_OFF + ' by the IPF.')
                    return False
            else:
                report.notice('Defaulted ' + common.PROP_BKG_DIAGNOSTICS + ' to '
                              + common.BKG_DIAGNOSTICS_ON + '.')
                return True
        elif bkgDiagnostics == common.BKG_DIAGNOSTICS_ON:
            return True
        return False

    def _detDiagnostics(self, mainWS, maskWS, wsNames, wsCleanup, report, subalgLogging):
        """Perform and apply detector diagnostics."""
        reportWS = None
        if not self.getProperty(common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS).isDefault:
            reportWSName = self.getProperty(common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS).valueAsStr
            reportWS = _createDiagnosticsReportTable(reportWSName, mainWS.getNumberHistograms(), subalgLogging)
        userMaskWS = _extractUserMask(maskWS, wsNames, subalgLogging)
        diagnosticsWSName = wsNames.withSuffix('diagnostics')
        diagnosticsWS = CloneWorkspace(InputWorkspace=userMaskWS,
                                       OutputWorkspace=diagnosticsWSName,
                                       EnableLogging=subalgLogging)
        userMaskedSpectra = _reportUserMask(reportWS, userMaskWS)
        peakMaskedSpectra = None
        bkgMaskedSpectra = None
        wsCleanup.cleanup(userMaskWS)
        if self.getProperty(common.PROP_ELASTIC_PEAK_DIAGNOSTICS).value == common.ELASTIC_PEAK_DIAGNOSTICS_ON:
            eppWS = self.getProperty(common.PROP_EPP_WS).value
            sigmaMultiplier = self.getProperty(common.PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER).value
            integratedPeaksWS = _integrateElasticPeaks(mainWS, eppWS, sigmaMultiplier, wsNames, wsCleanup, subalgLogging)
            lowThreshold = self.getProperty(common.PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD).value
            highThreshold = self.getProperty(common.PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD).value
            significanceTest = self.getProperty(common.PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST).value
            settings = _DiagnosticsSettings(lowThreshold, highThreshold, significanceTest)
            peakDiagnosticsWS = _elasticPeakDiagnostics(integratedPeaksWS, settings, wsNames, subalgLogging)
            peakMaskedSpectra = _reportPeakDiagnostics(reportWS, integratedPeaksWS, peakDiagnosticsWS)
            wsCleanup.cleanup(integratedPeaksWS)
            diagnosticsWS = Plus(LHSWorkspace=diagnosticsWS,
                                 RHSWorkspace=peakDiagnosticsWS,
                                 OutputWorkspace=diagnosticsWSName,
                                 EnableLogging=subalgLogging)
            wsCleanup.cleanup(peakDiagnosticsWS)
        bkgDiagnosticsEnabled = self._bkgDiagnosticsEnabled(mainWS, report)
        if bkgDiagnosticsEnabled:
            eppWS = self.getProperty(common.PROP_EPP_WS).value
            sigmaMultiplier = self.getProperty(common.PROP_BKG_SIGMA_MULTIPLIER).value
            integratedBkgs = _integrateBkgs(mainWS, eppWS, sigmaMultiplier, wsNames, wsCleanup, subalgLogging)
            lowThreshold = self.getProperty(common.PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD).value
            highThreshold = self.getProperty(common.PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD).value
            significanceTest = self.getProperty(common.PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST).value
            settings = _DiagnosticsSettings(lowThreshold, highThreshold, significanceTest)
            bkgDiagnosticsWS = _bkgDiagnostics(integratedBkgs, settings, wsNames, subalgLogging)
            bkgMaskedSpectra = _reportBkgDiagnostics(reportWS, integratedBkgs, bkgDiagnosticsWS)
            wsCleanup.cleanup(integratedBkgs)
            diagnosticsWS = Plus(LHSWorkspace=diagnosticsWS,
                                 RHSWorkspace=bkgDiagnosticsWS,
                                 OutputWorkspace=diagnosticsWSName,
                                 EnableLogging=subalgLogging)
            wsCleanup.cleanup(bkgDiagnosticsWS)
        wsCleanup.cleanup(mainWS)
        self._outputReports(reportWS, userMaskedSpectra, peakMaskedSpectra, bkgMaskedSpectra)
        return diagnosticsWS

    def _finalize(self, outWS, wsCleanup, report):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        wsCleanup.cleanup(outWS)
        wsCleanup.finalCleanup()
        report.toLog(self.log())

    def _inputWS(self, wsNames, wsCleanup, subalgLogging):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        wsCleanup.protect(mainWS)
        return mainWS

    def _outputReports(self, reportWS, userMaskedSpectra, peakMaskedSpectra, bkgMaskedSpectra):
        """Set the optional output report properties."""
        if reportWS is not None:
            self.setProperty(common.PROP_OUTPUT_DIAGNOSTICS_REPORT_WS, reportWS)
        report = 'Spectra masked by user:\n'
        if userMaskedSpectra:
            report += _maskedListToStr(userMaskedSpectra)
        else:
            report += 'None'
        report += '\nSpectra marked as bad by elastic peak diagnostics:\n'
        if peakMaskedSpectra:
            report += _maskedListToStr(peakMaskedSpectra)
        else:
            report += 'None'
        report += '\nSpectra marked as bad by flat background diagnostics:\n'
        if bkgMaskedSpectra:
            report += _maskedListToStr(bkgMaskedSpectra)
        else:
            report += 'None'
        self.setProperty(common.PROP_OUTPUT_DIAGNOSTICS_REPORT, report)


AlgorithmFactory.subscribe(DirectILLDiagnostics)
