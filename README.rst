GMAT Qt desktop preview
=======================

Public previews of the Qt 6 frontend and related engine work, based on NASA
GMAT. The default branch is ``main``. The frozen preview branch
``codex/stable-qt-preview-20261104`` is also retained.

`Download preview packages <https://github.com/dcajacob/GMAT-Qt/releases/tag/qt-preview-20261104-winvm1>`_:

* `Windows x64 <https://github.com/dcajacob/GMAT-Qt/releases/download/qt-preview-20261104-winvm1/GMAT-Qt-preview-20261104-windows-x64.zip>`_
* `Apple Silicon Mac (macOS 15+) <https://github.com/dcajacob/GMAT-Qt/releases/download/qt-preview-20261104-winvm1/GMAT-Qt-preview-20261104-macos-arm64.zip>`_
* `Intel Mac (macOS 15+) <https://github.com/dcajacob/GMAT-Qt/releases/download/qt-preview-20261104-winvm1/GMAT-Qt-preview-20261104-macos-x64.zip>`_

Extract the complete package and keep its folder together. The Mac apps are
ad-hoc signed and are not notarized. Checksums and native verification receipts
are attached to the release. The Windows package uses source ``cad8e5a``; the
earlier Mac source commits are identified in the release notes and receipts.

See ``TESTING.rst`` for build instructions, testing suggestions and known limits.
The source-versioned NASA acceptance campaign is at 1,052/1,158 original
requirements; full acceptance and scientific qualification remain incomplete.

The original project overview and license remain in ``README.txt`` and
``License.txt``.
