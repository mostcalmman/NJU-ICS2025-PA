#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

// bool keydown; int keycode
void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t scancode = inl(KBD_ADDR);
  if (scancode | KEYDOWN_MASK) {
    kbd->keydown = 1;
    kbd->keycode = scancode & ~KEYDOWN_MASK;
    return;
  }
  kbd->keydown = 0;
  kbd->keycode = AM_KEY_NONE;
}
