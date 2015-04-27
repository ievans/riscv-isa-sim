// See LICENSE for license details.

#include "decode.h"
#include "disasm.h"
#include "sim.h"
#include "htif.h"
#include <sys/mman.h>
#include <termios.h>
#include <map>
#include <iostream>
#include <climits>
#include <cinttypes>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

static std::string readline(int fd)
{
  struct termios tios;
  bool noncanonical = tcgetattr(fd, &tios) == 0 && (tios.c_lflag & ICANON) == 0;

  std::string s;
  for (char ch; read(fd, &ch, 1) == 1; )
  {
    if (ch == '\x7f')
    {
      if (s.empty())
        continue;
      s.erase(s.end()-1);

      if (noncanonical && write(fd, "\b \b", 3) != 3)
        ; // shut up gcc
    }
    else if (noncanonical && write(fd, &ch, 1) != 1)
      ; // shut up gcc

    if (ch == '\n')
      break;
    if (ch != '\x7f')
      s += ch;
  }
  return s;
}

void sim_t::interactive()
{
  while (!htif->done())
  {
    std::cerr << ": " << std::flush;
    std::string s = readline(2);

    std::stringstream ss(s);
    std::string cmd, tmp;
    std::vector<std::string> args;

    if (!(ss >> cmd))
    {
      set_procs_debug(true);
      step(1);
      continue;
    }

    while (ss >> tmp)
      args.push_back(tmp);

    typedef void (sim_t::*interactive_func)(const std::string&, const std::vector<std::string>&);
    std::map<std::string,interactive_func> funcs;

    funcs["c"] = &sim_t::interactive_run_silent;
    funcs["r"] = &sim_t::interactive_run_noisy;
    funcs["rs"] = &sim_t::interactive_run_silent;
    funcs["reg"] = &sim_t::interactive_reg;
    funcs["regt"] = &sim_t::interactive_reg_t;
    funcs["wreg"] = &sim_t::interactive_wreg;
    funcs["wregt"] = &sim_t::interactive_wreg_t;
    funcs["fregs"] = &sim_t::interactive_fregs;
    funcs["fregd"] = &sim_t::interactive_fregd;
    funcs["mem"] = &sim_t::interactive_mem;
    funcs["memt"] = &sim_t::interactive_mem_t;
    funcs["dump"] = &sim_t::interactive_dump;
    funcs["watch"] = &sim_t::interactive_watch;
    funcs["when"] = &sim_t::interactive_when;
    funcs["pc"] = &sim_t::interactive_pc;
    funcs["asm"] = &sim_t::interactive_asm;
    funcs["str"] = &sim_t::interactive_str;
    funcs["until"] = &sim_t::interactive_until;
    funcs["untilnot"] = &sim_t::interactive_untilnot;
    funcs["while"] = &sim_t::interactive_until;
    funcs["q"] = &sim_t::interactive_quit;
    funcs["stats"] = &sim_t::interactive_cachestats;
    funcs["reset"] = &sim_t::interactive_cachereset;

    try
    {
      if(funcs.count(cmd))
        (this->*funcs[cmd])(cmd, args);
    }
    catch(trap_t t) {}
  }
  ctrlc_pressed = false;
}

void sim_t::interactive_run_noisy(const std::string& cmd, const std::vector<std::string>& args)
{
  interactive_run(cmd,args,true);
}

void sim_t::interactive_run_silent(const std::string& cmd, const std::vector<std::string>& args)
{
  interactive_run(cmd,args,false);
}

void sim_t::interactive_run(const std::string& cmd, const std::vector<std::string>& args, bool noisy)
{
  size_t steps = args.size() ? atoll(args[0].c_str()) : -1;
  ctrlc_pressed = false;
  set_procs_debug(noisy);
  for (size_t i = 0; i < steps && !ctrlc_pressed && !htif->done(); i++)
    step(1);
}

void sim_t::interactive_quit(const std::string& cmd, const std::vector<std::string>& args)
{
  exit(0);
}

void sim_t::interactive_pc(const std::string& cmd, const std::vector<std::string>& args)
{
   fprintf(stderr, "0x%016" PRIx64 "\n", get_pc(args));
}

