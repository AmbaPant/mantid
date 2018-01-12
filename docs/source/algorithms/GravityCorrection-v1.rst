
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs a modification of time-of-flight values and their final angles, i.e. angles between the reflected beam and the sample, of neutrons by moving their counts to different detectors to cancel the gravitational effect on a chosen 2DWorkspace (notably the ILL Figaro reflectometer).
An initial computation of the final angle :math:`\theta_f` due to gravitation is required when the neutron flies from the source to the sample.
For the path from the sample to the detector, gravitation plays a role which can be cancelled.
Other properties of the :literal:`InputWorkspace` will be present in the :literal:`OutputWorkspace`.
Both cases, reflection up and down, can be treated.
Please take a look at the gravity correction for ILL reflectometers with the reduction software COSMOS here: Gutfreund et. al. Towards generalized data reduction on a time-of-flight neutron reflectometer.
Counts of neutrons that do not hit the detector after correction will not be considered in the :literal:`OutputWorkspace`, an information will be logged.
Please note, that the output workspace likely has varying bins and consider a subsequent rebinning step (:ref:`algm-Rebin`).
The instrument definition may only contain the position in beam direction and the height of the slits will be computed internally.
The potential output workspace adds " cancelled gravitation " to its title.

Requirements
------------

.. role:: red

- The x-axis of the :literal:`InputWorkspace` must be in :red:`time-of-flight`.
- Those time-of-flight values, :math:`t_{\mbox{tof}}`, are valid for a neutron travel distance from source to detector and do not take gravitation into account.
- The instrument must consist of a :red:`source`, :red:`sample`, :red:`detector` and a collimeter with its :red:`slits` or two other known locations of the neutron flight path between source and sample position.
  Please note that the slit position in beam direction is sufficiant, the horizontal position is supposed to be zero and the up position will be computed.
- The instrument must be defined in :red:`units metre`.
- The beam direction must be the axis direction of :literal:`X, Y` or :literal:`Z`, which is usually the case.
- The algorithm did not already execute for the given :literal:`InputWorkspace`.

Introduction
------------

All following images visualize a single neutron flight and its correction which corresponds to a single count in the workspace.
Please note that the images do not represent a real physical behaviour and serve only to describe the algorithm.
For all following considerations, the sample centre is at position :math:`x_s` = 0 m, :math:`y_s` = 0 m.
The beam direction is along the :math:`x` axis and the up direction along the :math:`y` axis.
The neutron flies in beam direction and the horizontal axis does not play a role.
In a first step, it is necessary to take correct final angles due to gravitation into account.
Then, the time-of-flight axis will be updated accordingly.

Corrected final angles
----------------------

The following image shows schematically gravitational effects for a detector positioned normal to the beam direction.

.. figure:: /images/GravityCorrection1.png
   :align: center

The orange line indicates the assumed neutron flight path which is present in the :literal:`InputWorkspace`.
The following parabola describes the spatial position of the neutron travelling from source to the sample:

.. math:: y = y_0 - k \left( x - x_0 \right)^2

The neutron must travel via the slits :math:`s_{1}` and :math:`s_{2}`:

.. math:: x_0 = \frac{k(x_{s_1}^2 - x_{s_2}^2)+(y_{s_1}-y_{s_2})}{2k (x_{s_1}-x_{s_2})}

.. math:: y_0 = y_{\mbox{s}_1} + k \left( x_{\mbox{s}_1} - x_0 \right)^2,

where the y-coordinate of a slit, depending on the reflection up or down is defined by the initial, uncorrected, incident angle :math:`\theta_{f_{i}}` (orange in the above image), is

.. math:: y_{s_1} = sign( {\theta_{f_{i}}} ) \ x_{s_1} \ tan \left( {\theta_{f_{i}}} \right).

The characteristic inverse length :math:`k` is given by

.. math:: k = \frac{g}{2 v_i^2}.

The velocity :math:`v_{i} = \frac{s_1 + s_2}{t_{\mbox{tof}}}` is the initial neutron velocity, taking the real flight path into account as described in `Parabola arc length`_ and the initial, not updated time-of-flight values.

The detector can have an arbitrary position as shown in the following image.

.. figure:: /images/GravityCorrection3.png
   :align: center

The corrected final angle :math:`\theta_f` shown in this image can be computed by using the gradient of the parabola at sample center position :math:`x_s` = 0 m, :math:`y_s` = 0 m:

