# Historical TLE example data

`ExampleSatellites-2020-06-23.txt` contains the 25 spacecraft used by the
FalconSats, Falconsat7Contacts and GSFCSats demonstrations. Each three-line record
is copied byte-for-byte from `TLE/Active-2020-06-23.txt` in Thinking Systems'
[original R2020a Ubuntu plugin distribution](https://www.thinksysinc.com/archives/GMATR2020aDownloads.html).
The subset retains the original names, epochs, orbital elements and checksums.

The examples start at 23 June 2020 UTC and retain their original three-day
(FalconSat) or five-day (GSFC) propagation durations. They demonstrate historical
propagation and station visibility. They do not reproduce the original GSFC
December 2019 geometry or predict present-day positions. FalconSat-7 reentered
on 2 July 2021; changing its epoch to today's date cannot provide live contacts.
For a current-time demonstration, obtain recent elements for spacecraft still
in orbit and update both the element input and the mission epoch together.

The Starlink demonstration remains at its original December 2019 epoch and
requires matching elements for all 117 named satellites. This June catalog
omits STARLINK-46 and STARLINK-67 and is not a complete replacement.

Provenance:

- Archive: `GMATR2020a-TlePropagator-Ubuntu20.04.tar.gz`
- Archive SHA256: `9ff6ac53dc97e26ab0f67c1b1d0af6f6f102ef8cce422bf3e3a030b7a319ee34`
- Full catalog SHA256: `6b89cfa0c5bdde2437c74f6dc28ebaee835537f8159631028efe2abe3e2bac2a`
- Source: https://www.thinksysinc.com/downloads/GMATR2020a/GMATR2020a-TlePropagator-Ubuntu20.04.tar.gz

The distributor's Apache 2.0 plugin license is retained at
`../License-TlePropagator.txt`. The preserved original archive, catalog, README
and license are also in the local qualification evidence tree.