#define ASM_SIZE 16
void sim_t::interactive_asm(const std::string& cmd, const std::vector<std::string>& args) {
  int p = 0;
  if(args.size() >= 1) {
    p = atoi(args[0].c_str());
  }
  if(p >= (int)num_cores()) {
    throw trap_illegal_instruction();
  }

  processor_t *proc = procs[p];
  mmu_t *mmu = proc->get_mmu();
  disassembler_t* disassembler = proc->get_disassembler();

  reg_t pc = 0;
  if(args.size() >= 2 && strcmp(args[1].c_str(), "pc") != 0) {
    pc = parse_addr(args[1]);
  } else {
    pc = proc->state.pc;
  }

  int n = ASM_SIZE;
  if(args.size() >= 3) {
    n = atoi(args[2].c_str());
  }

  for(int i = 0; i < n; i++) {
    insn_fetch_t fetch = mmu->load_insn(pc);
    uint64_t bits = fetch.insn.bits() & ((1ULL << (8 * insn_length(fetch.insn.bits()))) - 1);
    fprintf(stderr, "%3d: 0x%016" PRIx64 " (0x%08" PRIx64 ") %s\n",
          p, pc, bits, disassembler->disassemble(fetch.insn).c_str());
    pc += 4;
  }
}

reg_t sim_t::get_pc(const std::vector<std::string>& args)
{
  if(args.size() != 1)
    throw trap_illegal_instruction();

  int p = atoi(args[0].c_str());
  if(p >= (int)num_cores())
    throw trap_illegal_instruction();

  return procs[p]->state.pc;
}

reg_t sim_t::get_reg(const std::vector<std::string>& args)
{
  if(args.size() != 2)
    throw trap_illegal_instruction();

  int p = atoi(args[0].c_str());
  int r = std::find(xpr_name, xpr_name + NXPR, args[1]) - xpr_name;
  if (r == NXPR)
    r = atoi(args[1].c_str());
  if(p >= (int)num_cores() || r >= NXPR)
    throw trap_illegal_instruction();

  return procs[p]->state.XPR[r];
}

tagged_reg_t sim_t::get_reg_tagged(const std::vector<std::string>& args)
{
  if(args.size() != 2)
    throw trap_illegal_instruction();

  int p = atoi(args[0].c_str());
  int r = std::find(xpr_name, xpr_name + NXPR, args[1]) - xpr_name;
  if (r == NXPR)
    r = atoi(args[1].c_str());
  if(p >= (int)num_cores() || r >= NXPR)
    throw trap_illegal_instruction();

  tagged_reg_t tr;
  tr.val = procs[p]->state.XPR[r];
  tr.tag = procs[p]->state.XPR.read_tag(r);
  return tr;
}

void sim_t::write_reg(const std::vector<std::string>& args)
{
  if (args.size() != 3)
    throw trap_illegal_instruction();

  int p = atoi(args[0].c_str());
  int r = std::find(xpr_name, xpr_name + NXPR, args[1]) - xpr_name;
  if (r == NXPR)
    r = atoi(args[1].c_str());
  if (p >= (int)num_cores() || r >= NXPR)
    throw trap_illegal_instruction();

  // convert value to reg_t
  reg_t value = parse_addr(args[2]);
  
  procs[p]->state.XPR.write(r, value);
}

void sim_t::write_reg_t(const std::vector<std::string>& args)
{
  if (args.size() != 3)
    throw trap_illegal_instruction();

  int p = atoi(args[0].c_str());
  int r = std::find(xpr_name, xpr_name + NXPR, args[1]) - xpr_name;
  if (r == NXPR)
    r = atoi(args[1].c_str());
  if (p >= (int)num_cores() || r >= NXPR)
    throw trap_illegal_instruction();

  // convert tag value to tag_t
  tag_t value = parse_tag(args[2]);

  procs[p]->state.XPR.write_tag(r, value);
}

reg_t sim_t::get_freg(const std::vector<std::string>& args)
{
  if(args.size() != 2)
    throw trap_illegal_instruction();

  int p = atoi(args[0].c_str());
  int r = std::find(fpr_name, fpr_name + NFPR, args[1]) - fpr_name;
  if (r == NFPR)
    r = atoi(args[1].c_str());
  if(p >= (int)num_cores() || r >= NFPR)
    throw trap_illegal_instruction();

  return procs[p]->state.FPR[r];
}

void sim_t::interactive_reg(const std::string& cmd, const std::vector<std::string>& args)
{
  fprintf(stderr, "0x%016" PRIx64 "\n", get_reg(args));
}

