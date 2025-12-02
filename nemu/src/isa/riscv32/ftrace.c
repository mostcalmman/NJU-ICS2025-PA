#include "common.h"
#include "cpu/decode.h"
#include "debug.h"
#include <locale.h>
#include <stdio.h>
#include <elf.h>
#include <string.h>
// #include "ftrace.h"
#ifdef CONFIG_FTRACE
typedef struct {
    char name[32];
    vaddr_t addr;
    uint32_t size;
} FunctionMap;
static int func_count;
static FunctionMap *function_map;
static FILE *ftrace_log;
static int g_ftrace_tab_num = 0;
static const char *ignore_funcs[] = {"putch", NULL};


// false表示解析失败, true表示成功
static bool parse_elf(const char *elf_file) {
    FILE *fp = fopen(elf_file, "rb"); // 以二进制只读方式打开
    if (!fp) {
        Log("Failed to open ELF file");
        return false;
    }

    // 检查ELF头合法性, 由于针对riscv32设计, 后面的元信息不需要处理
    Elf32_Ehdr ehdr;
    if(fread(&ehdr, sizeof(Elf32_Ehdr), 1, fp) != 1) {
        Log("Failed to read ELF header");
        fclose(fp);
        return false;
    }
    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        Log("Invalid ELF file");
        fclose(fp);
        return false;
    }

    // 读取Section Header Table
    fseek(fp, ehdr.e_shoff, SEEK_SET);
    int shdr_table_size = ehdr.e_shnum * ehdr.e_shentsize; // 暂不考虑 e_shnum >= SHN_LORESERVE 的情况
    Elf32_Shdr *shdr_table = malloc(shdr_table_size);
    if(fread(shdr_table, 1, shdr_table_size, fp) != shdr_table_size) {
        Log("Failed to read Section Header Table");
        free(shdr_table);
        fclose(fp);
        return false;
    }
    Elf32_Shdr *shstrtab_shdr = &shdr_table[ehdr.e_shstrndx]; // 在Section Header Table中定位到.shstrtab节
    char *shstrtab_data = malloc(shstrtab_shdr->sh_size);
    fseek(fp, shstrtab_shdr->sh_offset, SEEK_SET); // 偏置到.shstrtab节的数据位置
    if(fread(shstrtab_data, 1, shstrtab_shdr->sh_size, fp) != shstrtab_shdr->sh_size) {
        Log("Failed to read .shstrtab section");
        free(shstrtab_data);
        free(shdr_table);
        fclose(fp);
        return false;
    }

    // 遍历Section Header Table寻找.symtab和.strtab节
    Elf32_Shdr *symtab_shdr = NULL;
    Elf32_Shdr *strtab_shdr = NULL;
    for(int i = 0; i < ehdr.e_shnum; i++) {
        const char *section_name = shstrtab_data + shdr_table[i].sh_name; // 利用.shstrtab节获取节名
        if(strcmp(section_name, ".symtab") == 0) {
            symtab_shdr = &shdr_table[i];
        } else if(strcmp(section_name, ".strtab") == 0) {
            strtab_shdr = &shdr_table[i];
        }
    }
    if(!symtab_shdr || !strtab_shdr){
        Log("No .symtab or .strtab found in ELF file");
        free(shdr_table);
        free(shstrtab_data);
        fclose(fp);
        return false;
    }

    // 读取.strtab和.symtab
    char *strtab_data = malloc(strtab_shdr->sh_size);
    fseek(fp, strtab_shdr->sh_offset, SEEK_SET);
    if(fread(strtab_data, 1, strtab_shdr->sh_size, fp) != strtab_shdr->sh_size) {
        Log("Failed to read .strtab section");
        free(strtab_data);
        free(shdr_table);
        free(shstrtab_data);
        fclose(fp);
        return false;
    }
    Elf32_Sym *symtab_data = malloc(symtab_shdr->sh_size);
    fseek(fp, symtab_shdr->sh_offset, SEEK_SET);
    if(fread(symtab_data, 1, symtab_shdr->sh_size, fp) != symtab_shdr->sh_size) {
        Log("Failed to read .symtab section");
        free(symtab_data);
        free(strtab_data);
        free(shdr_table);
        free(shstrtab_data);
        fclose(fp);
        return false;
    }

    // 遍历符号表, 提取函数符号和大小
    int num_symbols = symtab_shdr->sh_size / sizeof(Elf32_Sym);
    FunctionMap *func_map = malloc(num_symbols * sizeof(FunctionMap));
    func_count = 0;
    for(int i = 0; i < num_symbols; i++) {
        if(ELF32_ST_TYPE(symtab_data[i].st_info) == STT_FUNC) {
            strcpy(func_map[func_count].name, strtab_data + symtab_data[i].st_name); // 根据偏置找名字
            func_map[func_count].addr = symtab_data[i].st_value;
            func_map[func_count].size = symtab_data[i].st_size;
            func_count++;
        }
    }

    // 节省空间
    function_map = malloc(func_count * sizeof(FunctionMap));
    memcpy(function_map, func_map, func_count * sizeof(FunctionMap));

    free(shdr_table);
    free(shstrtab_data);
    free(strtab_data);
    free(symtab_data);
    free(func_map);
    fclose(fp);
    Log("ELF file parsed successfully, %d functions found", func_count);
    return true;
}

