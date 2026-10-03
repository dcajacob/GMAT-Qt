#pragma once
#include <QString>
class GmatBase;
bool resourceCanBeRenamed(GmatBase *object);
// Patch only the current document. Literal strings, comments and implicit
// settings are retained. Literal external dependencies are inspected read-only
// using the built document's existing engine paths; potentially affected or
// unresolved dependencies require coordinated script-editor changes.
QString renameResourceSource(const QString &source,GmatBase &object,const QString &name);
