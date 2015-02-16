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