bool init_ftrace(const char *elf_file, const char *log_file) {
    if(elf_file == NULL) {
        Log("FTRACE enabled but no ELF file provided! Function trace will not be available.");
        return false;
    }
    ftrace_log = fopen(log_file, "w");
    if(!ftrace_log) {
        Log("Failed to open ftrace log file: %s, Function trace will not be available.", log_file);
        return false;
    }
    Log("Function trace enabled using ELF file: %s, log file: %s", elf_file, log_file);
    return parse_elf(elf_file);
}

// 在function_map中查找对应addr的函数名
static const char* get_function_name(vaddr_t addr) {
    for(int i = 0; i < func_count; i++) {
        if(function_map[i].addr == addr) {
            return function_map[i].name;
        }
    }
    return NULL; // 未找到
}

// 根据地址查找它属于哪个函数
static const char* find_function_containing(vaddr_t addr) {
    for(int i = 0; i < func_count; i++) {
        // 检查地址是否在 [func.addr, func.addr + func.size) 范围内
        if(addr >= function_map[i].addr && addr < (function_map[i].addr + function_map[i].size)) {
            return function_map[i].name;
        }
    }
    return NULL; // 未找到
}

void print_function_map() {
    printf("Function Map, Count: %d\n", func_count);
    for(int i = 0; i < func_count; i++) {
        printf("Function: %s, Address: 0x%08x\n", function_map[i].name, function_map[i].addr);
    }
}

void ftrace_clean() {
    if(function_map) {
        free(function_map);
        function_map = NULL;
    }
    if(ftrace_log) {
        fclose(ftrace_log);
        ftrace_log = NULL;
    }
}

static void ftrace_log_write(const char *buf) {
    if(ftrace_log) {
        fputs(buf, ftrace_log);
    }
}

static bool should_ignore(const char *name) {
    if (name == NULL) return false;
    if (strcmp(name, ignore_funcs[0]) == 0) {
        return true;
    }
    // for (int i = 0; ignore_funcs[i] != NULL; i++) {
    //     if (strcmp(name, ignore_funcs[i]) == 0) {
    //         return true;
    //     }
    // }
    return false;
}

void ftrace_trace(Decode *_this, vaddr_t dnpc){
  // _this->logbuf[24]开始是反汇编助记符
  char ftracebuf[16384];
  char *ptr = ftracebuf;
  ftracebuf[0] = '\0'; // 清空
  if(memcmp(_this->logbuf + 24, "jal", 3) == 0){
    const char *func_name = get_function_name(dnpc);

    if(should_ignore(func_name)) return; // 过滤

    ptr += sprintf(ptr, "%.12s", _this->logbuf); // 复制类似 "0x80000000: " 的字符串
    for(int i = 0; i < g_ftrace_tab_num; i++) {
        ptr += sprintf(ptr, "  ");
    }
    sprintf(ptr, "call [%s@" FMT_WORD "]\n", func_name == NULL  ? "???" : func_name, dnpc);
    g_ftrace_tab_num++;
  }else if(memcmp(_this->logbuf + 24, "ret", 3) == 0){
    const char *func_name = find_function_containing(_this->pc);

    if(should_ignore(func_name)) return; // 过滤

    g_ftrace_tab_num = g_ftrace_tab_num > 0 ? g_ftrace_tab_num - 1 : 0; // 防止负数
    ptr += sprintf(ptr, "%.12s", _this->logbuf); // 复制类似 "0x80000000: " 的字符串
    for(int i = 0; i < g_ftrace_tab_num; i++) {
        ptr += sprintf(ptr, "  ");
    }
    sprintf(ptr, "ret  [%s]\n", func_name == NULL ? "???" : func_name);
  }
  if (ftracebuf[0] != '\0') { ftrace_log_write(ftracebuf); }
}

#endif