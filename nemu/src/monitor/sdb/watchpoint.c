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

#include "sdb.h"

static WP wp_pool[NR_WP] = {};
static WP *free_ = NULL;
WP *watchpoint_head = NULL; // 疑问

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  watchpoint_head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(){
  if(free_ == NULL){
    printf("No free watchpoint. Create failed.\n");
    return NULL;
  }

  WP *new_wp = free_;
  free_ = free_->next;

  new_wp->next = watchpoint_head;
  watchpoint_head = new_wp;

  return new_wp;
}

void free_wp(WP *wp){
  if(wp == NULL) return;

  if(watchpoint_head == wp){
    watchpoint_head = watchpoint_head->next;
  }
  else{
    WP *p = watchpoint_head;
    while(p != NULL && p->next != wp) p = p->next; // find the previous node
    if(p != NULL) p->next = wp->next;
  }

  // reset free list
  wp->next = free_;
  free_ = wp;
}
