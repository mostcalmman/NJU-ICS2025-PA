#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

static void sh_handle_cmd(const char *cmd) {
  if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ') {
    int i = 5;
    while(cmd[i] == ' ') i++;
    sh_printf("%s", cmd + i);
    return;
  }
  char buf[256];
  strncpy(buf, cmd, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  size_t len = strlen(buf);
  // 检查倒数第一个字符是否是换行符 '\n'
  if (len > 0 && buf[len - 1] == '\n') {
      buf[len - 1] = '\0';
  }

  char *argv[32];
  int argc = 0;
  char *token = strtok(buf, " ");
  while (token != NULL && argc < 31) {
    argv[argc++] = token;
    token = strtok(NULL, " ");
  }
  argv[argc] = NULL;

  if (argc > 0) {
    SDL_FillScreen(0x00000000);
    for (int i = 0; i < argc; i++) {
      printf("argv[%d] = %s\n", i, argv[i]);
    }
    // char* tempargv[] = {"pal", "--skip", NULL};
    execvp(argv[0], argv);
    // execvp(tempargv[0], tempargv);
  }
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

  setenv("PATH", "/bin", 0);

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}
