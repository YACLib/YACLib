# Style guide

## Formatting

You can use our git hooks, see [githooks](../.githooks) for detail.

Only additions here (see .clang-format). See .clang-tidy for naming convention.

* Insert empty line at the end of each file.
* Insert empty lines after open brace and before close brace of namespace definition
  (exception: don't add empty line when the namespace started just after the parent one).

## Code style

Only additions here (see .clang-tidy).

* Write `final` for classes if possible
* Write `noexcept` for methods if possible
* Write `final` for virtual methods, if not `override`
* Don't write any combination of `virtual`, `override`, `final` at the same time
* Don't use relative include `#include "..."`, use `#include <...>` instead of this
* Don't use `#include guard`, use `#pragma once`
* Write `=` for constructor if possible, if not try `{}`, if not `()`
* Always use typename for templates
