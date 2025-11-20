#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

// bool keydown; int keycode
void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t scancode = inl(KBD_ADDR);
  if(scancode != AM_KEY_NONE) {
    kbd->keydown = (scancode & KEYDOWN_MASK) ? true : false;
    kbd->keycode = scancode & (~KEYDOWN_MASK);
  } else {
    kbd->keydown = false;
    kbd->keycode = AM_KEY_NONE;
  }
}