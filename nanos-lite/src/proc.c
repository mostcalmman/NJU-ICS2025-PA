#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;
void naive_uload(PCB *pcb, const char *filename);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);

static void inline context_kload(PCB* pcb, void (*entry)(void *), void *arg){
  pcb->cp = kcontext((Area){pcb->stack, pcb->stack + STACK_SIZE}, entry, arg);
}

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%d' for the %dth time!", (int)arg, j);
    j ++;
    yield();
  }
}

const int arr[1];
int const arr1[1];

void init_proc() {
  Log("Initializing processes...");
  
  
  // context_kload(&pcb[1], hello_fun, (void*)1);
  context_uload(&pcb[0], "/bin/cpu-exec", (char*[]){"/bin/cpu-exec", "--skip", NULL}, (char*[]) {NULL});
  // context_uload(&pcb[0], "/bin/nterm", (char*[]){"/bin/nterm"}, (char*[]) {NULL});
  // context_uload(&pcb[0], "/bin/dummy", (char*[]){"/bin/dummy"}, (char*[]) {NULL});
  
  switch_boot_pcb();
  yield();


  // load program here
  Log("Should not reach here. Switching to naive load...");
  naive_uload(NULL,"/bin/nterm");

}

Context* schedule(Context *prev) {
  // assert(current == &pcb_boot || current == &pcb[0] || current == &pcb[1]);
  // if (current == &pcb_boot) {
  //   Log("Switching from boot PCB to PCB 0");
  // }
  // else {
  //   Log("Switching from PCB %d to PCB %d", (current == &pcb[0] ? 0 : 1), (current == &pcb[0] ? 1 : 0));
  // }
  current->cp = prev;
  // current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  current = &pcb[0];
  return current->cp;
}
