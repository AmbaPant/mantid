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
- The `OutputWorkspace` of the algorithm :ref:`algm-Stitch1D` will be created according to the default option for the x values `WeightedMeanOverlap` or the newly implemented option `OriginalOverlap`. Furthermore, the Dx error values will be propagated for the `WeightedMeanOverlap` output option.


:ref:`Release 3.13.0 <v3.13.0>`
