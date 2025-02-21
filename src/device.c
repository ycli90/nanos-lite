#include <common.h>
#include <fs.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  for (int i = 0; i < len; i++) {
    putch(*(char*)(buf + i));
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode == AM_KEY_NONE) return 0;
  return snprintf(buf, len, "%s %s\n", ev.keydown ? "kd" : "ku", keyname[ev.keycode]);
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  return snprintf(buf, len, "WIDTH:%d\nHEIGHT:%d", screen_width, screen_height);
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  assert(offset % 4 == 0 && len % 4 == 0);
  int pixel_offset = offset / 4;
  int pixel_len = len / 4;
  if (pixel_len < screen_width) {
    int x = pixel_offset % screen_width;
    assert(x + pixel_len <= screen_width);
    int y = pixel_offset / screen_width;
    io_write(AM_GPU_FBDRAW, x, y, (void*)buf, pixel_len, 1, true);
  } else {
    assert(pixel_offset % screen_width == 0);
    assert(pixel_len % screen_width == 0);
    int y = pixel_offset / screen_width;
    int h = pixel_len / screen_width;
    io_write(AM_GPU_FBDRAW, 0, y, (void*)buf, screen_width, h, true);
  }
  return len;
}

size_t sbcfg_read(void *buf, size_t offset, size_t len) {
  int bufsize = io_read(AM_AUDIO_CONFIG).bufsize;
  return snprintf((char *)buf, len, "%d %d", 1, bufsize);
}

size_t sbctl_read(void *buf, size_t offset, size_t len) {
  int bufsize = io_read(AM_AUDIO_CONFIG).bufsize;
  int count = io_read(AM_AUDIO_STATUS).count;
  return snprintf((char *)buf, len, "%d", bufsize - count);
}

size_t sbctl_write(const void *buf, size_t offset, size_t len) {
  assert(len >= 3 * sizeof(int));
  const int *args = (const int *)buf;
  int freq = args[0];
  int channels = args[1];
  int samples = args[2];
  io_write(AM_AUDIO_CTRL, freq, channels, samples);
  return len;
}

size_t sb_write(const void *buf, size_t offset, size_t len) {
  Area area = {(void*)buf, (void*)buf + len};
  io_write(AM_AUDIO_PLAY, area);
  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
