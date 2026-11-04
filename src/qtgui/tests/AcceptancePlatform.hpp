#pragma once
#include <QGuiApplication>
#include <QtGlobal>

// The native launcher opts in its child without calling unattended execution
// visible. Rendering/exposure tests retain their own stricter guards.
inline bool acceptanceWidgetPlatform()
{
   const auto platform=QGuiApplication::platformName();
   return platform=="offscreen" ||
      ((qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible" ||
        qEnvironmentVariable("GMAT_QT_NATIVE_TEST_LAUNCH")=="1") &&
       (platform=="xcb" || platform=="windows"));
}
