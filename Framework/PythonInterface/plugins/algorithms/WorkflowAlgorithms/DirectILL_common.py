# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import DeleteWorkspace


CLEANUP_OFF = 'Cleanup OFF'
CLEANUP_ON = 'Cleanup ON'

DIAGNOSTICS_OFF = 'Diagnostics OFF'
DIAGNOSTICS_ON = 'Diagnostics ON'

ELASTIC_CHANNEL_SAMPLE_LOG = 'Default Elastic Channel'
ELASTIC_CHANNEL_FIT = 'Fit Elastic Channel'

EPP_METHOD_FIT = 'Fit EPP'
EPP_MEHTOD_CALCULATE = 'Calculate EPP'

INCIDENT_ENERGY_CALIBRATION_OFF = 'Energy Calibration OFF'
INCIDENT_ENERGY_CALIBRATION_ON = 'Energy Calibration ON'

INDEX_TYPE_DET_ID = 'Detector ID'
INDEX_TYPE_WS_INDEX = 'Workspace Index'
INDEX_TYPE_SPECTRUM_NUMBER = 'Spectrum Number'

NORM_METHOD_MON = 'Normalisation Monitor'
NORM_METHOD_OFF = 'Normalisation OFF'
NORM_METHOD_TIME = 'Normalisation Time'

PROP_BINNING_PARAMS_Q = 'QBinningParams'
PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD = 'NoisyBkgHighThreshold'
PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD = 'NoisyBkgLowThreshold'
PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST = 'NoisyBkgErrorThreshold'
PROP_CLEANUP_MODE = 'Cleanup'
PROP_DIAGNOSTICS_WS = 'DiagnosticsWorkspace'
PROP_DET_DIAGNOSTICS = 'Diagnostics'
PROP_EC_SCALING = 'EmptyContainerScaling'
PROP_EC_WS = 'EmptyContainerWorkspace'
PROP_ELASTIC_CHANNEL_MODE = 'ElasticChannel'
PROP_ELASTIC_CHANNEL_WS = 'ElasticChannelWorkspace'
PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER = 'ElasticPeakWidthInSigmas'
PROP_EPP_METHOD = 'EPPCreationMethod'
PROP_EPP_SIGMA = 'SigmaForCalculatedEPP'
PROP_EPP_WS = 'EPPWorkspace'
PROP_FLAT_BKG_SCALING = 'FlatBkgScaling'
PROP_FLAT_BKG_WINDOW = 'FlatBkgAveragingWindow'
PROP_FLAT_BKG_WS = 'FlatBkgWorkspace'
PROP_INCIDENT_ENERGY_CALIBRATION = 'IncidentEnergyCalibration'
PROP_INCIDENT_ENERGY_WS = 'IncidentEnergyWorkspace'
PROP_INPUT_FILE = 'InputFile'
PROP_INPUT_WS = 'InputWorkspace'
PROP_MON_EPP_WS = 'MonitorEPPWorkspace'
PROP_MON_INDEX = 'Monitor'
PROP_MON_PEAK_SIGMA_MULTIPLIER = 'MonitorPeakWidthInSigmas'
PROP_NORMALISATION = 'Normalisation'
PROP_NUMBER_OF_SIMULATION_WAVELENGTHS = 'NumberOfCalculationPoints'
PROP_OUTPUT_DET_EPP_WS = 'OutputEPPWorkspace'
PROP_OUTPUT_DIAGNOSTICS_REPORT_WS = 'OutputDiagnosticsReportWorkspace'
PROP_OUTPUT_ELASTIC_CHANNEL_WS = 'OutputElasticChannelWorkspace'
PROP_OUTPUT_FLAT_BKG_WS = 'OutputFlatBkgWorkspace'
PROP_OUTPUT_INCIDENT_ENERGY_WS = 'OutputIncidentEnergyWorkspace'
PROP_OUTPUT_MON_EPP_WS = 'OutputMonitorEPPWorkspace'
PROP_OUTPUT_SELF_SHIELDING_CORRECTION_WS = 'OutputSelfShieldingCorrectionWorkspace'
PROP_OUTPUT_THETA_W_WS = 'OutputSofThetaEnergyWorkspace'
PROP_OUTPUT_WS = 'OutputWorkspace'
PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD = 'ElasticPeakHighThreshold'
PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD = 'ElasticPeakLowThreshold'
PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST = 'ElasticPeakErrorThreshold'
PROP_REBINNING_PARAMS_W = 'EnergyRebinningParams'
PROP_SELF_SHIELDING_CORRECTION_WS = 'SelfShieldingCorrectionWorkspace'
PROP_SUBALG_LOGGING = 'SubalgorithmLogging'
PROP_TEMPERATURE = 'Temperature'
PROP_TRANSPOSE_SAMPLE_OUTPUT = 'Transposing'
PROP_USER_MASK = 'MaskedDetectors'
PROP_USER_MASK_COMPONENTS = 'MaskedComponents'
PROP_VANA_WS = 'IntegratedVanadiumWorkspace'