void sim_t::interactive_reg_t(const std::string& cmd, const std::vector<std::string>& args)
{
  tagged_reg_t contents = get_reg_tagged(args);
  fprintf(stderr, "0x%016" PRIx64 " tag: 0x%04x\n", contents.val, contents.tag);
}

void sim_t::interactive_wreg(const std::string& cmd, const std::vector<std::string>& args)
{
  write_reg(args);
  fprintf(stderr, "Write success\n");
}

void sim_t::interactive_wreg_t(const std::string& cmd, const std::vector<std::string>& args)
{
  write_reg_t(args);
  fprintf(stderr, "Tag write success\n");
}

union fpr
{
  reg_t r;
  float s;
  double d;
};

void sim_t::interactive_fregs(const std::string& cmd, const std::vector<std::string>& args)
{
  fpr f;
  f.r = get_freg(args);
  fprintf(stderr, "%g\n",f.s);
}

void sim_t::interactive_fregd(const std::string& cmd, const std::vector<std::string>& args)
{
  fpr f;
  f.r = get_freg(args);
  fprintf(stderr, "%g\n",f.d);
}

reg_t sim_t::get_mem(const std::vector<std::string>& args)
{
  if(args.size() != 1 && args.size() != 2)
    throw trap_illegal_instruction();

  std::string addr_str = args[0];
  mmu_t* mmu = debug_mmu;
  if(args.size() == 2)
  {
    int p = atoi(args[0].c_str());
    if(p >= (int)num_cores())
      throw trap_illegal_instruction();
    mmu = procs[p]->get_mmu();
    addr_str = args[1];
  }

  reg_t addr = parse_addr(addr_str);
  reg_t val;

  switch(addr % 8)
  {
    case 0:
      val = mmu->load_tagged_uint64(addr).val;
      break;
    case 4:
      val = mmu->load_tagged_uint32(addr).val;
      break;
    case 2:
    case 6:
      val = mmu->load_tagged_uint16(addr).val;
      break;
    default:
      val = mmu->load_tagged_uint8(addr).val;
      break;
  }
  return val;
}

reg_t sim_t::parse_addr(std::string addr_str) {
  reg_t addr = strtol(addr_str.c_str(),NULL,16);
  if(addr == LONG_MAX)
    addr = strtoul(addr_str.c_str(),NULL,16);
  return addr;
}

tag_t sim_t::parse_tag(std::string tag_str) {
  reg_t parsed_tag = parse_addr(tag_str);
  
  // check for overflow
  tag_t tag = (tag_t) parsed_tag;
  if (parsed_tag > std::numeric_limits<tag_t>::max())
    fprintf(stderr, "Tag 0x%016" PRIx64 " overflowed,"
            " interpreting as 0x%016" PRIu8 "\n", parsed_tag, tag);
  return tag;
}

tagged_reg_t sim_t::get_mem_tagged(const std::vector<std::string>& args)
{
  if(args.size() != 1 && args.size() != 2)
    throw trap_illegal_instruction();

  std::string addr_str = args[0];
  mmu_t* mmu = debug_mmu;
  if(args.size() == 2)
  {
    int p = atoi(args[0].c_str());
    if(p >= (int)num_cores())
      throw trap_illegal_instruction();
    mmu = procs[p]->get_mmu();
    addr_str = args[1];
  }

  reg_t addr = parse_addr(addr_str);
  tagged_reg_t val;

  switch(addr % 8)
  {
    case 0:
      val = mmu->load_tagged_uint64(addr);
      break;
    case 4:
      val = mmu->load_tagged_uint32(addr);
      break;
    case 2:
    case 6:
      val = mmu->load_tagged_uint16(addr);
      break;
    default:
      val = mmu->load_tagged_uint8(addr);
      break;
  }
  return val;
}

void sim_t::do_watch(size_t proc, reg_t addr)
{
  // Check processor number and get MMU.
  if(proc >= num_cores())
    throw trap_illegal_instruction();
  mmu_t* mmu = procs[proc]->get_mmu(); 

  // Get the watch_loc and tell it to watch.
  watch_loc * wl = mmu->get_watch_loc();
  if (wl) {
    wl->update_addr(addr);
    std::cerr << "Now watching..." << std::endl;
  } else {
    std::cerr << "Memory watching not enabled at startup." << std::endl;
  }
}

