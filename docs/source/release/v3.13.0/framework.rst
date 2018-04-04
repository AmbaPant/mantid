=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


Bug fixes
#########

- The documentation of the algorithm :ref:`algm-CreateSampleWorkspace` did not match its implementation. The axis in beam direction will now be correctly described as Z instead of X.

Algorithms
----------

- :ref:`ConvertToPointData <algm-ConvertToPointData>` and :ref:`ConvertToHistogram <algm-ConvertToHistogram>` now propagate the Dx errors to the output.
- The algorithm :ref:`CreateWorkspace <algm-CreateWorkspace>` can now optionally receive the Dx errors.
- The algorithm :ref:`SortXAxis <algm-SortXAxis>` has a new input option that allows ascending (default) and descending sorting. Furthermore, Dx values will be considered if present. The documentation needed to be corrected.


:ref:`Release 3.13.0 <v3.13.0>`
