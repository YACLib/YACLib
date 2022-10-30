#pragma once

namespace yaclib {

/**
 * This Policy describe how algorithm produce result
 *
 * Fifo  -- order of results is fifo
 *  Note: non-deterministic in multithreading environment
 *  Note: now it's default for WhenAll because it allow call delete early
 * Same  -- order of results same as order of input
 */
enum class OrderPolicy : unsigned char {
  Fifo = 0,
  Same = 1,
};

}  // namespace yaclib
