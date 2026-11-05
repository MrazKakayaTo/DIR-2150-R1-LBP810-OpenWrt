/* Build on an x86_64 machine with CUPS headers and libcups to test the bridge.
 * Usage: raster_smoke gen test.pwg | raster_smoke inspect test.cups
 */
#include <cups/raster.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    int generate = strcmp(argv[1], "gen") == 0;
    int inspect = strcmp(argv[1], "inspect") == 0;
    if (!generate && !inspect) return 2;
    int fd = open(argv[2], generate ? (O_CREAT | O_TRUNC | O_WRONLY) : O_RDONLY, 0600);
    if (fd < 0) { perror("open"); return 1; }
    cups_raster_t *r = cupsRasterOpen(fd, generate ? CUPS_RASTER_WRITE_PWG : CUPS_RASTER_READ);
    if (!r) return 1;
    cups_page_header2_t h;
    memset(&h, 0, sizeof(h));
    if (generate) {
        h.HWResolution[0] = h.HWResolution[1] = 600;
        h.cupsWidth = h.cupsHeight = 16;
        h.cupsBitsPerColor = h.cupsBitsPerPixel = 8;
        h.cupsColorSpace = CUPS_CSPACE_SW;
        h.cupsColorOrder = CUPS_ORDER_CHUNKED;
        h.cupsBytesPerLine = 16;
        h.cupsNumColors = 1;
        h.cupsPageSize[0] = 595;
        h.cupsPageSize[1] = 842;
        h.cupsInteger[0] = 4960;
        h.cupsInteger[1] = 7014;
        snprintf(h.cupsPageSizeName, sizeof(h.cupsPageSizeName), "A4");
        if (!cupsRasterWriteHeader2(r, &h)) return 1;
        for (int y = 0; y < 16; y++) {
            unsigned char line[16];
            for (int x = 0; x < 16; x++) line[x] = x < 8 ? 0 : 255;
            if (cupsRasterWritePixels(r, line, 16) != 16) return 1;
        }
    } else {
        if (!cupsRasterReadHeader2(r, &h)) return 1;
        printf("%ux%u %ubpp %u bytes/line %u dpi, page size %u, %u, %u\n",
               h.cupsWidth, h.cupsHeight, h.cupsBitsPerPixel, h.cupsBytesPerLine,
               h.HWResolution[0], h.cupsInteger[0], h.cupsInteger[1], h.cupsInteger[2]);
        if (h.cupsWidth != 16 || h.cupsHeight != 16 || h.cupsBitsPerPixel != 1 ||
            h.cupsBytesPerLine != 2 || h.cupsInteger[2] != 2) return 1;
        for (int y = 0; y < 16; y++) {
            unsigned char line[2] = {0, 0};
            if (cupsRasterReadPixels(r, line, 2) != 2) return 1;
            if (line[0] != 255 || line[1] != 0) {
                fprintf(stderr, "row %d: %02x %02x (expected ff 00)\n", y, line[0], line[1]);
                return 1;
            }
        }
        if (cupsRasterReadHeader2(r, &h)) return 1;
        puts("PASS: PWG Raster was converted to CAPT-ready 1-bit CUPS Raster");
    }
    cupsRasterClose(r);
    close(fd);
    return 0;
}
