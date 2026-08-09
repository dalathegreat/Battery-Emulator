#ifndef VERSION_H
#define VERSION_H

// Assembles BUILD_VERSION from raw macros set by tools/identify_build.py.
// Each macro is defined only when its value is available.

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

#endif  // VERSION_H