void sim_t::interactive_watch(const std::string& cmd, const std::vector<std::string>& args)
{
  if(args.size() != 2)
    throw trap_illegal_instruction();
  std::string addr_str = args[1];

  // Check processor number and get mmu.
  size_t p = strtoul(args[0].c_str(), NULL, 10);
  if(p >= num_cores())
    throw trap_illegal_instruction();
  mmu_t* mmu = procs[p]->get_mmu(); 

  // Process the provided address.
  reg_t addr = parse_addr(addr_str);
 
  // Tell the processor to watch. 
  do_watch(p, addr);
}


reg_t sim_t::get_when(size_t proc, size_t numToGet)
{
  // Check processor number and get mmu.
  if(proc >= num_cores())
    throw trap_illegal_instruction();
  mmu_t* mmu = procs[proc]->get_mmu(); 

  // Get our watch_loc and get the write's PC.
  watch_loc* wl = mmu->get_watch_loc();
  if (!wl) {
    std::cerr << "No write tracer found." << std::endl;
    return (reg_t) 0; // Handle case when watch_loc is not turned on.
  }

  return wl->get_nth_recent_access(numToGet);
}

void sim_t::interactive_when(const std::string& cmd, const std::vector<std::string>& args)
{
  if(args.size() < 1 || args.size() > 2)
    throw trap_illegal_instruction();

  // Get processor number.
  size_t p = (size_t) strtoul(args[0].c_str(), NULL, 10);
  if(p >= num_cores())
    throw trap_illegal_instruction();

  // Get number of writes to print.
  size_t n;
  if (args.size() == 2) {
    n = (size_t) strtoul(args[1].c_str(), NULL, 10);
  } else {
    n = 16; // default: print 16 accesses
  }

  // Print the PC of the most recent n writes
  // to the address being watched.
  for (size_t i = 0; i < n; i++) {
    reg_t pc = get_when(p, i);
    std::cerr << std::dec << i << " writes ago, PC was: 0x" << std::hex << pc << "\n";
  }
  std::cerr << std::endl;
}


#define DUMP_SIZE 16 // in words
tagged_reg_t dump_buffer[DUMP_SIZE];
reg_t dump_addr;
tagged_reg_t* sim_t::get_dump_tagged(const std::vector<std::string>& args)
{
  if(args.size() != 1 && args.size() != 2)
    throw trap_illegal_instruction();

  std::string addr_str = args[0];
  mmu_t* mmu = debug_mmu;
  if(args.size() == 2)
  {
    int p = atoi(args[0].c_str());
    if(p >= (int)num_cores())
      throw trap_illegal_instruction();
    mmu = procs[p]->get_mmu();
    addr_str = args[1];
  }

  reg_t addr = parse_addr(addr_str);

  if(addr % 8 != 0) {
    return NULL;
  }
  int i;
  for(i = 0; i < DUMP_SIZE; i++) {
    dump_buffer[i] = mmu->load_tagged_uint64(addr + 8*i);
  }
  dump_addr = addr;
  return dump_buffer;
}

void sim_t::interactive_mem(const std::string& cmd, const std::vector<std::string>& args)
{
  fprintf(stderr, "0x%016" PRIx64 "\n", get_mem(args));
}

void sim_t::interactive_mem_t(const std::string& cmd, const std::vector<std::string>& args)
{
  tagged_reg_t contents = get_mem_tagged(args);
  fprintf(stderr, "0x%016" PRIx64 " tag: 0x%04x\n", contents.val, contents.tag);
}

#define DUMP_WIDTH 2
void sim_t::interactive_dump(const std::string& cmd, const std::vector<std::string>& args)
{
  tagged_reg_t *contents = get_dump_tagged(args);
  reg_t addr = dump_addr;
  if(contents != NULL) {
    int i, j;
    for(i = 0; i < DUMP_SIZE / DUMP_WIDTH; i++) {
      fprintf(stderr, "0x%016" PRIx64 ": ", addr + 8 * DUMP_WIDTH * i);
      for(j = 0; j < DUMP_WIDTH; j++) {
        int k = DUMP_WIDTH * i + j;
        if(j > 0) {
          fprintf(stderr, "    ");
        }
        fprintf(stderr, "0x%016" PRIx64 " 0x%04x", contents[k].val, contents[k].tag);
      }
      fprintf(stderr, "\n");
    }
  } else {
    fprintf(stderr, "Error: dump requires 64-bit aligned address\n");
  }
}

void sim_t::interactive_cachestats(const std::string& cmd, const std::vector<std::string>& args) {
  int p = 0;
  if(args.size() >= 1)
    p = atoi(args[0].c_str());

  // Get the mmu
  if(p >= (int)num_cores())
    throw trap_illegal_instruction();
  mmu_t* mmu = procs[p]->get_mmu();

  mmu->print_memtracer();
}

