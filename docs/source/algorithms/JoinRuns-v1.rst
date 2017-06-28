
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm joins the input workspaces into a single one by concatenating their spectra. The concatenation is done in the same order as in the input workspaces list. Consider using :ref:`SortXAxis <algm-SortXAxis>` afterwards, if necessary. The instrument and the units are copied from the first workspace. The sample logs are also copied from the first input, but the behaviour can be controlled by the instrument parameter file (IPF), as described in :ref:`MergeRuns <algm-MergeRuns>`. Furthermore, that behaviour can be overriden by providing input to the relevant optional properties of the algorithm.

InputWorkspaces
---------------
This can be a mixed list of workspaces and workspace groups on AnalysisDataService (ADS), that will be flattened to a list of workspaces. MatrixWorkspaces representing point-data are required with:

- the same instrument
- the same number of histograms
- the same units

SampleLogAsXAxis
----------------

If specified, this log values will constitute the x-axis of the resulting workspace. The log must exist in all the input workspaces and must be numeric (int or double), in which case the input workspaces must contain single bin only, or numeric time series, in which case the lenght of the series must match the number of bins. 

Usage
-----

**Example - JoinRuns**

.. testcode:: JoinRunsExample
   
    # Create input workspaces
    list = []
    for i in range(3):
        ws = "ws_{0}".format(i)
        CreateSampleWorkspace(Function="One Peak", NumBanks=1, BankPixelWidth=2,
                              XMin=i*100, XMax=(i+1)*100, BinWidth=50,
                              Random=True, OutputWorkspace=ws)
        ConvertToPointData(InputWorkspace=ws, OutputWorkspace=ws)
        list.append(ws)

    # Join the workspaces
    out = JoinRuns(list)

    # Check the output
    print("out has {0} bins with x-axis as: {1}, {2}, {3}, {4}, {5}, {6}".
          format(out.blocksize(), out.readX(0)[0], out.readX(0)[1], out.readX(0)[2],
                 out.readX(0)[3], out.readX(0)[4], out.readX(0)[5]))

Output:

.. testoutput:: JoinRunsExample

    out has 6 bins with x-axis as: 25.0, 75.0, 125.0, 175.0, 225.0, 275.0

**Example - JoinRuns with a numeric log**

.. testcode:: JoinRunsLogExample

    # Create input workspaces
    list = []
    for i in range(3):
        ws = "ws_{0}".format(i)
        CreateSampleWorkspace(Function="One Peak", NumBanks=1, BankPixelWidth=2,
                              XMin=i*100, XMax=(i+1)*100, BinWidth=100,
                              Random=True, OutputWorkspace=ws)
        ConvertToPointData(InputWorkspace=ws, OutputWorkspace=ws)
        AddSampleLog(ws, LogName='LOG',LogType='Number', LogText=str(5*i))
        list.append(ws)

    # Join the workspaces
    out = JoinRuns(list, SampleLogAsXAxis='LOG')

    # Check the output
    print("out has {0} bins with x-axis as: {1}, {2}, {3}".
          format(out.blocksize(), out.readX(0)[0], out.readX(0)[1], out.readX(0)[2]))

Output:

.. testoutput:: JoinRunsLogExample

    out has 3 bins with x-axis as: 0.0, 5.0, 10.0

**Example - JoinRuns with a numeric time series log**

.. testcode:: JoinRunsTSLogExample

    import datetime
    # Create input workspaces
    list = []
    for i in range(3):
        ws = "ws_{0}".format(i)
        CreateSampleWorkspace(Function="One Peak", NumBanks=1, BankPixelWidth=2,
                              XMin=i*100, XMax=(i+1)*100, BinWidth=50,
                              Random=True, OutputWorkspace=ws)
        ConvertToPointData(InputWorkspace=ws, OutputWorkspace=ws)

        for j in range(2):
            AddTimeSeriesLog(ws, Name='LOG',Time=str(datetime.datetime.now()), Value=str(10*i+0.25*j))

        list.append(ws)

    # Join the workspaces
    out = JoinRuns(list, SampleLogAsXAxis='LOG')

    # Check the output
    print("out has {0} bins with x-axis as: {1}, {2}, {3}, {4}, {5}, {6}".
          format(out.blocksize(), out.readX(0)[0], out.readX(0)[1], out.readX(0)[2],
          out.readX(0)[3], out.readX(0)[4], out.readX(0)[5]))

Output:

.. testoutput:: JoinRunsTSLogExample

    out has 6 bins with x-axis as: 0.0, 0.25, 10.0, 10.25, 20.0, 20.25

**Example - JoinRuns to fail with a sample log forbidding to merge**

.. testcode:: JoinRunsLogFail

     # Create input workspaces
    list = []
    for i in range(3):
        ws = "ws_{0}".format(i)
        CreateSampleWorkspace(Function="One Peak", NumBanks=1, BankPixelWidth=2,
                            XMin=i*100, XMax=(i+1)*100, BinWidth=50,
                            Random=True, OutputWorkspace=ws)
        ConvertToPointData(InputWorkspace=ws, OutputWorkspace=ws)
        AddSampleLog(Workspace=ws, LogName="Wavelength", LogType="Number", LogText=str(2+0.5*i))
        list.append(ws)
    try:
        out = JoinRuns(list, SampleLogsFail="Wavelength", SampleLogsFailTolerances="0.1", FailBehaviour="Stop")
    except ValueError:
        print("The differences in the wavelength of the inputs are more than the allowed tolerance")

Output:

.. testoutput:: JoinRunsLogFail

    The differences in the wavelength of the inputs are more than the allowed tolerance

.. categories::

.. sourcelink::

