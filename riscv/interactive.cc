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
#include <cctype>
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

    funcs["asm"] = &sim_t::interactive_asm;
    funcs["c"] = &sim_t::interactive_run_silent;
    funcs["dump"] = &sim_t::interactive_dump;
    funcs["eval"] = &sim_t::interactive_eval;
    funcs["fregs"] = &sim_t::interactive_fregs;
    funcs["fregd"] = &sim_t::interactive_fregd;
    funcs["m"] = &sim_t::interactive_mem;
    funcs["pc"] = &sim_t::interactive_pc;
    funcs["q"] = &sim_t::interactive_quit;
    funcs["r"] = &sim_t::interactive_regs;
    funcs["reg"] = &sim_t::interactive_reg;
    funcs["reset"] = &sim_t::interactive_cachereset;
    funcs["rn"] = &sim_t::interactive_run_noisy;
    funcs["rs"] = &sim_t::interactive_run_silent;
    funcs["stats"] = &sim_t::interactive_cachestats;
    funcs["str"] = &sim_t::interactive_str;
    funcs["tmem"] = &sim_t::interactive_track_mem;
    funcs["treg"] = &sim_t::interactive_track_reg;
    funcs["until"] = &sim_t::interactive_until;
    funcs["untilnot"] = &sim_t::interactive_untilnot;
    funcs["watch"] = &sim_t::interactive_watch;
    funcs["when"] = &sim_t::interactive_when;
    funcs["while"] = &sim_t::interactive_until;
    funcs["wmem"] = &sim_t::interactive_wmem;
    funcs["wmemt"] = &sim_t::interactive_wmem_t;
    funcs["wreg"] = &sim_t::interactive_wreg;
    funcs["wregt"] = &sim_t::interactive_wreg_t;

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
   fprintf(stderr, "0x%016" PRIx64 "\n", get_pc(args).val);
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
  if(args.size() >= 2) {
    pc = parse_expr(proc, arg_join(args, 1));
  } else {
    pc = proc->state.pc;
  }

  for(int i = 0; i < ASM_SIZE; i++) {
    insn_fetch_t fetch = mmu->load_insn(pc);
    uint64_t bits = fetch.insn.bits() & ((1ULL << (8 * insn_length(fetch.insn.bits()))) - 1);
    fprintf(stderr, "%3d: 0x%016" PRIx64 " (0x%08" PRIx64 ") %s\n",
          p, pc, bits, disassembler->disassemble(fetch.insn).c_str());
    pc += 4;
  }
}

tagged_reg_t sim_t::get_pc(const std::vector<std::string>& args)
{
  size_t p = 0;
  if (args.size() > 1)
    throw trap_illegal_instruction();
  if (args.size() == 1) {
    p = strtoul(args[0].c_str(), NULL, 10);
    if(p >= num_cores())
      throw trap_illegal_instruction();
  }

  tagged_reg_t tr;
  tr.val = procs[p]->state.pc;
  tr.tag = 0;

  return tr;
}

int sim_t::parse_reg(const std::string& str) {
  int r = std::find(xpr_name, xpr_name + NXPR, str) - xpr_name;
  if (r == NXPR)
    r = atoi(str.c_str());
  return r;
}

tagged_reg_t sim_t::get_reg(const std::vector<std::string>& args)
{
  size_t p = 0, start_of_reg = 0;
  if (args.size() > 2 || args.empty())
    throw trap_illegal_instruction();
  if (args.size() == 2) {
    p = strtoul(args[0].c_str(), NULL, 10);
    if(p >= num_cores())
      throw trap_illegal_instruction();
    start_of_reg = 1;
  }
  int r = parse_reg(args[start_of_reg]);

  tagged_reg_t tr;
  tr.val = procs[p]->state.XPR[r];
  tr.tag = procs[p]->state.XPR.read_tag(r);
  return tr;
}

void sim_t::print_regs(const std::vector<std::string>& args)
{
  int p = 0;
  if (args.size() > 1)
    throw trap_illegal_instruction();
  else if (args.size() == 1) {
    p = atoi(args[0].c_str());
    if (p >= (int)num_cores())
      throw trap_illegal_instruction();
  }

  // print PC as well
  fprintf(stderr, "%3s: 0x%016" PRIx64 " tag: ------  ",
    "pc", procs[p]->state.pc);

  // don't print zero reg
  size_t reg_num;
  for (reg_num = 1; reg_num < NXPR; ++reg_num) {
    fprintf(stderr, "%3s: 0x%016" PRIx64 " tag: 0x%04x",
      xpr_name[reg_num], procs[p]->state.XPR[reg_num],
      procs[p]->state.XPR.read_tag(reg_num));
    if (reg_num % 2 == 1) {
      fprintf(stderr, "\n");
    } else {
      fprintf(stderr, "  ");
    }
  }
}