void sim_t::interactive_cachereset(const std::string& cmd, const std::vector<std::string>& args) {
  int p = 0;
  if(args.size() >= 1)
    p = atoi(args[0].c_str());

  // Get the mmu
  if(p >= (int)num_cores())
    throw trap_illegal_instruction();
  mmu_t* mmu = procs[p]->get_mmu();

  mmu->reset_memtracer();
  fprintf(stderr, "Caches have been reset.\n");
}

void sim_t::interactive_str(const std::string& cmd, const std::vector<std::string>& args)
{
  if(args.size() != 1)
    throw trap_illegal_instruction();

  reg_t addr = strtol(args[0].c_str(),NULL,16);

  char ch;
  while((ch = debug_mmu->load_tagged_uint8(addr++).val))
    putchar(ch);

  putchar('\n');
}

void sim_t::interactive_until(const std::string& cmd, const std::vector<std::string>& args)
{
  bool cmd_until = cmd == "until";

  if(args.size() < 3)
    return;

  reg_t val = strtol(args[args.size()-1].c_str(),NULL,16);
  if(val == LONG_MAX)
    val = strtoul(args[args.size()-1].c_str(),NULL,16);
  
  std::vector<std::string> args2;
  args2 = std::vector<std::string>(args.begin()+1,args.end()-1);

  // returns reg_t
  auto func = args[0] == "reg"  ? &sim_t::get_reg :
              args[0] == "pc"   ? &sim_t::get_pc :
              args[0] == "mem"  ? &sim_t::get_mem :
              NULL;

  // returns tagged_reg_t
  auto func_t = args[0] == "regt" ? &sim_t::get_reg_tagged :
                args[0] == "memt" ? &sim_t::get_mem_tagged :
                NULL;

  if (func == NULL && func_t == NULL)
    return;

  ctrlc_pressed = false;

  // loop on test returning reg_t
  if (func != NULL) {
    while (1) {
      try {
        reg_t current = (this->*func)(args2);

        if (cmd_until == (current == val))
          break;
        if (ctrlc_pressed)
          break;
      }
      catch (trap_t t) {}

      set_procs_debug(false);
      step(1);
    }
  } else if (func_t != NULL) { // loop on test returning tagged_reg_t
    while (1) {
      try {
        tagged_reg_t current = (this->*func_t)(args2);

        if (cmd_until == (current.tag == val))
          break;
        if (ctrlc_pressed)
          break;
      }
      catch (trap_t t) {}

      set_procs_debug(false);
      step(1);
    }
  }
}

void sim_t::interactive_untilnot(const std::string& cmd, const std::vector<std::string>& args)
{
  bool cmd_untilnot = cmd == "untilnot";

  if(args.size() < 3)
    return;

  reg_t val = strtol(args[args.size()-1].c_str(),NULL,16);
  if(val == LONG_MAX)
    val = strtoul(args[args.size()-1].c_str(),NULL,16);
  
  std::vector<std::string> args2;
  args2 = std::vector<std::string>(args.begin()+1,args.end()-1);

  // returns reg_t
  auto func = args[0] == "reg"  ? &sim_t::get_reg :
    args[0] == "pc"   ? &sim_t::get_pc :
    args[0] == "mem"  ? &sim_t::get_mem :
    NULL;

  // returns tagged_reg_t
  auto func_t = args[0] == "regt" ? &sim_t::get_reg_tagged :
    args[0] == "memt" ? &sim_t::get_mem_tagged :
    NULL;

  if (func == NULL && func_t == NULL)
    return;

  ctrlc_pressed = false;

  // loop on test returning reg_t
  if (func != NULL) {
    while (1) {
      try {
        reg_t current = (this->*func)(args2);

        if (cmd_untilnot == (current != val))
          break;
        if (ctrlc_pressed)
          break;
      }
      catch (trap_t t) {}

      set_procs_debug(false);
      step(1);
    }
  } else if (func_t != NULL) { // loop on test returning tagged_reg_t
    while (1) {
      try {
        tagged_reg_t current = (this->*func_t)(args2);

        if (cmd_untilnot == (current.tag != val))
          break;
        if (ctrlc_pressed)
          break;
      }
      catch (trap_t t) {}

      set_procs_debug(false);
      step(1);
    }
  }
}
