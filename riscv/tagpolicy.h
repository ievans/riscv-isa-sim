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


#define TAG_ADD(tag1, tag2) ((tag1) ^ (tag2))
#define TAG_SUB(tag1, tag2) ((tag1) ^ (tag2))
#define TAG_ARITH(tag1, tag2) ((tag1) & (tag2))
#define TAG_LOGIC(tag1, tag2) ((tag1) & (tag2))

#define TAG_ADD_IMMEDIATE(tag1) (tag1)
#define TAG_SUB_IMMEDIATE(tag1) (tag1)
#define TAG_ARITH_IMMEDIATE(tag1) (tag1)
#define TAG_LOGIC_IMMEDIATE(tag1) (tag1)

#define TAG_CSR 0
#define TAG_PC 1
#define TAG_NULL 0
#define TAG_IMMEDIATE 0

#ifdef TAG_POLICY_NO_RETURN_COPY
#define CLEAR_PC_TAG(tag) 0
#define CLEAR_TAG(reg, tag) WRITE_REG_TAG(reg, CLEAR_PC_TAG(tag))
#endif
