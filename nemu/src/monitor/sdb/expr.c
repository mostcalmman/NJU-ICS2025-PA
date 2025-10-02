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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

word_t paddr_read(paddr_t addr, int len);

enum {
  TK_NOTYPE = 256,
  TK_PLUS = '+',
  TK_MINUS = '-',
  TK_MULTIPLY = '*',
  TK_DIVIDE = '/',
  TK_NUMBER = 'd',
  TK_LPAREN = '(',
  TK_RPAREN = ')',
  TK_EQUAL = '=',   // ==
  TK_UEQUAL = '!',  // !=
  TK_AND = '&',     // &&
  // TK_OR = '|',      // ||
  TK_REG = '$',    // register
  TK_HEX = 'h',    // 0x
  TK_NEGATIVE = 'n', // -<expr>
  TK_DEREF = 'p',  // *<expr>

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},              // spaces
  {"\\+", TK_PLUS},               // plus
  {"-", TK_MINUS},                // minus
  {"\\*", TK_MULTIPLY},           // multiply
  {"/", TK_DIVIDE},               // divide
  {"==", TK_EQUAL},               // equal
  {"!=", TK_UEQUAL},              // unequal
  {"&&", TK_AND},                 // and
  {"\\$[0-9a-zA-Z]{1,3}", TK_REG},   // register
  {"0[xX][0-9a-fA-F]+", TK_HEX},  // hexadecimal number
  {"[0-9]+", TK_NUMBER},         // decimal number
  {"\\(", TK_LPAREN},            // (
  {"\\)", TK_RPAREN},            // )

};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

// MARK: 暂时调大tokens
static Token tokens[65536] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        assert(nr_token < 65536);

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;

          case TK_PLUS:
          case TK_MINUS:
          case TK_MULTIPLY:
          case TK_DIVIDE:
          case TK_LPAREN:
          case TK_RPAREN:
          case TK_EQUAL:
          case TK_UEQUAL:
          case TK_AND:
          // case TK_OR:
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;

          case TK_NUMBER:
          case TK_REG:
          case TK_HEX:
            if(rules[i].token_type == TK_HEX && substr_len > 10){
              printf("The hexadecimal number is too long!\n");
              return false;
            }
            assert(substr_len < 32);
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            ++nr_token;
            break;

          default:
            printf("regex: something went wrong\n");
            assert(0);
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentness(int p,int q){
  if(tokens[p].type != TK_LPAREN || tokens[q].type != TK_RPAREN){
    return false;
  }
  int waiting = 0;
  for(int i = p + 1; i < q; i++){
    if(tokens[i].type == TK_LPAREN) ++waiting;
    if(tokens[i].type == TK_RPAREN) --waiting;
    if(waiting < 0) return false;
  }
  return waiting == 0;
}