.. math:: \boxed{{\theta_f} =  sign( {\theta_{f_{i}}} ) \ atan \left( 2 k \sqrt{|\frac{y_0}{k}|} \right) =  sign( {\theta_{f_{i}}} ) \ atan \left( 2 k x_{0} \right)},

Updating a final angle for a count can be achieved by moving the count to the spectrum of the detector or detector group of the new final angle.

Corrected time-of-flight axis
-----------------------------

The neutron velocity is

.. math:: v_{N} =  \sqrt{ v_{i}^2 \mbox{cos}({\theta_f})^2 + ( v_{i} \mbox{sin}({\theta_f}) - gt )^2 }

and the neutron arrives at detector position

.. math:: x_{d} = v_{N} \mbox{cos}({\theta_f}) t

.. math:: y_{d} = v_{N} \mbox{sin}({\theta_f}) t - \frac{1}{2} g t^2.

The corrected neutron flight path is given by

.. math:: y = x \mbox{tan}({\theta_f}).

When neglecting gravitation :math:`g = 0 \frac{\mbox{m}}{\mbox{s}^2}`, the corrected time-of-flight values are given by

.. math:: \boxed{t = \frac{x_{d}}{v_i \mbox{cos}(\theta_f)}},

where :math:`x_{d}` describes the detector :math:`x` position. Correspondingly, the corrected neutron count will be for detector position

.. math:: x_{d} = v_i \mbox{cos}({\theta_f}) t

.. math:: y_{d} = v_i \mbox{sin}({\theta_f}) t.

Parabola arc length
-------------------

The length :math:`s` of the parabola arc from source to sample

.. math:: s_1 = \int_{x_1}^{0} \sqrt(1 + \left( \frac{\partial y}{\partial x} \right)^2) dx

.. math:: s_1 = \int_{x_1}^{0} \sqrt(1 + \left( - 2 k \left( x - x_{0} \right) \right)

substituting :math:`2 k x = z` results in

.. math:: s_1 = \frac{1}{2k} \int_{\frac{x_1}{2k}}^{0} \sqrt(1 + z^{2}) dz

and with

.. math:: \frac{\partial tan(z)}{\partial z} = 1 + tan^{2}(z) = \frac{1}{cos^{2}(z)}

one can obtain finally

.. math:: s_1 = -\frac{1}{4k} \left(x \sqrt(1+x_1^{2}) + ln | x + \sqrt(1 + x_1^{2}) | + constant \right).

Equivalently, the solution of the more general form needed for calculating the length from sample to detector

.. math:: s_2 = \int_{0}^{x_2} \sqrt(c + \left( \frac{\partial y_d}{\partial x} \right)^2) dx

is

.. math:: s_2 = \frac{1}{2} \left( x \sqrt(a+x_2^{2}) + a \ ln | \frac{x_2}{\sqrt{a}} + \sqrt(1 + \frac{x_2^{2}}{a}) | + constant \right).

Usage
-----

Example - GravityCorrection

.. testcode:: General: workspace with instrument where the x axis is parrallel and in direction to the beam.

        # A workspace with an instrument defined, each pixel has a side length of 4 mm
        ws = CreateSampleWorkspace(WorkspaceType = 'Histogram',
                                   NumBanks = 2,
                                   NumMonitors = 0,
                                   BankPixelWidth = 10,
                                   XUnit = 'TOF',
                                   XMin = 0,
                                   XMax = 20000,
                                   BinWidth = 200,
                                   PixelSpacing = 0.008,
                                   BankDistanceFromSample = 5,
                                   SourceDistanceFromSample = 10)

        # Perform correction due to gravitation effects
        wsCorrected = GravityCorrection(ws, "slit1", "slit2")

Output:

.. testoutput:: General
    :options: +NORMALIZE_WHITESPACE

.. include:: ../usagedata-note.txt

.. testcode:: ILL Figaro: workspace with instrument where the z axis is parallel and in direction to the beam.

        # Load an ILL Figaro File into a Workspace2D
        #if (!ws.getRun()->hasProperty("GravityCorrected"))
        ws = LoadILLReflectometry('ILL/Figaro/xxxx.nxs')

        # Perform correction due to gravitation effects
        wsCorrected = GravityCorrection(ws)

Output:

.. testoutput:: ILL Figaro
    :options: +NORMALIZE_WHITESPACE

.. categories::

