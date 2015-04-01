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
    funcs["fregs"] = &sim_t::interactive_fregs;
    funcs["fregd"] = &sim_t::interactive_fregd;
    funcs["mem"] = &sim_t::interactive_mem;
    funcs["memt"] = &sim_t::interactive_mem_t;
    funcs["dump"] = &sim_t::interactive_dump;
    funcs["pc"] = &sim_t::interactive_pc;
    funcs["asm"] = &sim_t::interactive_asm;
    funcs["str"] = &sim_t::interactive_str;
    funcs["until"] = &sim_t::interactive_until;
    funcs["while"] = &sim_t::interactive_until;
    funcs["q"] = &sim_t::interactive_quit;
    funcs["stats"] = &sim_t::interactive_cachestats;

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
      val = mmu->load_uint64(addr);
      break;
    case 4:
      val = mmu->load_uint32(addr);
      break;
    case 2:
    case 6:
      val = mmu->load_uint16(addr);
      break;
    default:
      val = mmu->load_uint8(addr);
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

void sim_t::interactive_str(const std::string& cmd, const std::vector<std::string>& args)
{
  if(args.size() != 1)
    throw trap_illegal_instruction();

  reg_t addr = strtol(args[0].c_str(),NULL,16);

  char ch;
  while((ch = debug_mmu->load_uint8(addr++)))
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