PROPGROUP_BKG_DIAGNOSTICS = 'Background Diagnostics'
PROPGROUP_INCIDENT_ENERGY_CALIBRATION = 'Indicent Energy Calibration'
PROPGROUP_FLAT_BKG = 'Flat Time-Independent Background'
PROPGROUP_MON_NORMALISATION = 'Neutron Flux Normalisation'
PROPGROUP_OPTIONAL_OUTPUT = 'Optional Output'
PROPGROUP_PEAK_DIAGNOSTICS = 'Elastic Peak Diagnostics'
PROPGROUP_REBINNING = 'Rebinning for SofQW'
PROPGROUP_TOF_AXIS_CORRECTION = 'TOF Axis Correction for Elastic Channel'
PROPGROUP_USER_MASK = 'Additional Masking'

SUBALG_LOGGING_OFF = 'Logging OFF'
SUBALG_LOGGING_ON = 'Logging ON'

TRANSPOSING_OFF = 'Transposing OFF'
TRANSPOSING_ON = 'Transposing ON'

WS_CONTENT_DETS = 0
WS_CONTENT_MONS = 1


class IntermediateWSCleanup:
    """A class to manage intermediate workspace cleanup."""

    def __init__(self, cleanupMode, deleteAlgorithmLogging):
        """Initialize an instance of the class."""
        self._deleteAlgorithmLogging = deleteAlgorithmLogging
        self._doDelete = cleanupMode == CLEANUP_ON
        self._protected = set()
        self._toBeDeleted = set()

    def cleanup(self, *args):
        """Delete the workspaces listed in *args."""
        for ws in args:
            self._delete(ws)

    def cleanupLater(self, *args):
        """Mark the workspaces listed in *args to be cleaned up later."""
        map(self._toBeDeleted.add, map(str, args))

    def finalCleanup(self):
        """Delete all workspaces marked to be cleaned up later."""
        for ws in self._toBeDeleted:
            self._delete(ws)

    def protect(self, *args):
        """Mark the workspaces listed in *args to be never deleted."""
        map(self._protected.add, map(str, args))

    def _delete(self, ws):
        """Delete the given workspace in ws if it is not protected, and
        deletion is actually turned on.
        """
        if not self._doDelete:
            return
        ws = str(ws)
        if ws not in self._protected and mtd.doesExist(ws):
            DeleteWorkspace(Workspace=ws,
                            EnableLogging=self._deleteAlgorithmLogging)


class NameSource:
    """A class to provide names for intermediate workspaces."""

    def __init__(self, prefix, cleanupMode):
        """Initialize an instance of the class."""
        self._names = set()
        self._prefix = prefix
        if cleanupMode == CLEANUP_ON:
            self._prefix = '__' + prefix

    def withSuffix(self, suffix):
        """Returns a workspace name with given suffix applied."""
        return self._prefix + '_' + suffix + '_'


class Report:
    """A logger for final report generation."""

    _LEVEL_NOTICE = 0
    _LEVEL_WARNING = 1
    _LEVEL_ERROR = 2

    class _Entry:
        """A private class for report entries."""

        def __init__(self, text, loggingLevel):
            """Initialize an entry."""
            self._text = text
            self._loggingLevel = loggingLevel

        def contents(self):
            """Return the contents of this entry."""
            return self._text

        def level(self):
            """Return the logging level of this entry."""
            return self._loggingLevel

    def __init__(self):
        """Initialize a report."""
        self._entries = list()

    def error(self, text):
        """Add an error entry to this report."""
        self._entries.append(Report._Entry(text, Report._LEVEL_ERROR))

    def notice(self, text):
        """Add an information entry to this report."""
        self._entries.append(Report._Entry(text, Report._LEVEL_NOTICE))

    def warning(self, text):
        """Add a warning entry to this report."""
        self._entries.append(Report._Entry(text, Report._LEVEL_WARNING))

    def toLog(self, log):
        """Write this report to a log."""
        for entry in self._entries:
            loggingLevel = entry.level()
            if loggingLevel == Report._LEVEL_NOTICE:
                log.notice(entry.contents())
            elif loggingLevel == Report._LEVEL_WARNING:
                log.warning(entry.contents())
            elif loggingLevel == Report._LEVEL_ERROR:
                log.error(entry.contents())


def convertToWorkspaceIndex(i, ws, indexType=INDEX_TYPE_DET_ID):
    """Convert given number to workspace index."""
    if indexType == INDEX_TYPE_WS_INDEX:
        return i
    elif indexType == INDEX_TYPE_SPECTRUM_NUMBER:
        return ws.getIndexFromSpectrumNumber(i)
    else:  # INDEX_TYPE_DET_ID
        for j in range(ws.getNumberHistograms()):
            if ws.getSpectrum(j).hasDetectorID(i):
                return j
        raise RuntimeError('No workspace index found for detector id {0}'.format(i))


def convertListToWorkspaceIndices(indices, ws, indexType=INDEX_TYPE_DET_ID):
    """Convert a list of spectrum nubmers/detector IDs to workspace indices."""
    return [convertToWorkspaceIndex(i, ws, indexType) for i in indices]
