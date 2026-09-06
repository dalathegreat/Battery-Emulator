#include <gtest/gtest.h>

#include <cctype>
#include <fstream>
#include <string>

#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/devboard/utils/ota_confirm_gate.h"

/* A fresh image is confirmed by a RUNNING system, and every steady state
 * this firmware can be in has to reach that confirmation.
 *
 * Once verifyRollbackLater() defers the mark to us, not confirming is
 * not "nothing happens" - the bootloader undoes the update on the next reset.
 * So the dangerous shape is a normal operating state that never gets to the
 * mark, and one already exists: on the wizard branches setup() returns early,
 * above the line that starts the core tick. A board in wizard mode is a fresh
 * or factory-reset board, which is exactly the one being rebooted.
 *
 * Two halves below. The first exercises the gate. The second reads
 * Software.cpp, because Software.cpp is not in this binary - which is also why
 * a mark placed there could sit wrong without any test noticing.
 */

namespace {

/* Strip comments out, before anything is asserted about the code.
 *
 * Every test below decides whether the firmware does something by looking for
 * the text of a call, and this file is surrounded by comments that NAME those
 * calls - the placement comment in core_loop says `ota_confirm_check`, the one
 * in setup() explains why the mark is gone. Commenting a call out then leaves
 * its name in place and the assertion passes on dead code, which is exactly
 * how an earlier mutation harness certified a mutation it never ran. Newlines
 * are kept so brace depth and ordering still mean what they meant.
 */
std::string strip_comments(const std::string& src) {
  std::string out;
  out.reserve(src.size());
  for (size_t i = 0; i < src.size();) {
    if (src.compare(i, 2, "//") == 0) {
      while (i < src.size() && src[i] != '\n') {
        ++i;
      }
    } else if (src.compare(i, 2, "/*") == 0) {
      const size_t end = src.find("*/", i + 2);
      const size_t stop = end == std::string::npos ? src.size() : end + 2;
      for (; i < stop; ++i) {
        if (src[i] == '\n') {
          out += '\n';
        }
      }
    } else {
      out += src[i++];
    }
  }
  return out;
}

std::string read_source(const std::string& relative_to_test_dir) {
  // Located relative to this file rather than through a CMake define, so the
  // test needs no build-system plumbing to run.
  const std::string self = __FILE__;
  const std::string dir = self.substr(0, self.find_last_of('/'));
  const std::string path = dir + "/" + relative_to_test_dir;
  std::ifstream src(path);
  EXPECT_TRUE(src.is_open()) << "this test reads " << path;
  return strip_comments(std::string((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>()));
}

// The body of a function, by brace depth from its signature line.
std::string function_body(const std::string& src, const std::string& signature) {
  const size_t at = src.find(signature);
  EXPECT_NE(at, std::string::npos) << "no `" << signature << "` in the source this test reads";
  if (at == std::string::npos) {
    return "";
  }
  const size_t open = src.find('{', at);
  int depth = 0;
  for (size_t i = open; i < src.size(); ++i) {
    if (src[i] == '{') {
      ++depth;
    } else if (src[i] == '}') {
      if (--depth == 0) {
        return src.substr(open, i - open + 1);
      }
    }
  }
  return "";
}

// The `{ ... }` block that follows `at`, by brace depth. Used to ask whether a
// call sits INSIDE a particular guard rather than merely somewhere near it.
std::string brace_block_at(const std::string& src, size_t at) {
  const size_t open = src.find('{', at);
  if (open == std::string::npos) {
    return "";
  }
  int depth = 0;
  for (size_t i = open; i < src.size(); ++i) {
    if (src[i] == '{') {
      ++depth;
    } else if (src[i] == '}') {
      if (--depth == 0) {
        return src.substr(open, i - open + 1);
      }
    }
  }
  return "";
}

// The text between the parentheses of the first `call` in `src`, trimmed.
std::string call_argument(const std::string& src, const std::string& call) {
  const size_t at = src.find(call);
  if (at == std::string::npos) {
    return "";
  }
  const size_t open = at + call.size();
  const size_t close = src.find(')', open);
  if (close == std::string::npos) {
    return "";
  }
  std::string arg = src.substr(open, close - open);
  const size_t first = arg.find_first_not_of(" \t\n");
  const size_t last = arg.find_last_not_of(" \t\n");
  return first == std::string::npos ? "" : arg.substr(first, last - first + 1);
}

// A bare C identifier - no literal, no arithmetic, no call.
bool is_identifier(const std::string& s) {
  if (s.empty() || (!isalpha(static_cast<unsigned char>(s[0])) && s[0] != '_')) {
    return false;
  }
  for (const char c : s) {
    if (!isalnum(static_cast<unsigned char>(c)) && c != '_') {
      return false;
    }
  }
  return true;
}

class OtaConfirmGate : public ::testing::Test {
 protected:
  void SetUp() override { ota_confirm_gate_reset(); }
  void TearDown() override { ota_confirm_gate_reset(); }
};

}  // namespace

/* The boundary, stated as the ruling states it: nothing is owed at
 * 42 * 1000 - 1 ms, and the confirmation is owed at 42 * 1000 exactly. The
 * constant is the interesting part of the change, so it is pinned rather than
 * left to be read off a comment.
 */
TEST_F(OtaConfirmGate, NothingIsOwedOneMillisecondShortOfTheWindow) {
  EXPECT_EQ(OTA_CONFIRM_UPTIME_MS, 42u * 1000u);

  ota_confirm_check(0);
  ota_confirm_check(OTA_CONFIRM_UPTIME_MS - 1);
  EXPECT_FALSE(ota_confirm_take_pending()) << "an image was confirmed before it had survived the window";
}

TEST_F(OtaConfirmGate, TheConfirmationIsOwedAtTheWindowExactly) {
  ota_confirm_check(OTA_CONFIRM_UPTIME_MS);
  EXPECT_TRUE(ota_confirm_take_pending());
}

/* The write happens once per boot. The check runs on every 1 ms tick and the
 * take runs on every pass of loop(), so "once" is the property that keeps this
 * from becoming a flash write per millisecond for the rest of the board's
 * uptime.
 */
TEST_F(OtaConfirmGate, TheConfirmationIsTakenExactlyOnce) {
  for (uint32_t t = OTA_CONFIRM_UPTIME_MS; t < OTA_CONFIRM_UPTIME_MS + 10; ++t) {
    ota_confirm_check(t);
  }
  EXPECT_TRUE(ota_confirm_take_pending());

  for (uint32_t t = OTA_CONFIRM_UPTIME_MS + 10; t < OTA_CONFIRM_UPTIME_MS + 20; ++t) {
    ota_confirm_check(t);
    EXPECT_FALSE(ota_confirm_take_pending()) << "a second confirmation was owed at uptime " << t;
  }
}

/* A deliberate restart inside the window keeps the update. Without this, the
 * most ordinary thing a user does after updating - press restart - is what
 * reverts it.
 */
TEST_F(OtaConfirmGate, AnIntentionalRestartConfirmsBeforeTheWindowElapses) {
  ota_confirm_check(1000);
  ASSERT_FALSE(ota_confirm_take_pending());

  ota_confirm_request();
  EXPECT_TRUE(ota_confirm_take_pending());
}

/* ...and once confirmed, a restart does not arm a second write. */
TEST_F(OtaConfirmGate, AnIntentionalRestartAfterConfirmationOwesNothing) {
  ota_confirm_check(OTA_CONFIRM_UPTIME_MS);
  ASSERT_TRUE(ota_confirm_take_pending());

  ota_confirm_request();
  EXPECT_FALSE(ota_confirm_take_pending());
}

/* graceful_restart() is the one restart path in the tree (#2551), so it is the
 * one place this has to be wired. Testing it through safety.cpp rather than by
 * reading the source, because safety.cpp IS in this binary.
 */
TEST_F(OtaConfirmGate, GracefulRestartArmsTheConfirmation) {
  reset_all_events();
  ota_confirm_check(1000);
  ASSERT_FALSE(ota_confirm_take_pending());

  graceful_restart();
  EXPECT_TRUE(ota_confirm_take_pending()) << "a requested restart inside the window would revert the update";
}

/* ---- the source-readable half ---- */

/* The mark is in runtime cadence, not in setup(). Two things would be wrong if
 * it went back: an image confirmed at the end of setup() has survived driver
 * construction and nothing else, and the previous placement sat immediately
 * after the core_loop task was created, so whether the first loop pass was
 * covered was decided by a race of microseconds.
 */
TEST(OtaConfirmPlacement, SetupDoesNotConfirmTheImage) {
  const std::string setup = function_body(read_source("../Software/Software.cpp"), "void setup() {");
  ASSERT_FALSE(setup.empty());

  EXPECT_EQ(setup.find("mark_ota_image_valid"), std::string::npos)
      << "setup() confirms the image again - reaching the end of setup() is not evidence the image works";
  EXPECT_EQ(setup.find("ota_confirm_service"), std::string::npos)
      << "setup() performs the confirmation write - it belongs in runtime cadence";
}

/* The check has to be unconditional inside the core tick. Inside the 10 ms or
 * 1 s sub-task it would still fire, but it would then be witnessing that ONE
 * sub-task ran, which is a weaker statement than the tick itself coming round.
 */
TEST(OtaConfirmPlacement, TheCoreTickChecksUnconditionally) {
  const std::string core_loop = function_body(read_source("../Software/Software.cpp"), "void core_loop(void*) {");
  ASSERT_FALSE(core_loop.empty());
  ASSERT_NE(core_loop.find("ota_confirm_check("), std::string::npos)
      << "the core tick no longer checks - in normal mode nothing would ever confirm the image";

  // Depth 1 is the while-loop body; anything deeper is inside one of the
  // conditional sub-tasks.
  int depth = 0;
  int depth_at_check = -1;
  for (size_t i = 0; i < core_loop.size(); ++i) {
    if (core_loop[i] == '{') {
      ++depth;
    } else if (core_loop[i] == '}') {
      --depth;
    } else if (core_loop.compare(i, 18, "ota_confirm_check(") == 0) {
      depth_at_check = depth;
    }
  }
  EXPECT_EQ(depth_at_check, 2) << "the confirmation check sits inside a conditional in core_loop - it must run on "
                                  "every tick, because the tick coming round is the thing it is measuring";
}

/* The write side is reached from ordinary main-task context. */
TEST(OtaConfirmPlacement, LoopPerformsTheConfirmationWrite) {
  const std::string loop = function_body(read_source("../Software/Software.cpp"), "void loop() {");
  EXPECT_NE(loop.find("ota_confirm_service("), std::string::npos)
      << "loop() no longer services the gate - the check would set a flag nobody acts on";
}

/* THE COMPOSITION GUARD, and the reason this item exists.
 *
 * `setup()` returning early means the core tick is never started, so the whole
 * of the normal-mode confirmation path is skipped. On the wizard branches that
 * is exactly what happens, and wizard mode is a normal operating state - its
 * own comment calls it "a strict PREFIX of a normal boot".
 *
 * So: any early exit from setup() has to be matched by a steady state that
 * checks for itself. connectivity_loop() is the loop that keeps running when
 * setup() stops early - it is the task that serves the wizard UI - so that is
 * where the second check belongs.
 *
 * On a tree whose setup() runs straight through, this asserts nothing. It goes
 * red the moment an early return is added without the matching check, which is
 * the merge these two lanes must not make unnoticed.
 */
TEST(OtaConfirmPlacement, AnEarlyReturnFromSetupBringsItsOwnCheck) {
  const std::string src = read_source("../Software/Software.cpp");
  const std::string setup = function_body(src, "void setup() {");
  ASSERT_FALSE(setup.empty());

  if (setup.find("return;") == std::string::npos) {
    GTEST_SKIP() << "setup() has no early exit on this branch - nothing to compose with yet";
  }

  const std::string service = function_body(src, "void connectivity_loop(void*) {");
  ASSERT_FALSE(service.empty());
  ASSERT_NE(service.find("ota_confirm_check("), std::string::npos)
      << "setup() can now return before the core tick is started, and the loop that keeps running when it does "
         "never confirms the image. A board that boots into that state has a good update reverted on its next "
         "reset, repeatedly.";

  /* And it has to be GATED on wizard mode. Presence alone was what this
     asserted, and an ungated check passes that while quietly weakening the
     normal boot: connectivity_loop runs there too, so whichever of the two
     loops reaches 42 s first would confirm, and the witness degrades from "the
     core tick has been coming round" to "networking survived". Both are true
     of a healthy board; only the first is true of a board whose core tick has
     stopped. */
  const size_t gate = service.find("if (wizard_mode_active()) {");
  ASSERT_NE(gate, std::string::npos) << "the wizard-mode check in connectivity_loop is not gated on "
                                        "wizard_mode_active() - in a normal boot it would confirm on the "
                                        "networking task's liveness instead of the core tick's";
  EXPECT_NE(brace_block_at(service, gate).find("ota_confirm_check("), std::string::npos)
      << "connectivity_loop checks outside the wizard_mode_active() gate - see above";
}

/* WHAT the core tick passes to the check, not just that it calls it.
 *
 * The placement tests above pin that a call exists and that it is
 * unconditional, and both survive `ota_confirm_check(0)` and
 * `ota_confirm_check(currentMillis - previousMillis10ms)` - a plausible
 * copy-paste from the two lines under it. Either one compiles, keeps every
 * other test green, and means the gate never fires in normal mode: nothing
 * confirms, and the bootloader reverts a good update on every reset. That is
 * the whole failure this item exists to prevent, so the argument is pinned to
 * absolute uptime the same way the constant is.
 */
TEST(OtaConfirmPlacement, TheCoreTickChecksAgainstAbsoluteUptime) {
  const std::string core_loop = function_body(read_source("../Software/Software.cpp"), "void core_loop(void*) {");
  ASSERT_FALSE(core_loop.empty());

  const std::string argument = call_argument(core_loop, "ota_confirm_check(");
  ASSERT_FALSE(argument.empty()) << "no ota_confirm_check(...) call in core_loop";

  EXPECT_TRUE(is_identifier(argument)) << "the core tick passes `" << argument
                                       << "` to ota_confirm_check. It takes absolute uptime, not a literal and not "
                                          "an interval - an elapsed-time expression never reaches 42 s and nothing "
                                          "would ever confirm";
  EXPECT_NE(core_loop.find(argument + " = millis();"), std::string::npos)
      << "`" << argument << "` is passed as the uptime but is not assigned from millis() in core_loop";
}

/* The write side actually writes.
 *
 * LoopPerformsTheConfirmationWrite pins that loop() CALLS ota_confirm_service();
 * nothing pins what that function does, and it lives in ota_rollback.cpp, which
 * has no host build - the file the change's own note calls out as the one where
 * a mistake "could sit unnoticed". Gutting the body leaves all 216 tests green.
 * Read it, then, the same way Software.cpp is read.
 */
TEST(OtaConfirmPlacement, TheServiceWritesExactlyWhenTheGateSaysSo) {
  const std::string service =
      function_body(read_source("../Software/src/devboard/utils/ota_rollback.cpp"), "void ota_confirm_service(void) {");
  ASSERT_FALSE(service.empty());

  const size_t gate = service.find("if (ota_confirm_take_pending()) {");
  ASSERT_NE(gate, std::string::npos) << "ota_confirm_service() no longer performs the write on exactly the pass the "
                                        "gate hands it - the check would set a flag nobody acts on";
  EXPECT_NE(brace_block_at(service, gate).find("mark_ota_image_valid()"), std::string::npos)
      << "the gate is taken but the image is never marked valid - every update would be reverted on the next reset";
}