void sim_t::write_reg(const std::vector<std::string>& args)
{
  size_t p = 0, start_of_reg = 0;
  if (args.size() < 2)
    throw trap_illegal_instruction();
  else if (args.size() > 2) {
    // ambiguous: could be a proc or an expression
    p = strtoul(args[0].c_str(), NULL, 10);
    start_of_reg = 1;
    if (p >= num_cores()) {
      // try extra arg as an expression
      p = 0;
      start_of_reg = 0;
    }
  }

  int r = parse_reg(args[start_of_reg]);
  if (r >= NXPR)
    throw trap_illegal_instruction();

  // convert value to reg_t
  reg_t value = parse_expr(procs[p], arg_join(args, start_of_reg + 1));
  
  procs[p]->state.XPR.write(r, value);
}

void sim_t::write_reg_t(const std::vector<std::string>& args)
{
  size_t p = 0, start_of_reg = 0;
  if (args.size() < 2 || args.size() > 3)
    throw trap_illegal_instruction();
  else if (args.size() == 3) {
    p = strtoul(args[0].c_str(), NULL, 10);
    if (p >= num_cores()) {
      throw trap_illegal_instruction();
    }
    start_of_reg = 1;
  }

  int r = parse_reg(args[start_of_reg]);
  if (r >= NXPR)
    throw trap_illegal_instruction();

  // convert tag value to tag_t
  tag_t value = parse_tag(args[start_of_reg + 1]);

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
  tagged_reg_t contents = get_reg(args);
  fprintf(stderr, "0x%016" PRIx64 " tag: 0x%04x\n", contents.val, contents.tag);
}

void sim_t::interactive_regs(const std::string& cmd, const std::vector<std::string>& args)
{
  print_regs(args);
}

void sim_t::interactive_mem(const std::string& cmd, const std::vector<std::string>& args)
{
  tagged_reg_t contents = get_mem(args);
  fprintf(stderr, "0x%016" PRIx64 " tag: 0x%04x\n", contents.val, contents.tag);
}

void sim_t::interactive_wmem(const std::string& cmd, const std::vector<std::string>& args)
{
  write_mem(args);
  fprintf(stderr, "Write success\n");    
}

