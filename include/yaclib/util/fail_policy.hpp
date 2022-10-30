#pragma once

namespace yaclib {

/**
 * This Policy describe how algorithm interpret if Future will be fulfilled by fail (exception or error)
 *
 * None      -- fail same as ok, another words save all fails
 * FirstFail -- save first fail, default for WhenAll
 * LastFail  -- save last fail,  default for WhenAny
 */
enum class FailPolicy : unsigned char {
  None = 0,
  FirstFail = 1,
  LastFail = 2,
};

}  // namespace yaclib
