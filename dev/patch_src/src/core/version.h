// Version of the patch: DDraw.dll's version resource (buzzpatch.rc) says it, and the installer reads it from there
// to show it and to know when an installed engine is older than the one it carries. build.bat adds the build:
// days since 2026-01-01 and minute of the day, so a new build of the same version can update an installed one.
#pragma once
#define PATCH_VERSION_MAJOR 1
#define PATCH_VERSION_MINOR 1
#define PATCH_VERSION_TEXT "1.1"
#ifndef PATCH_BUILD_DAY
#define PATCH_BUILD_DAY 0
#define PATCH_BUILD_MINUTE 0
#endif
