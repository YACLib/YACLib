#pragma once

namespace yaclib {

/**
 * This Policy describe how When* algorithm interpret if Future will be fulfilled by error or exception
 *
 * None -- Fail same as Ok
 * FirstFail -- save first fail
 * LastFail -- save last fail
 */
enum class WhenPolicy {
  None,
  FirstFail,
  LastFail,
};

}  // namespace yaclib