word_t val(int p, int q, bool *success){
  if(p > q){
    *success = false;
    return 0;
  }

  if( p==q ){
    if(tokens[p].type == TK_NUMBER) return strtoull(tokens[p].str, NULL, 10);
    if(tokens[p].type == TK_HEX) return strtoull(tokens[p].str, NULL, 16);
    if(tokens[p].type == TK_REG){
      word_t ret = isa_reg_str2val(tokens[p].str, success);
      if(!*success){
        printf("Unknown register '%s'\n", tokens[p].str);
        return 0;
      }
      return ret;
    }
  }

  // // 不够完善的负号处理方案
  // if(tokens[p].type == TK_MINUS) return -val(p + 1, q, success);

  // nagative
  if(tokens[p].type == TK_NEGATIVE){
    if(q == p + 1){
      if( tokens[p+1].type != TK_NUMBER && tokens[p+1].type != TK_HEX && tokens[p+1].type != TK_REG ){
        *success = false;
        return 0;
      }
      return -val(p + 1, q, success);
    }

    if(tokens[p+1].type == TK_LPAREN && check_parentness(p + 1, q)){
      return -val(p + 2, q - 1, success);
    }

    if(q == p + 2 && tokens[p+1].type == TK_DEREF) return -val(p + 1, q, success);    
  }

  // dereference
  if(tokens[p].type == TK_DEREF){
    if(q == p+1){
      // MARK: 暂不允许十进制地址
      // if( tokens[p+1].type != TK_NUMBER && tokens[p+1].type != TK_HEX && tokens[p+1].type != TK_REG ){
      if( tokens[p+1].type != TK_HEX && tokens[p+1].type != TK_REG ){
        *success = false;
        return 0;
      }
      word_t addr = val(p + 1, q, success);
      if( addr < 0x80000000 || addr > 0x87ffffff ){
        printf("Invalid address 0x%x\n", addr);
        *success = false;
        return 0;
      }
      return paddr_read(addr, 4);
    }
    if(tokens[p+1].type == TK_LPAREN && check_parentness(p + 1, q)){
      word_t addr = val(p + 2, q - 1, success);
      if( addr < 0x80000000 || addr > 0x87ffffff ){
        printf("Invalid address 0x%x\n", addr);
        *success = false;
        return 0;
      }
      return paddr_read(addr, 4);
    }
  }

  if (check_parentness(p, q)) {
    return val(p + 1, q - 1, success);
  }

  // find the main operator
  typedef struct {
    int position;
    int type;
  } Op;
  Op op = { -1, -1 };
  int waiting = 0;
  for(int i = p; i <= q; i++ ){
    if(tokens[i].type == TK_LPAREN) ++waiting;
    if(tokens[i].type == TK_RPAREN) --waiting;
    if(waiting < 0){
      *success = false;
      return 0;
    }
    if(waiting != 0) continue;

    if(tokens[i].type == TK_PLUS || tokens[i].type == TK_MINUS){
      op.position = i;
      op.type = tokens[i].type;

      // // 不够完善的负号处理方案
      // if(tokens[i+1].type == TK_MINUS){
      //   if(tokens[i+2].type == TK_MINUS || tokens[i+2].type == TK_PLUS){
      //     *success = false;
      //     return 0;
      //   }
      //   ++i; // skip negative sign
      // }
    }
    if( (tokens[i].type == TK_MULTIPLY || tokens[i].type == TK_DIVIDE) && op.type != TK_PLUS && op.type != TK_MINUS){
      op.position = i;
      op.type = tokens[i].type;
    }
  }

  if(waiting != 0 || op.position == -1){
    *success = false;
    return 0;
  }

  // calculate
  word_t val1 = val(p, op.position - 1, success);
  word_t val2 = val(op.position + 1, q, success);
  switch(op.type){
    case TK_PLUS: return val1 + val2;
    case TK_MINUS: return val1 - val2;
    case TK_MULTIPLY: return val1 * val2;
    case TK_DIVIDE: 
      if(val2 == 0){
        *success = false;
        return 0;
      }
      return val1 / val2;
    default: assert(0);
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // address dereference and negative number
  for(int i = 0; i < nr_token; i++){
    if(
       tokens[i].type == TK_MINUS &&
       (
        i == 0 || tokens[i - 1].type == TK_PLUS || tokens[i - 1].type == TK_MINUS || 
        tokens[i - 1].type == TK_MULTIPLY || tokens[i - 1].type == TK_DIVIDE || tokens[i - 1].type == TK_LPAREN || 
        tokens[i - 1].type == TK_EQUAL || tokens[i - 1].type == TK_UEQUAL || tokens[i - 1].type == TK_AND
       )
      )
    {
      tokens[i].type = TK_NEGATIVE;
    }

    if(
       tokens[i].type == TK_MULTIPLY &&
       (
        i == 0 || tokens[i - 1].type == TK_PLUS || tokens[i - 1].type == TK_MINUS || tokens[i - 1].type == TK_NEGATIVE || // 解引用前可以有负号
        tokens[i - 1].type == TK_MULTIPLY || tokens[i - 1].type == TK_DIVIDE || tokens[i - 1].type == TK_LPAREN || 
        tokens[i - 1].type == TK_EQUAL || tokens[i - 1].type == TK_UEQUAL || tokens[i - 1].type == TK_AND
       )
      )
    {
      tokens[i].type = TK_DEREF;
    }
  }

  return val(0, nr_token - 1, success);
}
