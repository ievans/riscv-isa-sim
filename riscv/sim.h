// See LICENSE for license details.

#ifndef _RISCV_SIM_H
#define _RISCV_SIM_H

#include <vector>
#include <string>
#include <memory>
#include "processor.h"
#include "mmu.h"

class htif_isasim_t;

// this class encapsulates the processors and memory in a RISC-V machine.
class sim_t
{
public:
  sim_t(size_t _nprocs, size_t mem_mb, const std::vector<std::string>& htif_args);
  ~sim_t();

  // run the simulation to completion
  int run();
  bool running();
  void stop();
  void set_debug(bool value);
  void set_histogram(bool value);
  void set_cache_level(bool value);
  void set_procs_debug(bool value);
  void set_procs_noisy(bool value);
  htif_isasim_t* get_htif() { return htif.get(); }
  mmu_t* get_debug_mmu() { return debug_mmu; }

  // deliver an IPI to a specific processor
  void send_ipi(reg_t who);

  // returns the number of processors in this simulator
  size_t num_cores() { return procs.size(); }
  processor_t* get_core(size_t i) { return procs.at(i); }

  // read one of the system control registers
  reg_t get_scr(int which);

  size_t get_memsz() {
    return memsz;
  }
  char* get_tagmem() {
    return tagmem;
  }
  size_t get_tagsz() {
    return tagsz;
  }
  void monitor();


private:
  std::unique_ptr<htif_isasim_t> htif;
  char* mem; // main memory
  size_t memsz; // memory size in bytes
  char *tagmem; // tag memory
  size_t tagsz; // tag memory size in bytes
  mmu_t* debug_mmu;  // debug port into main memory
  std::vector<processor_t*> procs;

  void step(size_t n); // step through simulation
  static const size_t INTERLEAVE = 5000;
  size_t current_step;
  size_t current_proc;
  bool debug;
  bool histogram_enabled; // provide a histogram of PCs
  int cache_level; // what memory accesses should take place for cache stats

  // presents a prompt for introspection into the simulation
  void interactive();

  // functions that help implement interactive()
  void interactive_quit(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_run(const std::string& cmd, const std::vector<std::string>& args, bool noisy);
  void interactive_run_noisy(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_run_silent(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_pc(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_asm(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_reg(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_wmem(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_wmem_t(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_wreg(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_wreg_t(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_regs(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_fregs(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_fregd(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_mem(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_dump(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_str(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_until(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_untilnot(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_when(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_watch(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_cachestats(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_cachereset(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_track_mem(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_track_reg(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_eval(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_insn(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_debug(const std::string& cmd, const std::vector<std::string>& args);
  std::string get_insn(const std::vector<std::string>& args);
  std::string get_insn_name(const std::vector<std::string>& args);
  tagged_reg_t get_reg(const std::vector<std::string>& args);
  void write_mem(const std::vector<std::string>& args);
  void write_mem_t(const std::vector<std::string>& args);
  void write_reg(const std::vector<std::string>& args);
  void write_reg_t(const std::vector<std::string>& args);
  void print_regs(const std::vector<std::string>& args);
  reg_t get_freg(const std::vector<std::string>& args);
  int parse_reg(const std::string& str);
  reg_t parse_addr(const std::string& addr_str);
  tag_t parse_tag(const std::string& tag_str);
  reg_t parse_val(processor_t* proc, const std::string& str);
  reg_t parse_expr(processor_t* proc, const std::string& str);
  std::string arg_join(const std::vector<std::string>& args, size_t start, size_t end = 0);
  tagged_reg_t get_mem(const std::vector<std::string>& args);
  tagged_reg_t* get_dump_tagged(const std::vector<std::string>& args);
  tagged_reg_t get_pc(const std::vector<std::string>& args);
  reg_t get_tohost(const std::vector<std::string>& args);
  void do_watch(processor_t* proc, reg_t addr);
  void do_until(const std::string& cmd, bool invert, const std::vector<std::string>& args);
  reg_t get_when(size_t proc, size_t numToGet);

  friend class htif_isasim_t;
};

extern volatile bool ctrlc_pressed;

#endif
