/*
 * Minimal PWG Raster -> 1-bit CUPS Raster filter for Canon CAPT v1.
 *
 * Designed for Windows IPP Class Driver -> CUPS -> captppd on OpenWrt.
 * This is an experimental bridge, deliberately restricted to 600 DPI/A4.
 * It streams scanlines and never buffers a whole page.
 *
 * License: MIT.
 */
#include <cups/raster.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static void fail(const char *message) {
    fprintf(stderr, "ERROR: pwgtocaptraster: %s\n", message);
}

static int supported(const cups_page_header2_t *h) {
    if (h->HWResolution[0] != 600 || h->HWResolution[1] != 600) {
        fail("only 600x600 DPI is supported");
        return 0;
    }
    if (h->cupsWidth == 0 || h->cupsWidth > 15000 ||
        h->cupsHeight == 0 || h->cupsHeight > 20000 ||
        h->cupsBytesPerLine == 0 || h->cupsBytesPerLine > 131072 ||
        h->cupsColorOrder != CUPS_ORDER_CHUNKED) {
        fail("invalid dimensions or unsupported planar format");
        return 0;
    }
    if (h->cupsBitsPerPixel == 1 && h->cupsBitsPerColor == 1 &&
        (h->cupsColorSpace == CUPS_CSPACE_K || h->cupsColorSpace == CUPS_CSPACE_W)) {
        return h->cupsBytesPerLine >= (h->cupsWidth + 7) / 8;
    }
    if (h->cupsBitsPerPixel == 8 && h->cupsBitsPerColor == 8 &&
        (h->cupsColorSpace == CUPS_CSPACE_K || h->cupsColorSpace == CUPS_CSPACE_W ||
         h->cupsColorSpace == CUPS_CSPACE_SW)) {
        return h->cupsBytesPerLine >= h->cupsWidth;
    }
    if (h->cupsBitsPerPixel == 24 && h->cupsBitsPerColor == 8 &&
        (h->cupsColorSpace == CUPS_CSPACE_RGB || h->cupsColorSpace == CUPS_CSPACE_SRGB)) {
        return h->cupsBytesPerLine >= h->cupsWidth * 3;
    }
    fail("unsupported color space or bit depth");
    return 0;
}

static int luminance(const unsigned char *row, unsigned x, const cups_page_header2_t *h) {
    if (h->cupsBitsPerPixel == 1) {
        unsigned bit = (row[x / 8] >> (7 - (x % 8))) & 1;
        return h->cupsColorSpace == CUPS_CSPACE_K ? (bit ? 0 : 255) : (bit ? 255 : 0);
    }
    if (h->cupsBitsPerPixel == 8) {
        int val = row[x];
        return h->cupsColorSpace == CUPS_CSPACE_K ? 255 - val : val;
    }
    const unsigned char *rgb = row + x * 3;
    return (77 * rgb[0] + 150 * rgb[1] + 29 * rgb[2]) >> 8;
}

