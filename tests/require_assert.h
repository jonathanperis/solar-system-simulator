#ifndef SOLAR_TESTS_REQUIRE_ASSERT_H
#define SOLAR_TESTS_REQUIRE_ASSERT_H

/* Every C test reports failure through assert(). Defining NDEBUG removes
 * those checks (and any work done inside them), so a test binary would pass
 * while proving nothing. Refuse to build in that configuration. */
#ifdef NDEBUG
#error "C tests require assert(); build them without -DNDEBUG"
#endif

#endif
