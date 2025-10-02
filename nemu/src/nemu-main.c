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

#include <common.h>
#include <stdio.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
word_t expr(char *e, bool *success);

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  // Test expr
  char line[65536 + 128];
  FILE *file;
  file = fopen("/home/liushengrui/ics2025/nemu/tools/gen-expr/build/input", "r");
  int i = 1;
  while(fgets(line, sizeof(line), file)) {
    char *newline = strchr(line, '\n');
    if (newline) {
      *newline = '\0';
    }
    bool success = true;
    char *start = strchr(line, ' ');
    word_t expected = strtoull(line, NULL, 10);
    word_t result = expr(start + 1, &success);
    if(!success) {
      printf("The expression %d is illegal.\n", i);
    }
    if(result != expected) {
      printf("Test %d failed: expected %u, got %u\n", i, expected, result);
    }
    i++;
  }
  fclose(file);

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
