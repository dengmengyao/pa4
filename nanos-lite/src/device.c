#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  int key_code;

  if ((key_code = _read_key()) == _KEY_NONE) 
    snprintf(buf, len, "t %d\n", _uptime());
  else if (key_code & 0x8000) {
    key_code ^= 0x8000;
    snprintf(buf, len, "kd %s\n", keyname[key_code]);
    extern void switch_game();
    if (key_code == _KEY_F12)
      switch_game();
  }
  else 
    snprintf(buf, len, "ku %s\n", keyname[key_code]);
  return strlen(buf);
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  strncpy(buf,dispinfo+offset,len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  int x,y;
  int len_1,len_2,len_3;
  offset=offset>>2;
  y=offset/_screen.width;
  x=offset%_screen.width;

  len=len>>2;//首行
  len_1=len_2=len_3=0;

  len_1=len<=_screen.width-x?len:_screen.width-x;//中间行
  _draw_rect((uint32_t *)buf, x, y, len_1, 1);

  if(len>len_1&&((len-len_1)>_screen.width)){
    len_2=len-len_1;
    _draw_rect((uint32_t *)buf + len_1, 0, y + 1, _screen.width, len_2 / _screen.width);
  }

  if(len-len_1-len_2){//尾行
    len_3=len-len_1-len_2;
    _draw_rect((uint32_t *)buf + len_1 + len_2, 0, y + len_2 / _screen.width + 1, len_3, 1);
  }
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  strcpy(dispinfo ,"WIDTH:400\nHEIGHT:300");
}
