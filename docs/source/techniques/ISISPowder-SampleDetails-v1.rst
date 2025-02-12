.. _isis-powder-diffraction-sampleDetails-ref:

=========================================================
ISIS Powder Diffraction Scripts - SampleDetails Reference
=========================================================

.. contents:: Table of Contents
    :local:


Introduction
------------
The sample details object holds the user defined properties of
the current sample if absorption corrections are required whilst
focusing or a total scattering pdf calculation is being performed.
Only specific instruments support sample absorption
corrections. This can be determined from visiting the
instrument reference:
:ref:`instrument_doc_links_isis-powder-diffraction-ref`.
If the instrument has a *set_sample_details* method it supports sample
absorption corrections.

Before you can use absorption corrections you will need to:
- :ref:`create_sampleDetails_object_isis-powder-diffraction-ref`
- :ref:`set_material_sample_details_isis-powder-diffraction-ref`

Optionally you may also:

- :ref:`set_material_properties_sampleDetails_isis-powder-diffraction-ref`

.. _create_sampleDetails_object_isis-powder-diffraction-ref:

Create SampleDetails Object
---------------------------
This method assumes you are familiar with the concept of objects in Python.
If not more details can be read here: :ref:`intro_to_objects-isis-powder-diffraction-ref`

For more details on any of the parameters set here see:
:ref:`Set Sample<algm-SetSample>`.

Two geometries are currently supported - cylinder and slab.

To create a cylindrical SampleDetails object the following parameters
of the sample geometry are required:

- :ref:`shape-sampleDetails_isis-powder-diffraction-ref` - The sample
  shape (in this case a cylinder)
- :ref:`center_sampleDetails_isis-powder-diffraction-ref` - List of x, y, z
  positions of the sample
- :ref:`height_sampleDetails_isis-powder-diffraction-ref` - Cylinder height
- :ref:`radius_sampleDetails_isis-powder-diffraction-ref` - Cylinder radius

To create a slab SampleDetails object the following parameters of the
sample geometry are required:

- :ref:`shape-sampleDetails_isis-powder-diffraction-ref` - The sample
  shape (in this case a slab)
- :ref:`center_sampleDetails_isis-powder-diffraction-ref` - List of x, y, z
  positions of the sample
- :ref:`thickness-sampleDetails_isis-powder-diffraction-ref` - Slab thickness
- :ref:`width-sampleDetails_isis-powder-diffraction-ref` - Slab width
- :ref:`height_sampleDetails_isis-powder-diffraction-ref` - Slab
  height
- :ref:`angle_sampleDetails_isis-powder-diffraction-ref` - Slab angle

Example
^^^^^^^

..  code-block:: python

    from isis_powder import SampleDetails

    cylinder_height = 3.0
    cylinder_radius = 2.0
    cylinder_position = [0.0, 0.0, 0.2]
    sample_obj = SampleDetails(height=cylinder_height, radius=cylinder_radius,
                               center=cylinder_position, shape="cylinder")

    slab_thickness = 1.0
    slab_obj = SampleDetails(thickness=slab_thickness, shape="slab")

.. _angle_sampleDetails_isis-powder-diffraction-ref:

angle
^^^^^
The angle in degrees between the positive beam axis and the normal to
the face perpendicular to the beam axis when not rotated, increasing
in the anti-clockwise sense. Rotation is performed about the vertical
axis of the instrument's frame of reference.

Example Input:

.. code-block:: python

   sample_obj = SampleDetails(angle=45, ...)

.. _center_sampleDetails_isis-powder-diffraction-ref:

center
^^^^^^
The center of the sample as defined by X, Y and Z
co-ordinates. This co-ordinates must be numeric.

Example Input:

..  code-block:: python

    sample_obj = SampleDetails(center=[-1.0, 0.0, 1.0], ...)

.. _height_sampleDetails_isis-powder-diffraction-ref:

height
^^^^^^^
The height of the sample cylinder in cm. This must be a number
which is greater than 0.

Example Input:

..  code-block:: python

    sample_obj = SampleDetails(height=5.0, ...)

.. _radius_sampleDetails_isis-powder-diffraction-ref:

radius
^^^^^^
The radius of the sample cylinder in cm. This must be a number
which is greater than 0.

Example Input:

..  code-block:: python

    sample_obj = SampleDetails(radius=5.0, ...)

.. _shape-sampleDetails_isis-powder-diffraction-ref:

shape
^^^^^

The shape of the sample. Allowed values are currently **cylinder** and
**slab**.

Example Input:

.. code-block:: python

   sample_obj = SampleDetails(shape="cylinder", ...)

.. _thickness-sampleDetails_isis-powder-diffraction-ref:

thickness
^^^^^^^^^
The thickness of the slab sample in cm. This must be greater
than 0. In the case of HRPD, allowed values are **0.2, 0.5, 1.0, 1.5**

Example Input:

.. code-block:: python

   sample_obj = SampleDetails(thickness=1.0, ...)

.. _width-sampleDetails_isis-powder-diffraction-ref:

width
^^^^^
The width of the slab sample in cm. This must be greater than 0.

Example Input:

.. code-block:: python

   sample_obj = SampleDetails(width=1.0, ...)

.. _set_material_sample_details_isis-powder-diffraction-ref:


Set container details
---------------------

