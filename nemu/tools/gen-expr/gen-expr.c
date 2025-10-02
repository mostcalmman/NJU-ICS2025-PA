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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static uint32_t choose(uint32_t n) {
  return rand() % n;
}

static uint32_t gen_num() {
  return choose(256);
}

static void insert_space(int *length) {
  *length = strlen(buf);
  for(int i = 0; i < choose(10); i++){
    sprintf(buf + *length + i, " ");
  }
  *length = strlen(buf);
}

static void gen_rand_expr() {
  int length = strlen(buf);
  if(length > 65436) {
    sprintf(buf + length, "%u", gen_num());
    return;
  }
  switch(choose(3)){
    case 0:
      insert_space(&length);
      sprintf(buf + length, "%u", gen_num());
      insert_space(&length);
      break;
    case 1:
      insert_space(&length);
      sprintf(buf + length, "(");
      insert_space(&length);
      sprintf(buf + length, "%u", gen_num());
      insert_space(&length);
      sprintf(buf + length, ")");
      insert_space(&length);
      break;
    case 2:
    default:
      gen_rand_expr();
      insert_space(&length);
      switch(choose(4)){
        case 0: sprintf(buf + length, "+"); break;
        case 1: sprintf(buf + length, "-"); break;
        case 2: sprintf(buf + length, "*"); break;
        case 3: sprintf(buf + length, "/"); break;
        default: assert(0);
      }
      insert_space(&length);
      gen_rand_expr();
      break;
  }
  return;
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    memset(buf, '\0', sizeof(buf));
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    int status = pclose(fp);
    if ( status != 0 || !WIFEXITED(status) ){
      --i;
      continue;
    }

    printf("%u %s\n", result, buf);
  }
  return 0;
}
