/*
// notes from meeting:
********************************************************************************
TAG_POLICY_MATCH_CALLRET
policy 1: match call/ret 
-- A. ret must have "ret" tag
-- B. only call sets "ret" tag
            
TAG_POLICY_NO_RETURN_COPY
policy 2: prevent copy of stack pointers for rop chain
-- A. only one copy of ret live
-- B. no arith on tags

TAG_POLICY_FP
policy 3: function pointers
-- A. priveleged functions to introduce function pointers
-- B. compiler support for casts
-- C.  every call checks for function pointer tag

TAG_POLICY_NO_FP_ARITH
policy 4: (experimental)
-- A. no arith on function pointer tags

TAG_POLICY_TEMPORAL
policy 5: temporal safety
-- todo
********************************************************************************
*/

#ifndef _RISCV_TAG_POLICY_H
#define _RISCV_TAG_POLICY_H

/*
void print_tag_policy() {
    printf("tag policies enabled: ");
    printf("TAG_POLICY_MATCH_CALLRET: %s\n",
#if defined(TAG_POLICY_MATCH_CALLRET) 
           "enabled");
#else
           "disabled");
#endif
    printf("TAG_POLICY_NO_RETURN_COPY: %s\n", 
#if defined(TAG_POLICY_NO_RETURN_COPY) 
           "enabled");
#else
           "disabled");
#endif
    printf("TAG_POLICY_FP: %s\n", 
#if defined(TAG_POLICY_FP) 
           "enabled");
#else
           "disabled");
#endif
}
*/

#define CLEAR_PC_TAG(tag) ((tag) & ~TAG_PC)

#ifdef TAG_POLICY_NO_RETURN_COPY
#define CLEAR_TAG(reg, tag) WRITE_REG_TAG(reg, CLEAR_PC_TAG(tag))
#endif // TAG_POLICY_NO_RETURN_COPY

#ifdef TAG_POLICY_NO_RETURN_COPY
  #define TAG_ADD(tag1, tag2) CLEAR_PC_TAG(((tag1) ^ (tag2)))
  #define TAG_SUB(tag1, tag2) CLEAR_PC_TAG(((tag1) ^ (tag2)))
  #define TAG_ARITH(tag1, tag2) CLEAR_PC_TAG(((tag1) & (tag2)))
  #define TAG_LOGIC(tag1, tag2) CLEAR_PC_TAG(((tag1) & (tag2)))
#elif defined TAG_POLICY_NO_PARTIAL_COPY
  #define TAG_ADD(tag1, tag2) TAG_MAX(tag1, tag2)
  #define TAG_SUB(tag1, tag2) TAG_MAX(tag1, tag2)
  #define TAG_ARITH(tag1, tag2) TAG_MIN(tag1, tag2)
  #define TAG_LOGIC(tag1, tag2) TAG_MIN(tag1, tag2)
#else
  #define TAG_ADD(tag1, tag2) ((tag1) ^ (tag2))
  #define TAG_SUB(tag1, tag2) ((tag1) ^ (tag2))
  #define TAG_ARITH(tag1, tag2) ((tag1) & (tag2))
  #define TAG_LOGIC(tag1, tag2) ((tag1) & (tag2))
#endif // TAG_POLICY_NO_RETURN_COPY

#define TAG_ADD_IMMEDIATE(tag1) (tag1)
#define TAG_SUB_IMMEDIATE(tag1) (tag1)
#define TAG_ARITH_IMMEDIATE(tag1) (tag1)
#define TAG_LOGIC_IMMEDIATE(tag1) (tag1)

#define TAG_DATA 4
#define TAG_PC 1
#define TAG_DEFAULT 0
#define PRIV_MASK TAG_PC
#define UNPRIV_MASK TAG_DATA

// Maximum privilege combination of two tags
#define TAG_MAX(tag1, tag2) ((PRIV_MASK & ((tag1) | (tag2))) | (UNPRIV_MASK & ((tag1) & (tag2))))
// Minimum privilege combination of two tags
#define TAG_MIN(tag1, tag2) ((PRIV_MASK & ((tag1) & (tag2))) | (UNPRIV_MASK & ((tag1) | (tag2))))

#define TAG_CSR TAG_DEFAULT
#define TAG_NULL 0
#define TAG_IMMEDIATE TAG_DEFAULT
#define TAG_FLOAT TAG_DATA
#define TAG_HTIF TAG_DEFAULT


#endif // TAG_POLICY_H
