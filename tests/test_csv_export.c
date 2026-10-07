#define _POSIX_C_SOURCE 200809L

#include "require_assert.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app/csv_export.h"

/* Fixtures live in a fresh directory beside this binary (see main). */
static char fixture_dir[512];

static const char *fixture_path(const char *name)
{
    static char path[640];
    snprintf(path, sizeof(path), "%s/%s", fixture_dir, name);
    return path;
}

static mode_t file_mode(const char *path)
{
    struct stat info;
    assert(lstat(path, &info) == 0);
    return info.st_mode & 0777;
}

static void write_text(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    assert(file && fputs(text, file) >= 0 && fclose(file) == 0);
}

static void test_create_new_is_exclusive_and_owner_only(void)
{
    mode_t previous = umask(0);
    const char *path = fixture_path("new.csv");
    FILE *stream = simulation_csv_create_new(path);
    assert(stream && fclose(stream) == 0);
    assert(file_mode(path) == 0600);
    /* Anything already at the name, even a dangling link, is refused. */
    errno = 0;
    assert(!simulation_csv_create_new(path) && errno == EEXIST);
    /* fixture_path() reuses one static buffer, so copy each result before
     * calling it again; otherwise the link would point at itself. */
    char target[640], link[640]; /* same size as fixture_path()'s buffer */
    snprintf(target, sizeof(target), "%s", fixture_path("missing-target.csv"));
    snprintf(link, sizeof(link), "%s", fixture_path("dangling.csv"));
    assert(strcmp(target, link) != 0);
    assert(symlink(target, link) == 0);
    assert(!simulation_csv_create_new(link) && errno == EEXIST);
    assert(access(target, F_OK) != 0);
    umask(previous);
}

static void test_numbered_snapshots_skip_existing_names_and_report_exhaustion(void)
{
    mode_t previous = umask(0);
    char stem[640], path[700];
    snprintf(stem, sizeof(stem), "%s", fixture_path("snap"));
    write_text(fixture_path("snap-001.csv"), "earlier snapshot");
    FILE *stream = simulation_csv_create_numbered(stem, 3, path, sizeof(path));
    assert(stream && fclose(stream) == 0);
    assert(strcmp(path, fixture_path("snap-002.csv")) == 0);
    assert(file_mode(path) == 0600);
    stream = simulation_csv_create_numbered(stem, 3, path, sizeof(path));
    assert(stream && fclose(stream) == 0);
    assert(strcmp(path, fixture_path("snap-003.csv")) == 0);
    /* Every allowed name is taken: fail with EEXIST, never overwrite. */
    errno = 0;
    assert(!simulation_csv_create_numbered(stem, 3, path, sizeof(path)) && errno == EEXIST);
    FILE *earlier = fopen(fixture_path("snap-001.csv"), "r");
    char text[32] = {0};
    assert(earlier && fgets(text, sizeof(text), earlier) && fclose(earlier) == 0);
    assert(strcmp(text, "earlier snapshot") == 0);
    /* A buffer too small for the name is refused rather than truncated. */
    char tiny[4];
    assert(!simulation_csv_create_numbered(stem, 3, tiny, sizeof(tiny)) && errno == ENAMETOOLONG);
    umask(previous);
}

static void test_output_replaces_writable_files_and_refuses_read_only_ones(void)
{
    const char *path = fixture_path("series.csv");
    write_text(path, "old contents");
    assert(chmod(path, 0640) == 0);
    CsvOutputFile output;
    assert(simulation_csv_output_open(&output, path));
    assert(fputs("new contents", output.stream) >= 0);
    assert(simulation_csv_output_commit(&output));
    assert(file_mode(path) == 0640);
    /* A read-only destination refuses writes, as the old O_TRUNC open did.
     * (Root may write anyway, so the check only applies to other users.) */
    if (geteuid() != 0) {
        assert(chmod(path, 0444) == 0);
        errno = 0;
        assert(!simulation_csv_output_open(&output, path) && errno == EACCES);
        assert(output.stream == NULL && output.temp_path == NULL);
        assert(chmod(path, 0644) == 0);
    }
    errno = 0;
    assert(!simulation_csv_output_open(&output, "") && errno == ENOENT);
}

int main(int argc, char **argv)
{
    /* A per-process directory beside the binary keeps reruns independent.
     * (mkdtemp would do, but macOS hides it under strict POSIX mode.) */
    const char *slash = argc > 0 ? strrchr(argv[0], '/') : NULL;
    snprintf(fixture_dir, sizeof(fixture_dir), "%.*s/csv-export-%ld",
        slash ? (int)(slash - argv[0]) : 1, slash ? argv[0] : ".", (long)getpid());
    assert(mkdir(fixture_dir, 0700) == 0);
    test_create_new_is_exclusive_and_owner_only();
    test_numbered_snapshots_skip_existing_names_and_report_exhaustion();
    test_output_replaces_writable_files_and_refuses_read_only_ones();
    puts("test_csv_export passed");
    return 0;
}