void sim_t::interactive_wmem_t(const std::string& cmd, const std::vector<std::string>& args)
{
  write_mem_t(args);
  fprintf(stderr, "Tag write success\n");
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

void sim_t::write_mem(const std::vector<std::string>& args)
{
  size_t p = 0, start_of_addr = 0;
  if (args.size() < 2)
    throw trap_illegal_instruction();
  else if (args.size() >= 3) {
    // check processor number.
    p = strtoul(args[0].c_str(), NULL, 10);
    if(p >= num_cores())
      throw trap_illegal_instruction();
    start_of_addr = 1;
  }
  mmu_t* mmu = procs[p]->get_mmu(); 
  
  // check passed address (can be an expression; last arg is the value).
  std::string addr_str = arg_join(args, start_of_addr, args.size() - 1);
  reg_t addr = parse_expr(procs[p], addr_str);

  // parse value
  reg_t value = parse_addr(args.back());

  switch(addr % 8)
  {
    case 0:
      mmu->store_uint64(addr, value);
      break;
    case 4:
      mmu->store_uint32(addr, value);
      break;
    case 2:
    case 6:
      mmu->store_uint16(addr, value);
      break;
    default:
      mmu->store_uint8(addr, value);
      break;
  }
}

void sim_t::write_mem_t(const std::vector<std::string>& args)
{
  size_t p = 0, start_of_addr = 0;
  if (args.size() < 2)
    throw trap_illegal_instruction();
  else if (args.size() >= 3) {
    // check processor number.
    p = strtoul(args[0].c_str(), NULL, 10);
    if(p >= num_cores())
      throw trap_illegal_instruction();
    start_of_addr = 1;
  }
  mmu_t* mmu = procs[p]->get_mmu(); 
  
  // check passed address (can be an expression; last arg is the value).
  std::string addr_str = arg_join(args, start_of_addr, args.size() - 1);
  reg_t addr = parse_expr(procs[p], addr_str);

  // parse tag (will warn on overflow)
  tag_t tag = parse_tag(args.back());

  mmu->store_tag_value(tag, addr);
}

tagged_reg_t sim_t::get_mem(const std::vector<std::string>& args)
{
  size_t p = 0, start_of_addr = 0;
  if (args.empty())
    throw trap_illegal_instruction();
  else if (args.size() > 1) {
    // ambiguous: could be a proc or an expression
    p = strtoul(args[0].c_str(), NULL, 10);
    start_of_addr = 1;
    if (p >= num_cores()) {
      // try extra arg as an expression
      p = 0;
      start_of_addr = 0;
    }
  }
  mmu_t* mmu = procs[p]->get_mmu();

  std::string addr_str = arg_join(args, start_of_addr);
  reg_t addr = parse_expr(procs[p], addr_str);
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

std::string sim_t::arg_join(const std::vector<std::string>& args, size_t start, size_t end) {
  if(start >= args.size() || ((end != 0) && (end < start)) )
    throw trap_illegal_instruction();

  std::stringstream ss;
  if (end == 0) // don't provide an end
    end = args.size();
  for(size_t i = start; i < end; i++) {
    if(i != start)
      ss << " ";
    ss << args[i];
  }
  return ss.str();
}

reg_t sim_t::parse_val(processor_t* proc, const std::string& str) {
  const char *s = str.c_str();
  // Check for match in regnames
  for(int i = 0; i < NXPR; i++) {
    if(strcmp(s, xpr_name[i]) == 0)
      return proc->state.XPR[i];
  }
  // Check for match with pc
  if(strcmp(s, "pc") == 0)
    return proc->state.pc;

  // Check for hex
  if(strlen(s) >= 2 && s[0] == '0' && s[1] == 'x')
    return parse_addr(str);

  // None of the above, return decimal
  return strtol(s, NULL, 10);
}

reg_t sim_t::parse_expr(processor_t* proc, const std::string& str) {
  int priority[256];
  int unary[256];
  memset(priority, -1, sizeof(priority));
  memset(unary, -1, sizeof(unary));
  priority['%'] = 8;
  priority['/'] = 8;
  priority['*'] = 8;
  priority['<'] = 7;
  priority['>'] = 7;
  priority['+'] = 6;
  priority['-'] = 6;
  priority['&'] = 5;
  priority['|'] = 4;
  priority['^'] = 3;
  priority[0] = 0;

  unary['-'] = 1;
  unary['~'] = 1;
  unary['*'] = 1;

  std::vector<reg_t> buffer;
  std::vector<uint8_t> ops;
  std::vector<uint8_t> unary_ops;
  int n = str.size();
  int last = 0;

  for(int i = 0; i <= n; i++) {
    uint8_t c = 0;
    if(i < n)
      c = str[i];

    // ignore extra whitespace
    if(c == ' ' && i == last) {
      last = i+1;
      continue;
    }
    // handle unary operators
    if((buffer.size() == ops.size() || priority[c] < 0) && last == i && unary[c] >= 0) {
      unary_ops.push_back(c);
      last = i + 1;
      continue;
    }

    bool val_exists = false;
    reg_t val;

    // Handle parantheses
    if(c == '(') {
      if(i != last) {
        fprintf(stderr, "Failed to parse expression %s: consecutive values\n", str.c_str());
        throw trap_illegal_instruction();
      }
      // Find the matching ')'
      int balance = 1;
      int j;
      for(j = i+1; j < n; j++) {
        if(str[j] == '(')
          balance++;
        else if(str[j] == ')')
          balance--;
        if(balance == 0) break;
      }
      if(j == n) {
        fprintf(stderr, "Failed to parse expression %s: unbalanced parentheses\n", str.c_str());
        throw trap_illegal_instruction();
      }
      // Parse the subexpression
      val_exists = true;
      val = parse_expr(proc, str.substr(i+1, j - i - 1));

      // Update state
      i = j;
    }

    // Check if we are at the end of a value
    if(i > last && (c == ' ' || priority[c] >= 0)) {
      val_exists = true;
      val = parse_val(proc, str.substr(last, i - last));
      i -= 1;
    }

    if(val_exists) {
      // Apply unary operators
      while(!unary_ops.empty()) {
        uint8_t op = unary_ops.back(); unary_ops.pop_back();
        if(op == '-')
          val = -val;
        else if(op == '~')
          val = ~val;
        else if(op == '*') {
          mmu_t* mmu = proc->get_mmu();
          switch(val % 8)
          {
            case 0:
              val = mmu->load_tagged_uint64(val).val;
              break;
            case 4:
              val = mmu->load_tagged_uint32(val).val;
              break;
            case 2:
            case 6:
              val = mmu->load_tagged_uint16(val).val;
              break;
            default:
              val = mmu->load_tagged_uint8(val).val;
              break;
          }
        }
      }
      // Push new value to the value buffer
      buffer.push_back(val);
      if(buffer.size() > ops.size() + 1) {
        fprintf(stderr, "Failed to parse expression %s: consecutive values\n", str.c_str());
        throw trap_illegal_instruction();
      }
      last = i+1;
      continue;
    }

    // Apply operators
    if(priority[c] >= 0) {
      if(buffer.size() < ops.size() + 1) {
        fprintf(stderr, "Failed to parse expression %s: consecutive operators\n", str.c_str());
        throw trap_illegal_instruction();
      }
      if(unary_ops.size() > 0) {
        fprintf(stderr, "Failed to parse expression %s: unary operator applied to binary operator\n", str.c_str());
        throw trap_illegal_instruction();
      }
      // Handle two-char operators
      if(c == '<' || c == '>') {
        if(i+1 >= n || str[i+1] != str[i]) {
          fprintf(stderr, "Failed to parse expression %s: invalid operator\n", str.c_str());
          throw trap_illegal_instruction();
        }
        i++;
      }
      while(!ops.empty() && priority[c] <= priority[ops.back()]) {
        reg_t val2 = buffer.back(); buffer.pop_back();
        reg_t val1 = buffer.back(); buffer.pop_back();
        uint8_t op = ops.back(); ops.pop_back();
        reg_t val;
        if(op == '+')
          val = val1 + val2;
        else if(op == '-')
          val = val1 - val2;
        else if(op == '*')
          val = val1 * val2;
        else if(op == '/')
          val = val1 / val2;
        else if(op == '%')
          val = val1 % val2;
        else if(op == '<')
          val = val1 << val2;
        else if(op == '>')
          val = val1 >> val2;
        else if(op == '|')
          val = val1 | val2;
        else if(op == '&')
          val = val1 & val2;
        else if(op == '^')
          val = val1 ^ val2;
        else {
          fprintf(stderr, "Failed to parse expression %s: unimplemented operation\n", str.c_str());
          throw trap_illegal_instruction();
        }
        buffer.push_back(val);
      }
      ops.push_back(c);
      last = i + 1;
    }
  }

  if(buffer.empty()) {
    fprintf(stderr, "Failed to parse expression %s: no value found\n", str.c_str());
    throw trap_illegal_instruction();
  }
  return buffer.back();
}

reg_t sim_t::parse_addr(const std::string& addr_str) {
  reg_t addr = strtol(addr_str.c_str(),NULL,16);
  if(addr == LONG_MAX)
    addr = strtoul(addr_str.c_str(),NULL,16);
  return addr;
}

tag_t sim_t::parse_tag(const std::string& tag_str) {
  reg_t parsed_tag = parse_addr(tag_str);
  
  // check for overflow
  tag_t tag = (tag_t) parsed_tag;
  if (parsed_tag > std::numeric_limits<tag_t>::max())
    fprintf(stderr, "Tag 0x%016" PRIx64 " overflowed,"
            " interpreting as 0x%016" PRIu8 "\n", parsed_tag, tag);
  return tag;
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
  if(args.size() < 2)
    throw trap_illegal_instruction();
  std::string addr_str = arg_join(args, 1);

  // check processor number and get mmu.
  size_t p = strtoul(args[0].c_str(), NULL, 10);
  if(p >= num_cores())
    throw trap_illegal_instruction();
  mmu_t* mmu = procs[p]->get_mmu(); 

  // Process the provided address.
  reg_t addr = parse_expr(procs[p], addr_str);
 
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
  if(args.size() < 2)
    throw trap_illegal_instruction();

  int p = atoi(args[0].c_str());
  if(p >= (int)num_cores())
    throw trap_illegal_instruction();
  mmu_t* mmu = procs[p]->get_mmu();

  std::string addr_str = arg_join(args, 1);
  reg_t addr = parse_expr(procs[p], addr_str);

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

void sim_t::interactive_track_mem(const std::string& cmd, const std::vector<std::string>& args)
{
  if(args.size() < 2) {
    throw trap_illegal_instruction();
  }

  int p = atoi(args[0].c_str());
  if(p >= (int)num_cores()) {
    throw trap_illegal_instruction();
  }

  processor_t *proc = procs[p];
  mmu_t *mmu = proc->get_mmu();

  reg_t addr = parse_expr(proc, arg_join(args, 1));
  mmu->track_addr(addr);
}

void sim_t::interactive_eval(const std::string& cmd, const std::vector<std::string>& args)
{
  if(args.size() < 1) {
    throw trap_illegal_instruction();
  }

  processor_t *proc = procs[0];
  reg_t val = parse_expr(proc, arg_join(args, 0));
  fprintf(stderr, "0x%016" PRIx64 "\n", val);
}

void sim_t::interactive_track_reg(const std::string& cmd, const std::vector<std::string>& args)
{
  if(args.size() != 2) {
    throw trap_illegal_instruction();
  }

  int p = atoi(args[0].c_str());
  int r = parse_reg(args[1]);
  if(p >= (int)num_cores() || r >= NXPR) {
    throw trap_illegal_instruction();
  }

  mmu_t *mmu = procs[p]->get_mmu();
  mmu->track_reg(r);
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
  // Format of a valid until command:
  // until CMD [T] [PROC#] ARGS... VAL
  // ARGS can be empty, but CMD and VAL cannot
  // If args[1] is "t", we use tag value
  // If [PROC#] is not present, we use 0

  bool cmd_until = cmd == "until";

  if(args.size() < 2)
    return;

  // Check CMD for validity.
  auto func_t = args[0] == "reg" ? &sim_t::get_reg :
                args[0] == "pc"  ? &sim_t::get_pc  :
                args[0] == "m"   ? &sim_t::get_mem :
                NULL; 

  if (func_t == NULL)
    return;

  // Should we use tag or value?
  bool use_tag = (args[1] == "t");

  size_t p = 0, proc_arg = use_tag ? 2 : 1;
  bool proc_found = false;

  // If we can check the argument, and the first character is a digit,
  // use it as the processor number.
  if (args.size() > proc_arg && isdigit(args[proc_arg][0])) {
    p = strtoul(args[proc_arg].c_str(), NULL, 10);
    if (p >= num_cores())
      throw trap_illegal_instruction();
    proc_found = true;
  }

  // Calculate the index of the start of ARGS passed to CMD.
  size_t start_args2 = 1 + use_tag + proc_found;
  std::vector<std::string> args2;
  args2 = std::vector<std::string>(args.begin() + start_args2,
    args.end()-1);

  // The last argument (only one, sadly) can an expression here.
  reg_t val = parse_expr(procs[p], args.back());

  ctrlc_pressed = false;

  // Step loop.
  if (func_t != NULL) {
    while (1) {
      try {
        tagged_reg_t current = (this->*func_t)(args2);

        // Check tag or value, break if they match.
        if (cmd_until == (!use_tag) ? (current.val == val) : 
            (current.tag == val))
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
  // Format of a valid untilnot command:
  // untilnot CMD [T] [PROC#] ARGS... VAL
  // ARGS can be empty, but CMD and VAL cannot
  // If args[1] is "t", we use tag value
  // If [PROC#] is not present, we use 0

  bool cmd_untilnot = cmd == "untilnot";

  if(args.size() < 2)
    return;

  // Check CMD for validity.
  auto func_t = args[0] == "reg" ? &sim_t::get_reg :
                args[0] == "pc"  ? &sim_t::get_pc  :
                args[0] == "m"   ? &sim_t::get_mem :
                NULL; 

  if (func_t == NULL)
    return;

  // Should we use tag or value?
  bool use_tag = (args[1] == "t");

  size_t p = 0, proc_arg = use_tag ? 2 : 1;
  bool proc_found = false;

  // If we can check the argument, and the first character is a digit,
  // use it as the processor number.
  if (args.size() > proc_arg && isdigit(args[proc_arg][0])) {
    p = strtoul(args[proc_arg].c_str(), NULL, 10);
    if (p >= num_cores())
      throw trap_illegal_instruction();
    proc_found = true;
  }

  // Calculate the index of the start of ARGS passed to CMD.
  size_t start_args2 = 1 + use_tag + proc_found;
  std::vector<std::string> args2;
  args2 = std::vector<std::string>(args.begin() + start_args2,
    args.end()-1);

  // The last argument (only one, sadly) can an expression here.
  reg_t val = parse_expr(procs[p], args.back());

  ctrlc_pressed = false;

  // Step loop.
  if (func_t != NULL) {
    while (1) {
      try {
        tagged_reg_t current = (this->*func_t)(args2);

        // Check tag or value, break if they DON'T match.
        if (cmd_untilnot == (!use_tag) ? (current.val != val) : 
            (current.tag != val))
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
