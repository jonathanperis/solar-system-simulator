#include "require_assert.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app/input_file.h"

static const char *write_fixture(const char *name, const char *bytes, size_t length)
{
    static char path[256];
    snprintf(path, sizeof(path), "build/tests/%s", name);
    FILE *file = fopen(path, "wb");
    assert(file);
    assert(fwrite(bytes, 1, length, file) == length);
    assert(fclose(file) == 0);
    return path;
}

static void test_reads_complete_text_and_terminates_it(void)
{
    char text[16];
    memset(text, 'x', sizeof(text));
    assert(solar_read_text_file(write_fixture("input-ok.txt", "abc\n", 4), text, sizeof(text)));
    assert(strcmp(text, "abc\n") == 0);
    /* The largest accepted input fills every byte but the terminator. */
    assert(solar_read_text_file(write_fixture("input-full.txt", "0123456789abcde", 15), text, sizeof(text)));
    assert(strlen(text) == 15);
    assert(solar_read_text_file(write_fixture("input-empty.txt", "", 0), text, sizeof(text)));
    assert(text[0] == '\0');
}

static void test_rejects_truncated_missing_and_nul_inputs(void)
{
    char text[16];
    /* One byte too many would silently truncate a descriptor or experiment. */
    assert(!solar_read_text_file(write_fixture("input-long.txt", "0123456789abcdef", 16), text, sizeof(text)));
    /* An embedded NUL hides every later byte from C string parsers. */
    assert(!solar_read_text_file(write_fixture("input-nul.txt", "ab\0cd", 5), text, sizeof(text)));
    assert(!solar_read_text_file("build/tests/input-does-not-exist.txt", text, sizeof(text)));
    assert(!solar_read_text_file("build/tests/input-ok.txt", text, 0));
    /* Failures still leave a terminated (empty or partial) string behind. */
    assert(memchr(text, '\0', sizeof(text)));
}

int main(void)
{
    test_reads_complete_text_and_terminates_it();
    test_rejects_truncated_missing_and_nul_inputs();
    puts("test_input_file passed");
    return 0;
}
