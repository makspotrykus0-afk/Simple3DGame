---
description: Manage debug logs in C++ code
---
// turbo-all
1. Add logs: Use `std::cout << "[DEBUG] " << ... << std::endl;` to trace logic.
2. Remove logs: Use `grep` or `find` to locate and remove all `[DEBUG]` entries.
3. Temporary UI logs: Use `DrawText()` for in-game visualization.
