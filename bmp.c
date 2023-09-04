#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef char BYTE;

#pragma pack(1)
struct bmp_header {
    uint16_t type;
    uint32_t file_size;
    uint32_t reserved;
    uint32_t offset;
    uint32_t header_size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t image_size;
    uint32_t x_ppm;
    uint32_t y_ppm;
    uint32_t num_colors;
    uint32_t important_colors;
};

void swap(BYTE *a, BYTE *b) {
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

void fix_bmp_order(BYTE *data, struct bmp_header *header) {
    for (int h = 0; h < header->height >> 1; h++) {
        for (int w = 0; w < header->width; w++) {
            swap(data + (h * header->width + w) * 3,
                data + ((header->height - 1 - h) * header->width + w) * 3 + 2);
            swap(data + (h * header->width + w) * 3 + 1,
                data + ((header->height - 1 - h) * header->width + w) * 3 + 1);
            swap(data + (h * header->width + w) * 3 + 2,
                data + ((header->height - 1 - h) * header->width + w) * 3);
        }
    }
}

void rgb888_to_argb8888(BYTE *rgb, BYTE* argb, struct bmp_header *header) {
    uint32_t pixel_cnt = header->width * header->height;

    for (int i = 0; i < pixel_cnt; i++) {
        *(argb + 4 * i + 1) = *(rgb + i * 3);
        *(argb + 4 * i + 2) = *(rgb + i * 3 + 1);
        *(argb + 4 * i + 3) = *(rgb + i * 3 + 2);
    }
}

int main(int argc, char *argv[]) {
    FILE *fptr = NULL;
    BYTE *buffer = NULL;
    BYTE *data = NULL;
    BYTE *argb = NULL;
    uint32_t size = 0;
    uint32_t pixel_cnt = 0;
    struct bmp_header *header = NULL;

    /* bmp file */
    if ((fptr = fopen(argv[1], "rb")) == NULL) {
        return -1;
    }

    fseek(fptr, 0, SEEK_END);
    size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    buffer = malloc(size * sizeof(BYTE));
    fread(buffer, size, sizeof(BYTE), fptr);
    fclose(fptr);

    header = (struct bmp_header *)buffer;
    if (header->type != 0x4d42) {
        goto exit;
    }
    printf("size=%u, width=%u, height=%u, offset=%u\n",
        header->file_size,
        header->width,
        header->height,
        header->offset);

    data = buffer + header->offset;
    pixel_cnt = header->width * header->height;

    fix_bmp_order(data, header);

    /* rgb888 */
    if ((fptr = fopen(argv[2], "wb")) == NULL) {
        return -1;
    }
    fwrite(data, size - header->offset, sizeof(BYTE), fptr);
    fclose(fptr);

    argb = malloc(pixel_cnt * sizeof(uint32_t));
    memset(argb, 0, pixel_cnt * sizeof(uint32_t));
    rgb888_to_argb8888(data, argb, header);

    /* rgb888 */
    if ((fptr = fopen(argv[3], "wb")) == NULL) {
        return -1;
    }
    fwrite(argb, pixel_cnt, sizeof(uint32_t), fptr);
    fclose(fptr);

exit:
    free(buffer);
    free(argb);

    return 0;
}