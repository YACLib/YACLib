# Memory Sanitizer doesn't work without compile stdlib with it
list(APPEND YACLIB_LINK_OPTIONS -fsanitize=memory)
list(APPEND YACLIB_COMPILE_OPTIONS -fsanitize=memory)
