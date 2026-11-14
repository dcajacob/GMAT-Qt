ContactLocator ordinary base-copy repair, 2026-11-08
===================================================

A valid native destination derived from ContactLocator could not initialize
its ContactLocator base state through Copy. The exact-destination-type guard
rejected it before copying. The original MultiTargetContactParameters case
exposed this in its OwnedLocator initialization path.

Copy now permits a derived destination for an ordinary copy when neither
source nor destination carries a FunctionValueContext. The exact supported
source-type check remains. Unsupported context-bearing derived copies still
fail before assignment, preserving the existing value-graph and lifetime
boundary. Provider eligibility and all exact-class function copies remain
unchanged. No object layout, virtual slot or physical equation changes.

The unchanged original native case and ContactLocator function-resource
control pass in both headless and visible modes: four passing executions.
The original test includes owned result destruction, target/reference copying,
invalid-input gates, worker invalidation and repeated initialization. All
1,334 protected original inputs remain unchanged. The complete pre-repair
baseline and failure are retained; current source delta is ContactLocator.cpp
only. This component earns zero new NASA row credit; full parent requirements
and campaign completion remain open.

Stage: build/nasa-coverage/coverage95-20261108-contact-copy-current-review
