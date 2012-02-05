#ifndef SIMSYS_W32_PNG_H
#define SIMSYS_W32_PNG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
// Try using GDIplus to save an screenshot.
bool dr_screenshot_png(char const* filename, int w, int h, int maxwidth, unsigned short* data, int bitdepth);
#endif

#ifdef __cplusplus
}
#endif

#endif
