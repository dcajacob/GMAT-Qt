=========================================================================
National Space Science Data Center      Data set  MN-61C       April 1987 
=========================================================================

NAME:  	     Mass-Spectrometer-Incoherent-Scatter (MSIS) neutral
             atmosphere model 

SCIENTIFIC CONTACT:      A. Hedin, GSFC code 614, Greenbelt, MD 20771

NSSDC CONTACT:		 D. Bilitza, GSFC/NSSDC code 633, Greenbelt,
             		 MD 20771, tel. (301) 441-4193
             		 SPAN:  NCF::[BILITZA]

                                                                bytes
FILES:       (1) FORTRAN source code for          msis86.for    28030
                 MSIS-86 subroutines 
             (2) data file needed for (1)         msis86.dat    19618
             (3) FORTRAN test program for (1)     m86tes.for     2640
             (4) output of test program           m86tes.dat     4958
             (5) FORTRAN driver program, allows   m86dri.for    21616
                 fast display of MSIS profiles     
             (6) user manual for older version of m86dri.doc    42802
	     (7) test run of M86DRI with examples m86dri.log    25946
	     (8) this file                     AAAREADME.DOC     3847

BRIEF DESCRIPTION:

The Mass-Spectrometer-Incoherent-Scatter-1986 (MSIS-86) neutral atmosphere 
model describes the neutral temperature and the densities of He, O, N2, O2, 
Ar, H, and N. The MSIS model is based on the extensive data compilation and 
analysis work of A. E. Hedin and his collaborators [A. E. Hedin et al., J. 
Geophys. Res. 82, 2139-2156, 1977; A. E. Hedin, J. Geophys. Res. 88, 10170-
10188, 1983; A. E. Hedin, J. Geophys. Res. 92,  4649, 1987]. MSIS-86 
constitutes the upper part of the COSPAR International Reference Atmosphere 
(CIRA-86). 

Data sources for the present model include temperature and density
measurements from several rockets, satellites (OGO-6, San Marco 3, Aeros-A, 
AE-C, AE-D, AE-E, ESRO 4 and DE-2) and incoherent scatter radars (Millstone 
Hill, St. Santin, Arecibo, Jicamarca, and Malvern). Since the MSIS-83 model, 
terms were added or changed to better represent seasonal variations in the 
polar regions under both quiet and magnetically disturbed conditions and 
local time variations in the magnetic activity effect. In addition a new 
species, atomic nitrogen, was added to the list of species covered by the 
model. 

The model expects as input: year, day of year, universal time, altitude, 
geodetic latitude and longitude, local apparent solar time, solar F10.7 flux 
(for previous day and 3-month average) and magnetic Ap index (daily or Ap 
history for last 59 hours). For this conditions the following output 
parameters are calculated: number density of He, O, N2, O2, Ar, H and N, 
total mass density, neutral temperature and exospheric temperature. For 
diagnostic purposes the source code is equipped with 23 flags to turn on/off 
particular variations.

The software package includes a test program (M86TES) supplied by A. Hedin 
and an interactive driver M86DRI developed at NSSDC. M86DRI produces tables 
of temperature and densities. Any of the model input parameters can be chosen 
as the variable for the table output. The model is also available on tape, 
and on floppy disk for use on IBM compatible PCs. 

=========================================================================
National Space Science Data Center      Data set  MN-61C       April 1987 
=========================================================================
