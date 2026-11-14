// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <QString>
#include <QStringList>
#include <functional>
#include <osg/Node>
#include <osg/ref_ptr>

// Static mesh/finite-primitive subset of POV-Ray SDL. Unsupported constructs
// fail as a whole; the caller receives a file/line diagnostic. Includes
// search the including file's directory, model directory, then includePaths.
// mesh2 supports solid finishes selected per face or blended per corner.
// mesh triangle texture_list blocks select three declared solid textures.
// Static expressions include scalar math, vector operations/rotations and
// component selection. Procedural scene evaluation remains separate.
// Arrays support fixed ranks up to five, dimensionless dynamic vectors,
// optional sparse initializers and explicitly mixed element types. Ordinary
// arrays retain homogeneous types. Copies retain shape and mode independently.
// Parenthesized comparisons/logical/conditional expressions and select use
// POV's eager scene-parser evaluation and default epsilon boolean rules.
// Conditional directives skip inactive tokens without expanding includes or
// changing symbols. Message directives retain their diagnostic/error behavior.
// For/while loops replay source bookmarks with live symbol scopes.
// Switch clauses retain native case/range matching, fallthrough and default
// behavior; break selects the nearest loop, switch or macro invocation.
// Macros retain source text, execute in caller-parented local scopes and
// accept typed values or references to caller identifier/array bindings.
osg::ref_ptr<osg::Node> readPovModel(const QString &path,QString *error=nullptr);
osg::ref_ptr<osg::Node> readPovModel(const QString &path,const QStringList &includePaths,QString *error=nullptr);
// Called on the importing thread; the predicate must be thread-safe and must
// not process GUI events or mutate the parser. Thread interruption is also
// checked by every overload. Cancellation returns no partial model.
using PovModelCancel=std::function<bool()>;
osg::ref_ptr<osg::Node> readPovModel(const QString &path,const QStringList &includePaths,QString *error,const PovModelCancel &cancel);
