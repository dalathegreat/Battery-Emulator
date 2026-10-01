#pragma once

// The build script (tools/identify_build.py) generates version_autogen.h
// with raw GIT_* / GITHUB_* defines. BUILD_VERSION is assembled below.

#if __has_include("version_autogen.h")
#include "version_autogen.h"
#endif

#if defined(GIT_TAG)
#define BUILD_VERSION GIT_TAG

#elif defined(GIT_ANCESTOR_TAG) && defined(GIT_SHORT_SHA)
#if defined(GITHUB_PR) && defined(GITHUB_PR_HEAD_SHORT_SHA)
#define BUILD_VERSION GIT_ANCESTOR_TAG "dev-" GITHUB_PR_HEAD_SHORT_SHA " (#" GITHUB_PR ")"
#elif defined(GIT_BRANCH)
#define BUILD_VERSION GIT_ANCESTOR_TAG "dev-" GIT_SHORT_SHA " (" GIT_BRANCH ")"
#else
#define BUILD_VERSION GIT_ANCESTOR_TAG "dev-" GIT_SHORT_SHA
#endif

#else
#define BUILD_VERSION "unknown"
#endif
