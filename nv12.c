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
    uint32_t width;
    uint32_t height;
    bool separated;
};

float rgb2y(float r, float g, float b) {
    return 0.257 * r + 0.504 * g + 0.098 * b + 16;
}

float rgb2u(float r, float g, float b) {
    return -0.148 * r - 0.291 * g + 0.439 * b + 128;
}

float rgb2v(float r, float g, float b) {
    return 0.439 * r - 0.368 * g - 0.071 * b + 128;
}

int main(int argc, char *argv[]) {
    FILE *fptr = NULL;
    BYTE *buffer = NULL;
    BYTE *nv12 = NULL;
    BYTE *luma = NULL;
    BYTE *chroma = NULL;
    uint32_t size = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    struct argument args;
    char ch;

    memset(&args, 0, sizeof(struct argument));
    while ((ch = getopt(argc, argv, "w:h:s")) != EOF) {
        switch (ch) {
            case 'w':
                args.width = atoi(optarg);
                break;
            case 'h':
                args.height = atoi(optarg);
                break;
            case 's':
                args.separated = 1;
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

    printf("src:\t%s\n", args.src);
    printf("dst:\t%s\n", args.dst);
    printf("width:\t%u\n", args.width);
    printf("height:\t%u\n", args.height);

    if ((fptr = fopen(args.src, "rb")) == NULL) {
        return -1;
    }

    fseek(fptr, 0, SEEK_END);
    size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    buffer = malloc(size * sizeof(BYTE));
    fread(buffer, size, sizeof(BYTE), fptr);
    fclose(fptr);

    size >>= 1;
    nv12 = malloc(size * sizeof(BYTE));
    luma = nv12;
    chroma = nv12 + args.width * args.height;
    memset(nv12, 0, size * sizeof(BYTE));

#if 0
    printf("size:\t%u\n", size);
    printf("nv12:\t%p\n", nv12);
    printf("luma:\t%p\n", luma);
    printf("chroma:\t%p\n", chroma);
#endif

    for (h = 0; h < args.height; h += 2) {
        for (w = 0; w < args.width; w += 2) {
            float r00 = (float)*(buffer + (h * args.width + w) * 3 + 0);
            float g00 = (float)*(buffer + (h * args.width + w) * 3 + 1);
            float b00 = (float)*(buffer + (h * args.width + w) * 3 + 2);

            float r01 = (float)*(buffer + (h * args.width + w) * 3 + 3);
            float g01 = (float)*(buffer + (h * args.width + w) * 3 + 4);
            float b01 = (float)*(buffer + (h * args.width + w) * 3 + 5);

            float r10 = (float)*(buffer + ((h + 1) * args.width + w) * 3 + 0);
            float g10 = (float)*(buffer + ((h + 1) * args.width + w) * 3 + 1);
            float b10 = (float)*(buffer + ((h + 1) * args.width + w) * 3 + 2);

            float r11 = (float)*(buffer + ((h + 1) * args.width + w) * 3 + 3);
            float g11 = (float)*(buffer + ((h + 1) * args.width + w) * 3 + 4);
            float b11 = (float)*(buffer + ((h + 1) * args.width + w) * 3 + 5);

            float y00 = rgb2y(r00, g00, b00) + 0.5;
            float y01 = rgb2y(r01, g01, b01) + 0.5;
            float y10 = rgb2y(r10, g10, b10) + 0.5;
            float y11 = rgb2y(r11, g11, b11) + 0.5;

            float u00 = rgb2u(r00, g00, b00);
            float u01 = rgb2u(r01, g01, b01);
            float u10 = rgb2u(r10, g10, b10);
            float u11 = rgb2u(r11, g11, b11);

            float v00 = rgb2v(r00, g00, b00);
            float v01 = rgb2v(r01, g01, b01);
            float v10 = rgb2v(r10, g10, b10);
            float v11 = rgb2v(r11, g11, b11);

            float u0 = (u00 + u01 + u10 + u11) * 0.25 + 0.5;
            float v0 = (v00 + v01 + v10 + v11) * 0.25 + 0.5;

            *(luma + (h * args.width + w)) = y00;
            *(luma + (h * args.width + w) + 1) = y01;
            *(luma + ((h + 1) * args.width + w)) = y10;
            *(luma + ((h + 1) * args.width + w) + 1) = y11;

            *(chroma + (h * (args.width >> 1) + w)) = u0;
            *(chroma + (h * (args.width >> 1) + w) + 1) = v0;
        }
    }

    if (args.separated) {
        char *luma_ext = ".y";
        char *chroma_ext = ".c";
        uint32_t len = strlen(args.dst) + 2 + 1;
        char *name = malloc(len * sizeof(char));

        memset(name, 0, len * sizeof(char));
        strcpy(name, args.dst);

        strcpy(name + strlen(args.dst), luma_ext);
        if ((fptr = fopen(name, "wb")) == NULL) {
            return -1;
        }
        fwrite(luma, args.width * args.height, sizeof(BYTE), fptr);
        fclose(fptr);

        strcpy(name + strlen(args.dst), chroma_ext);
        if ((fptr = fopen(name, "wb")) == NULL) {
            return -1;
        }
        fwrite(chroma, (args.width * args.height) >> 1, sizeof(BYTE), fptr);
        fclose(fptr);
    } else {
        if ((fptr = fopen(args.dst, "wb")) == NULL) {
            return -1;
        }
        fwrite(nv12, size, sizeof(BYTE), fptr);
        fclose(fptr);
    }

exit:
    free(buffer);
    free(nv12);

    return 0;
}