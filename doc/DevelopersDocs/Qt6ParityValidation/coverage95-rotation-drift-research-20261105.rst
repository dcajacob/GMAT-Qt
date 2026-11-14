Rotation drift rate: requirements research, 2026-11-05
====================================================

Finding
-------

The material reviewed does not establish that FRR!E227 requires spin
acceleration. The stronger interpretation is that the final two orientation
requirements were intended to be the third angle/rate pair, with E226's
``Rotation rate at reference epoch`` containing a naming error. On that
interpretation E227 corresponds to the existing constant ``RotationRate``
in degrees/day. This is an inference, not a recovered NASA correction or a
qualification decision. Do not introduce Wddot or change original row credit
on this basis.

Original source
---------------

``doc/SystemDocs/Requirements/GMAT Requirements.xlsx``, FRR!E222:E227,
lists these six fields in order:

* Spin-axis right ascension at the reference epoch; its drift rate.
* Spin-axis declination at the reference epoch; its drift rate.
* Rotation rate at the reference epoch; rotation drift rate.

Their source identifiers are FRR-16.7.1 through FRR-16.7.6. The last pair is
dimensionally ambiguous; neither units nor a governing equation are supplied
in these rows. The workbook has no comment parts. Its embedded metadata records
creation on 2011-06-08 and modification on 2012-05-06. The original bytes are
unchanged (SHA256 736847e909ca550731386261073e46a9fa3ac8d78f53f0c37dd5d93b11a85c23).
The 2012 requirements-to-test matrix in this repository does not supply a
matching drift-rate/RotationRate definition. This absence does not establish
the original author's intended meaning.

GMAT's documented model and owning implementation
------------------------------------------------

``doc/help/src/Resource_CelestialBody.xml``, under "Celestial body orientation
model", explicitly describes three angles and their three rates. They are
``SpinAxisRAConstant``, ``SpinAxisRARate``, ``SpinAxisDECConstant``,
``SpinAxisDECRate``, ``RotationConstant``, and ``RotationRate``. Its RotationRate
field is the body's spin rate in degrees/day; RotationConstant is an angle.

``CelestialBody::GetBodyCartographicCoordinates`` in
``src/base/solarsys/CelestialBody.cpp`` implements, for a custom body's simple
orientation model::

    alpha = alpha0 + alphaRate * T
    delta = delta0 + deltaRate * T
    W     = W0 + RotationRate * d
    Wdot  = RotationRate

Here d is the TDB interval from OrientationEpoch in days, and T is d/36525 in
Julian centuries. Pole rates use degrees/Julian century; the spin rate uses
degrees/day. The six-element orientation vector contains no quadratic spin
coefficient. This is source inspection, not a new numerical verification run.
The pre-change CelestialBody.hpp comment reversed the definitions of d and T
and described an obsolete time scale. The implementation pass corrected that
comment to match the executable equations; no orientation equation was changed.

Historical NASA mathematical context
------------------------------------

The `2008 NASA GMAT Mathematical Specifications draft
<https://ntrs.nasa.gov/api/citations/20080031744/downloads/20080031744.pdf>`_,
section 3.4.9 (printed pages 24-25), describes body orientation using the
pole direction and prime-meridian angle, and uses the derivative of W in
the body-fixed transformation rate. It does not define the worksheet phrase
or identify an additional custom-body spin-acceleration input. It supports
the angle/rate interpretation but is not a requirement change notice.

What SPICE does, and why it does not settle this requirement
----------------------------------------------------------

NAIF's `PCK Required Reading
<https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/pck.html#Models%20for%20the%20Sun,%20Planets,%20and%20some%20Minor%20Bodies%20in%20Text%20PCK%20Kernels>`_
defines an orientation polynomial with constant, linear and quadratic terms.
Writing its independent variable in days gives W = W0 + W1*d + W2*d*d.
W1 has units degrees/day and W2 degrees/day squared. By differentiation,
physical constant spin acceleration would be 2*W2, so an acceleration input
would enter the angle equation with a factor of one half. A PCK quadratic
coefficient and a physical acceleration must not be conflated.

This establishes a possible additional model, not that the legacy GMAT
worksheet asked for it. If quadratic spin is added as a separate feature,
its field name, coefficient-versus-acceleration convention and time scale
must be explicit. It should not silently redefine the existing RotationRate.

Campaign consequence
--------------------

Preserve the present angle/rate model while continuing independent
implementation work. Retain FRR!E227 and parent E221 as unresolved source
semantics. The E226-to-RotationConstant mapping also needs an explicit
interpretation during the later requirement-disposition phase. No source
assertion, denominator, historical receipt, original worksheet, row count or
qualification gate is changed by this research. No new spin-acceleration
implementation, numerical run or GUI acceptance run was performed.