static int convert_page(cups_raster_t *input, cups_raster_t *output,
                        const cups_page_header2_t *source) {
    const unsigned w = source->cupsWidth;
    const unsigned h = source->cupsHeight;
    const unsigned bytes_out = (w + 7) / 8;
    unsigned char *row_in = calloc(source->cupsBytesPerLine, 1);
    unsigned char *row_out = calloc(bytes_out, 1);
    /* Errors in units of 1/16 of a gray level; +2 for edge padding. */
    int32_t *err_curr = calloc(w + 2, sizeof(int32_t));
    int32_t *err_next = calloc(w + 2, sizeof(int32_t));
    if (!row_in || !row_out || !err_curr || !err_next) {
        fail("out of memory allocating scanline buffers");
        free(row_in); free(row_out); free(err_curr); free(err_next);
        return 0;
    }

    cups_page_header2_t dest = *source;
    dest.cupsBitsPerColor = 1;
    dest.cupsBitsPerPixel = 1;
    dest.cupsNumColors = 1;
    dest.cupsColorSpace = CUPS_CSPACE_K;
    dest.cupsColorOrder = CUPS_ORDER_CHUNKED;
    dest.cupsBytesPerLine = bytes_out;
    dest.cupsCompression = 0x1f; /* Normal toner density for captppd. */
    dest.cupsMediaType = 0;
    dest.cupsInteger[0] = 4960; /* A4 physical size at 600 DPI. */
    dest.cupsInteger[1] = 7014;
    dest.cupsInteger[2] = 2;    /* A4 size ID in captppd's generated PPD. */
    dest.cupsInteger[5] = 1;    /* Automatic Image Refinement. */
    dest.cupsInteger[6] = 0;    /* Toner saving off. */
    dest.cupsPageSize[0] = 595;
    dest.cupsPageSize[1] = 842;
    dest.cupsImagingBBox[0] = 0;
    dest.cupsImagingBBox[1] = 0;
    dest.cupsImagingBBox[2] = 595;
    dest.cupsImagingBBox[3] = 842;
    snprintf(dest.cupsPageSizeName, sizeof(dest.cupsPageSizeName), "A4");

    if (!cupsRasterWriteHeader2(output, &dest)) {
        fail("failed to write output raster header");
        free(row_in); free(row_out); free(err_curr); free(err_next);
        return 0;
    }

    for (unsigned y = 0; y < h; ++y) {
        if (cupsRasterReadPixels(input, row_in, source->cupsBytesPerLine) != source->cupsBytesPerLine) {
            fail("truncated input raster");
            free(row_in); free(row_out); free(err_curr); free(err_next);
            return 0;
        }
        memset(row_out, 0, bytes_out);
        for (unsigned x = 0; x < w; ++x) {
            int32_t gray16 = luminance(row_in, x, source) * 16 + err_curr[x + 1];
            int black = gray16 < 128 * 16;
            if (black) row_out[x / 8] |= (unsigned char)(0x80 >> (x % 8));
            if (source->cupsBitsPerPixel != 1) {
                int32_t quant_error = gray16 - (black ? 0 : 255 * 16);
                if (x + 1 < w) err_curr[x + 2] += (quant_error * 7) / 16;
                if (x > 0) err_next[x] += (quant_error * 3) / 16;
                err_next[x + 1] += (quant_error * 5) / 16;
                if (x + 1 < w) err_next[x + 2] += quant_error / 16;
            }
        }
        if (cupsRasterWritePixels(output, row_out, bytes_out) != bytes_out) {
            fail("failed to write output raster pixels");
            free(row_in); free(row_out); free(err_curr); free(err_next);
            return 0;
        }
        int32_t *tmp = err_curr;
        err_curr = err_next;
        err_next = tmp;
        memset(err_next, 0, (w + 2) * sizeof(int32_t));
    }

    free(row_in); free(row_out); free(err_curr); free(err_next);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 6 && argc != 7) {
        fail("usage: filter job-id user title copies options [file]");
        return 1;
    }
    int fd = STDIN_FILENO;
    if (argc == 7) {
        fd = open(argv[6], O_RDONLY);
        if (fd < 0) { perror("open input"); return 1; }
    }

    cups_raster_t *input = cupsRasterOpen(fd, CUPS_RASTER_READ);
    cups_raster_t *output = cupsRasterOpen(STDOUT_FILENO, CUPS_RASTER_WRITE);
    if (!input || !output) {
        fail("could not open CUPS raster streams");
        if (input) cupsRasterClose(input);
        if (output) cupsRasterClose(output);
        if (fd != STDIN_FILENO) close(fd);
        return 1;
    }

    int result = 0, pages = 0;
    cups_page_header2_t header;
    while (cupsRasterReadHeader2(input, &header)) {
        if (!supported(&header)) { result = 1; break; }
        if (!convert_page(input, output, &header)) { result = 1; break; }
        ++pages;
    }
    if (!pages) { fail("no raster pages received"); result = 1; }
    cupsRasterClose(input);
    cupsRasterClose(output);
    if (fd != STDIN_FILENO) close(fd);
    return result;
}
