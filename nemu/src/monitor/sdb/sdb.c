/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <limits.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

word_t paddr_read(paddr_t addr, int len);

int strToUint64(char *str, uint64_t *dest){
  if(strchr(str, '-') != NULL){
    return -3;
  }

  char *endptr;
  *dest = strtoull(str, &endptr, 0);

  // check errors
  if(endptr == str || (*endptr != '\0' && *endptr != '\n')){
    return -1;
  }
  if(*dest == ULLONG_MAX){
    return -2;
  }
  
  return 0;
};

void display_all_watchpoints() {
  WP *p = watchpoint_head;
  if(p == NULL){
    printf("No watchpoint.\n");
    return;
  }
  printf("Num\tWhat\tLast value\n");
  while(p != NULL){
    printf("%d\t%s\t0x%x\n", p->NO, p->expr, p->last_value);
    p = p->next;
  }
}

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  // LSR: fix the problem mentioned in the doc, the second line seems ok, so leave it here
  nemu_state.state = NEMU_QUIT;
  // nemu_state.state = NEMU_END;
  return -1;
}

static int cmd_si(char *args){
  if(args == NULL){
    cpu_exec(1);
    return 0;
  }

  uint64_t n;
  int ret = strToUint64(args, &n);

  if(ret == -1){
    printf("Invalid input: Not a number\n");
    return 0;
  }
  if(ret == -2){
    printf("Invalid input: Number too large\n");
    return 0;
  }
  if(ret == -3 || n == 0){
    printf("Invalid input: N should be a positive number\n");
    return 0;
  }

  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args){
  if(args == NULL){
    printf("Invalid input: Missing argument\n");
    return 0;
  }

  if(strcmp(args, "r") == 0){
    isa_reg_display();
  }
  else if(strcmp(args, "w") == 0){
    display_all_watchpoints();
  }
  else{
    printf("Invalid input: Unknown argument '%s'\n", args);
  }

  return 0;
}

static int cmd_x(char *args){
  if(args == NULL){
    printf("Invalid input: Missing argument\n");
    return 0;
  }
  char *end;
  unsigned long len = strtoul(args, &end, 10);
  if (end == args || *end != ' ') {
    printf("Invalid input: Missing argument or len isn't a number\n");
    return 0;
  }
  if(len == 0){
    printf("Invalid input: len should be a positive number\n");
    return 0;
  }
  // char *end1;
  // paddr_t addr = strtoul(end + 1, &end1, 16);
  // if (end1 == end + 1 || (*end1 != '\0' && *end1 != '\n')) {
  //   printf("Invalid input: Addr should be a number\n");
  //   return 0;
  // }
  bool success = true;
  paddr_t addr = expr(end + 1, &success);
  if(!success){
    printf("The expression is illegal.\n");
    return 0;
  }

  for(unsigned long i = 0; i < len; i++){
    if(addr + i * 4 < CONFIG_MBASE || addr + i * 4 > 0x87ffffff ){
      printf("Invalid address 0x%lx\n", addr + i * 4);
      return 0;
    }
    word_t data = paddr_read(addr + i * 4, 4);
    printf("0x%lx\t0x%x\n", addr + i * 4, data);
  }
  return 0;
}

static int cmd_p(char *args){
  bool success = true;
  word_t result = expr(args, &success);
  if(success){
    printf("Result is %u\n", result);
  }
  else{
    printf("The expression is illegal.\n");
  }
  return 0;
}

static int cmd_w(char *args){
  if(args == NULL){
    printf("Invalid input: Missing argument\n");
    return 0;
  }
  // check expression legality
  bool success = true;
  word_t val = expr(args, &success);
  if(!success){
    printf("The expression is illegal.\n");
    return 0;
  }

  WP *wp = new_wp();
  if(wp == NULL) return 0; // 错误信息会在new_wp中打印
  strncpy(wp->expr, args, 1023);
  wp->expr[1023] = '\0'; // 确保字符串以'\0'结尾
  wp->last_value = val;
  printf("New watchpoint %d: %s, original value = 0x%x\n", wp->NO, wp->expr, wp->last_value);
  return 0;
}

static int cmd_d(char *args){
  if(args == NULL){
    printf("Invalid input: Missing argument\n");
    return 0;
  }

  uint64_t n;
  int ret = strToUint64(args, &n);

  if(ret == -1){
    printf("Invalid input: Not a number\n");
    return 0;
  }
  if(ret == -2){
    printf("Invalid input: Number too large\n");
    return 0;
  }
  if(n >= NR_WP){
    printf("Invalid input: N should be less than %d\n", NR_WP);
    return 0;
  }

  WP *p = watchpoint_head;
  while(p != NULL && p->NO != n) p = p->next;
  if(p == NULL){
    printf("No watchpoint %lu\n", n);
    return 0;
  }
  free_wp(p);
  printf("Watchpoint %lu deleted\n", n);
  return 0;
}

static int cmd_test(char *args) {
  if(*args != '\0'){
    printf("%s\n", args);
  }
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "si N: Step the program for N instructions and then pause. (default N=1)", cmd_si },
  { "info", "Print the register status or the information of watchpoints", cmd_info },
  { "x", "Scan the memory", cmd_x },
  { "p", "Evaluate the expression EXPR and print its value", cmd_p },
  { "w", "Set a watchpoint for an expression EXPR", cmd_w },
  { "d", "Delete the watchpoint with the given number", cmd_d },
  { "test", "A test command for demonstration", cmd_test },
  { "p", "Get the value of the expression EXPR", cmd_p },

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }
  

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
