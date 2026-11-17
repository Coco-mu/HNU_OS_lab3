/* Minimal, bounds-checked reader for the flattened device tree (FDT) format. */
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FDT_MAGIC 0xd00dfeedu
#define FDT_BEGIN_NODE 1u
#define FDT_END_NODE 2u
#define FDT_PROP 3u
#define FDT_NOP 4u
#define FDT_END 9u

static uint32_t be32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static int inside(size_t off, size_t amount, size_t limit)
{
    return off <= limit && amount <= limit - off;
}

static void print_hex(const unsigned char *p, uint32_t n)
{
    uint32_t i, shown = n < 32 ? n : 32;
    for (i = 0; i < shown; ++i) printf("%02x%s", p[i], i + 1 == shown ? "" : " ");
    if (shown < n) printf(" ... (%u bytes total)", n);
}

static int is_cell_array(const unsigned char *p, uint32_t n)
{
    uint32_t i;
    if (!n || n % 4 != 0) return 0;
    for (i = 0; i < n; ++i) {
        if (p[i] == 0 || !isprint(p[i])) return 1;
    }
    return 0;
}

static void print_cells(const unsigned char *p, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i += 4)
        printf("0x%08x%s", be32(p + i), i + 4 == n ? "" : " ");
}

static int is_string(const unsigned char *p, uint32_t n)
{
    uint32_t i;
    if (!n || p[n - 1] != 0) return 0;
    for (i = 0; i + 1 < n; ++i) if (!isprint(p[i])) return 0;
    return 1;
}

static int is_string_list(const unsigned char *p, uint32_t n)
{
    uint32_t i;
    if (!n || p[n - 1] != 0) return 0;
    for (i = 0; i + 1 < n; ++i)
        if (p[i] != 0 && !isprint(p[i])) return 0;
    return 1;
}

static void print_string_list(const unsigned char *p, uint32_t n)
{
    uint32_t start = 0, i;
    int first = 1;
    for (i = 0; i < n; ++i) {
        if (p[i] == 0) {
            if (!first) printf(", ");
            printf("\"%.*s\"", (int)(i - start), (const char *)p + start);
            first = 0;
            start = i + 1;
        }
    }
}

int main(int argc, char **argv)
{
    FILE *f;
    unsigned char *blob;
    long file_size;
    uint32_t total, struct_off, strings_off, struct_size, strings_size, token;
    size_t pos, struct_end;
    char *path = argc > 1 ? argv[1] : "virt.dtb";
    unsigned depth = 0;

    f = fopen(path, "rb");
    if (!f) { perror(path); return 1; }
    if (fseek(f, 0, SEEK_END) || (file_size = ftell(f)) < 40 || fseek(f, 0, SEEK_SET)) {
        fprintf(stderr, "Invalid or truncated FDT file\n"); fclose(f); return 1;
    }
    blob = malloc((size_t)file_size);
    if (!blob || fread(blob, 1, (size_t)file_size, f) != (size_t)file_size) {
        fprintf(stderr, "Could not read FDT file\n"); free(blob); fclose(f); return 1;
    }
    fclose(f);
    total = be32(blob + 4); struct_off = be32(blob + 8); strings_off = be32(blob + 12);
    strings_size = be32(blob + 32); struct_size = be32(blob + 36);
    if (be32(blob) != FDT_MAGIC || total > (uint32_t)file_size || total < 40 ||
        struct_off > total || strings_off > total || struct_size > total - struct_off ||
        strings_size > total - strings_off) {
        fprintf(stderr, "Bad FDT header or out-of-range blocks\n"); free(blob); return 1;
    }
    struct_end = (size_t)struct_off + struct_size;
    printf("FDT %s: %u bytes\n", path, total);
    pos = struct_off;
    while (inside(pos, 4, struct_end)) {
        token = be32(blob + pos); pos += 4;
        if (token == FDT_BEGIN_NODE) {
            size_t start = pos, len;
            while (inside(pos, 1, struct_end) && blob[pos] != 0) ++pos;
            if (!inside(pos, 1, struct_end)) break;
            len = pos - start;
            printf("%*s%s\n", depth * 2, "", len ? (const char *)blob + start : "/");
            ++pos; pos = (pos + 3u) & ~(size_t)3u; ++depth;
        } else if (token == FDT_END_NODE) {
            if (depth) --depth;
        } else if (token == FDT_PROP) {
            uint32_t len, nameoff;
            const unsigned char *value;
            const char *name;
            if (!inside(pos, 8, struct_end)) break;
            len = be32(blob + pos); nameoff = be32(blob + pos + 4); pos += 8;
            if (!inside(pos, len, struct_end) || nameoff >= strings_size) break;
            value = blob + pos; name = (const char *)(blob + strings_off + nameoff);
            if (!memchr(name, 0, strings_size - nameoff)) break;
            printf("%*s- %s = ", depth * 2, "", name);
            if (is_string_list(value, len))
                print_string_list(value, len);
            else if (is_string(value, len))
                printf("\"%s\"", (const char *)value);
            else if (is_cell_array(value, len)) print_cells(value, len);
            else print_hex(value, len);
            putchar('\n'); pos = (pos + len + 3u) & ~(size_t)3u;
        } else if (token == FDT_NOP) {
            continue;
        } else if (token == FDT_END) {
            free(blob); return 0;
        } else {
            fprintf(stderr, "Unknown FDT token %u at offset %zu\n", token, pos - 4);
            free(blob); return 1;
        }
    }
    fprintf(stderr, "Truncated FDT structure block\n");
    free(blob); return 1;
}
