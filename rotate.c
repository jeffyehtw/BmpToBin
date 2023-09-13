#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <stdbool.h>

typedef uint8_t BYTE;

struct argument {
    char *src;
    char *dst;
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t format;
    uint16_t rotate;
};

bool valid(struct argument args) {
    return ((args.rotate == 0) || (args.rotate == 90)
            || (args.rotate == 180) || (args.rotate == 270));
}

void rotate(uint32_t width, uint32_t height, BYTE *src, BYTE *dst, uint8_t step) {
    uint16_t w = 0;
    uint16_t h = 0;
    uint16_t s = 0;

    for (h = 0; h < height; h++) {
        for (w = 0; w < width; w++) {
            for (s = 0; s < step; s++) {
                uint32_t src_offset = (w * height + h) * step + s;
                uint32_t dst_offset = (h * width + w) * step + s;

                *(dst + src_offset) = *(src + dst_offset);
            }
        }
    }
}

void swap(BYTE *x, BYTE *y) {
    *x ^= *y;
    *y ^= *x;
    *x ^= *y;
}

void mirror_h(uint32_t width, uint32_t height, BYTE* dst, uint8_t step) {
    uint16_t w = 0;
    uint16_t h = 0;
    uint16_t s = 0;

    for (h = 0; h < height; h++) {
        for (w = 0; w < (width >> 1); w++) {
            for (s = 0; s < step; s++) {
                swap(dst + (h * width + w) * step + s,
                    dst + ((h + 1) * width - 1 - w) * step + s);
            }
        }
    }
}

void mirror_v(uint32_t width, uint32_t height, BYTE* dst, uint8_t step) {
    uint16_t w = 0;
    uint16_t h = 0;
    uint16_t s = 0;

    for (h = 0; h < (height >> 1); h++) {
        for (w = 0; w < width; w++) {
            for (s = 0; s < step; s++) {
                swap(dst + (h * width + w) * step + s,
                    dst + ((height - 1 - h) * width + w) * step + s);
            }
        }
    }
}

void rgb888(struct argument *args, BYTE *src, BYTE *dst) {
    if (args->rotate == 90) {
        rotate(args->width, args->height, src, dst, 3);
        mirror_h(args->height, args->width, dst, 3);
    } else if (args->rotate == 180) {
        memcpy(dst, src, args->size * sizeof(BYTE));
        mirror_h(args->width, args->height, dst, 3);
        mirror_v(args->width, args->height, dst, 3);
    } else if (args->rotate == 270) {
        rotate(args->width, args->height, src, dst, 3);
        mirror_v(args->height, args->width, dst, 3);
    }
}

int main(int argc, char *argv[]) {
    FILE *fptr = NULL;
    BYTE *src = NULL;
    BYTE *dst = NULL;
    struct argument args;
    char ch;

    memset(&args, 0, sizeof(struct argument));
    while ((ch = getopt(argc, argv, "w:h:f:r:")) != EOF) {
        switch (ch) {
        case 'w':
            args.width = atoi(optarg);
            break;
        case 'h':
            args.height = atoi(optarg);
            break;
        case 'f':
            args.format = atoi(optarg);
            break;
        case 'r':
            args.rotate = atoi(optarg);
            break;
        default:
            printf("Unknown option: '%s'\n", optarg);
            return -1;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 2) {
        return -1;
    }
    args.src = argv[0];
    args.dst = argv[1];

    printf("format:\t%u\n", args.format);
    printf("src:\t%s\n", args.src);
    printf("dst:\t%s\n", args.dst);
    printf("width:\t%u\n", args.width);
    printf("height:\t%u\n", args.height);

    if ((fptr = fopen(args.src, "rb")) == NULL) {
        return -1;
    }

    fseek(fptr, 0, SEEK_END);
    args.size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    src = malloc(args.size * sizeof(BYTE));
    fread(src, args.size, sizeof(BYTE), fptr);
    fclose(fptr);

    dst = malloc(args.size * sizeof(BYTE));
    memset(dst, 0, args.size * sizeof(BYTE));

    switch (args.format) {
        /* rgb888 */
        case 1:
            rgb888(&args, src, dst);
            break;
        default:
            goto exit;
    }

    if ((fptr = fopen(args.dst, "wb")) == NULL) {
        goto exit;
    }
    fwrite(dst, args.size, sizeof(BYTE), fptr);
    fclose(fptr);

exit:
    free(src);
    free(dst);

    return 0;
}