Currently the supported container shapes are HollowCylinder (for Cylinder shaped samples)
and FlatPlateHolder (for Slab shaped samples). These container shapes are inferred from the sample so only a few
container parameters need to be set. These container shapes support the
:ref:`PaalmanPingsMonteCarloAbsorption <algm-PaalmanPingsMonteCarloAbsorption>` method available when focussing.

Example Input:

.. code-block:: python

    from isis_powder import SampleDetails

    cylinder_obj = SampleDetails(height=4.0, radius=1.0, center=[0, 0, 0], shape='cylinder')
    cylinder_obj.set_material(chemical_formula='Si')
    cylinder_obj.set_container(radius=2.0, chemical_formula='V')

    slab_obj = SampleDetails(height=4.0, width=4.0, thickness=2.0, angle=0.0, center=[0, 0, 0], shape='slab')
    slab_obj.set_material(chemical_formula='Si')
    slab_obj.set_container(front_thick=4.0, back_thick=4.0, chemical_formula='V')


Setting the material
--------------------
Having successfully defined the geometry
(see: :ref:`create_sampleDetails_object_isis-powder-diffraction-ref`)
we now must set the material of the sample.

This can only be set once per object without explicitly calling
the reset method or constructing a new object (which is preferred)
see: :ref:`changing_sample_properties_sampleDetails_isis-powder-diffraction-ref`

The following properties are required to set the sample material:

- :ref:`chemical_formula_sampleDetails_isis-powder-diffraction-ref`
- :ref:`number_density_sampleDetails_isis-powder-diffraction-ref`
- :ref:`crystal_density_sampleDetails_isis-powder-diffraction-ref`
  (Optional if *chemical_formula* is an element, otherwise mandatory).

Example
^^^^^^^

..  code-block:: python

    sample_obj.set_material(chemical_formula="V")
    # OR
    sample_obj.set_material(chemical_formula="VNb", number_density=123)

.. _chemical_formula_sampleDetails_isis-powder-diffraction-ref:

chemical_formula
^^^^^^^^^^^^^^^^
The chemical formula of this material. Isotopes can be defined
by the ratios as well. For example V 95.1% Nb 4.9% can be
expressed as *V0.951 Nb0.049*.

See: :ref:`SetSampleMaterial <algm-SetSampleMaterial>` for
more details.

Example Input:

..  code-block:: python

    sample_obj.set_material(chemical_formula="V")
    # Or
    sample_obj.set_material(chemical_formula="V0.951 Nb0.049", ...)

.. _number_density_sampleDetails_isis-powder-diffraction-ref:

number_density
^^^^^^^^^^^^^^
This parameter defines the number density of the property.
When :ref:`chemical_formula_sampleDetails_isis-powder-diffraction-ref`
defines an element this can automatically be calculated by Mantid.

If :ref:`chemical_formula_sampleDetails_isis-powder-diffraction-ref`
is not an element the user must enter this value.

Example Input:

..  code-block:: python

    sample_obj.set_material(number_density=0.123, ...)

.. _crystal_density_sampleDetails_isis-powder-diffraction-ref:

crystal_density
^^^^^^^^^^^^^^^
This parameter defines the full or crystallographic number density of the material.
If this is not set it will use the `number_density` in its place. This value
is used to convert between different PDF types in `Polaris.create_total_scattering_pdf`.

Example Input:

..  code-block:: python

    sample_obj.set_material(crystal_density=0.231, ...)

.. _set_material_properties_sampleDetails_isis-powder-diffraction-ref:

Setting material properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Advanced material properties can be optionally set instead of letting
Mantid calculate them. For more details see:
:ref:`SetSampleMaterial<algm-SetSampleMaterial>`
This can only be set once per object without
explicitly calling the reset method or constructing a new object (which is preferred)
see: :ref:`changing_sample_properties_sampleDetails_isis-powder-diffraction-ref`

These properties are:

- :ref:`absorption_cross_section_sampleDetails_isis-powder-diffraction-ref`
- :ref:`scattering_cross_section_sampleDetails_isis-powder-diffraction-ref`

Example
^^^^^^^

..  code-block:: python

        sample_obj.set_material_properties(absorption_cross_section=123,
                                           scattering_cross_section=456)

.. _absorption_cross_section_sampleDetails_isis-powder-diffraction-ref:

absorption_cross_section
^^^^^^^^^^^^^^^^^^^^^^^^
The absorption cross section for the sample in barns to use
whilst calculating absorption corrections.

.. _scattering_cross_section_sampleDetails_isis-powder-diffraction-ref:

scattering_cross_section
^^^^^^^^^^^^^^^^^^^^^^^^
The scattering cross section for the sample in barns to use
whilst calculating absorption corrections.

.. _changing_sample_properties_sampleDetails_isis-powder-diffraction-ref:

Changing sample properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. warning:: This method is not recommended for changing multiple samples.
             Instead it is recommended you create a new sample details object
             if you need to change properties mid way through a script.
             See :ref:`create_sampleDetails_object_isis-powder-diffraction-ref`
             and :ref:`intro_to_objects-isis-powder-diffraction-ref`.

*Note: The geometry of a sample cannot be changed without creating a new
sample details object*

Once you have set a material by calling *set_material* or set
the properties by calling *set_material_properties* you will
not be able to change (or set) these details without first
resetting the object. This is to enforce the sample properties
being set only once so that users are guaranteed of the state.

To change the chemical material or its advanced properties all
*reset_sample_material*. This will reset **all** details (i.e
advanced properties and chemical properties).

..  code-block:: python

    sample_obj.reset_sample_material()

.. categories:: Techniques
