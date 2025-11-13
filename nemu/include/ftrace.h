#include "common.h"

typedef struct {
    char name[32];
    vaddr_t addr;
} FunctionMap;

typedef struct {
    int func_number;
    vaddr_t func_addr[256];
} FuncStack;

bool parse_elf(const char *elf_file);
bool init_ftrace(const char *elf_file, const char *log_file);
const char* get_function_name(vaddr_t addr);
void print_function_map();
void ftrace_clean();
void fstackPush(vaddr_t addr, FuncStack *fstack);
vaddr_t fstackPop(FuncStack *fstack);
void ftrace_log_write(const char *buf);