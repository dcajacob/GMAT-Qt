# Native vector pixel qualification — 2026-10-04

The previously unverified native Relative Position and Body-Fixed vector pixels
now have a focused private Xvfb/software-OpenGL check. This adds missing evidence;
no application code, numerical mission, completed audit or old passing suite was
rerun. Source checkpoint is `42dc8940`, with only the new test and CMake registration.

`QtGui.NativeVectors` passes in **1.50 s**, renderer **llvmpipe (LLVM 21.1.8,
256 bits)**. A fixed independent pinhole projection checks colored shaft positions,
endpoints, arrowheads and empty regions before/beyond them. Both vector types cover
Auto and Manual length, translated/rotated replay frames, recoloring, endpoint
labels, absent/invalid samples, removal and return to an identical earlier frame.
Combined-frame pixels distinguish both arrows. Raw curve/vector/camera snapshots
remain unchanged. The oracle does not use the production vector geometry helper.

The three retained captures were visually inspected: arrows point in the expected
directions, remain separated in the combined frame, and labels are readable.

- [Build log](native-vectors-20261004-build.txt)
- [Verbose check and individual pixel counts](native-vectors-20261004-ctest.txt)
- [Runtime and source identity receipt](native-vectors-20261004.json)
- [Combined replay frame](native-vectors-20261004-both-frame2.png)
- [Relative Position label](native-vectors-20261004-relative-label.png)
- [Body-Fixed label](native-vectors-20261004-body-label.png)

The application remains 6,306,584 bytes / SHA e48df911… from the LastN checkpoint;
base/util/startup also remain unchanged. No rebuild was needed for this test-only
addition. This supersedes historical statements that native vector pixels were
unverified, within the bounded scenarios above. It does not establish physical-GPU
or host desktop acceptance. Live-host testing remains stopped; Mutter's observed
NULL Wayland-resource fault, unproven GMAT attribution and all nine original
acceptance gates retain their distinct limits. Windows/macOS/MATLAB are deferred.
