"""Independent clock, model-composition and boundary contracts for Earth FK5."""
import copy
from pathlib import Path
import unittest
import erfa
import numpy as np
import EarthFk5Oracle as oracle
from Fk5StandardsOracleTests import PublishedReferenceTests

ROOT=Path(__file__).resolve().parents[3]

class EarthContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.eop=oracle.read_eop(ROOT/'application/data/planetary_coeff/eopc04_08.62-now')

    def tai(self,date):
        t=erfa.utctai(*erfa.dtf2d('UTC',*date));return float(t[0]-oracle.JD+t[1])

    def test_leap_second_is_continuous_in_tai_and_ut1(self):
        left=self.tai((2016,12,31,23,59,59.5));right=self.tai((2017,1,1,0,0,.5))
        a,b=[oracle.clock(v,self.eop) for v in [left,right]]
        self.assertAlmostEqual((right-left)*86400,2.,delta=1e-6)
        self.assertAlmostEqual(((b['ut1'][0]-a['ut1'][0])+(b['ut1'][1]-a['ut1'][1]))*86400,2.,delta=5e-6)
        self.assertLess(abs(a['ut1_minus_tai']-b['ut1_minus_tai']),1e-5)

    def test_midnight_split_preserves_side_of_daily_knot(self):
        from decimal import Decimal, localcontext
        epoch=self.tai((2004,4,6,0,0,0.));c=oracle.clock(epoch,self.eop)
        with localcontext() as ctx:
            ctx.prec=50
            seconds=(Decimal.from_float(epoch)-Decimal('23101.5'))*Decimal(86400)-Decimal(32)
        self.assertLess(seconds,0)
        self.assertEqual(c['utc_day'],53100)
        self.assertGreater(c['utc_fraction'],.999999999)
        self.assertEqual(c['lod'],next(r['lod'] for r in self.eop if r['mjd']==53100))

    def test_daily_eop_values_and_midpoint(self):
        times=[self.tai((2004,4,6,h,0,0.001)) for h in [0,12]]
        a,b=[oracle.clock(v,self.eop) for v in times];left,right=a['eop_brackets'][2:]
        self.assertAlmostEqual(b['x'],(left['x']+right['x'])/2,delta=1e-9)
        self.assertAlmostEqual(b['y'],(left['y']+right['y'])/2,delta=1e-9)
        self.assertEqual(a['lod'],b['lod'])

    def test_outside_eop_rejects(self):
        for epoch in [0,50000]:
            with self.assertRaises(AssertionError):oracle.clock(epoch,self.eop)

    def test_composition_inverse_and_rate(self):
        c=oracle.clock(self.tai((2004,4,6,12,0,0.)),self.eop);r,d=oracle.reduction(c)
        self.assertLess(np.max(abs(r@r.T-np.eye(3))),2e-15)
        self.assertLess(np.max(abs(d@r.T+r@d.T)),2e-19)
        # Differentiate sidereal phase only, as NASA's explicit rate approximation.
        omega=7.29211514670698e-5*(1-c['lod']/86400);h=.02
        axis=np.array([0.,0.,1.]);pn=erfa.pnm80(*c['tt']);pef_z=r@pn.T@axis
        independent=-omega*np.cross(pef_z[:,None],r,axis=0)
        self.assertLess(np.max(abs(d-independent)),3e-20)
        self.assertGreater(np.max(abs(r-r.T)),.1)

    def test_missing_polar_motion_or_wrong_clock_is_distinguishable(self):
        c=oracle.clock(self.tai((2004,4,6,12,0,0.)),self.eop);r,d=oracle.reduction(c)
        for altered in [dict(c,x=0,y=0),dict(c,ut1=c['tt'])]:
            alternative,_=oracle.reduction(altered);self.assertGreater(np.max(abs(r-alternative)),1e-7)

    def test_pre1997_complementary_terms_policy(self):
        c=oracle.clock(self.tai((1990,6,1,12,0,0.)),self.eop);tt=c['tt']
        difference=erfa.eqeq94(*tt)-erfa.nut80(*tt)[0]*np.cos(erfa.obl80(*tt))
        self.assertGreater(abs(difference),1e-9)

if __name__=='__main__':unittest.main()
