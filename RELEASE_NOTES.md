# 4.6.6

This release publishes the current renderer/editor demo with reliability and project-maintenance fixes.

## Fixed

- Linux right-mouse camera look now uses centered relative motion and no longer stalls when the pointer reaches a window edge.
- Framebuffer completeness failures restore the default framebuffer before reporting an error.
- Linux builds prefer `g++-14` explicitly and reject older default GCC installations instead of failing later in module compilation.
- The README now reports the actual 269-record scene and removes obsolete patch-history text and unsupported build commands.

## Project maintenance

- Added automated validation for cfg/table indices, names, deleted records and UV scale.
- Added Windows and Linux CI.
- Added automatic GitHub release packaging after validated pushes to `main`.
