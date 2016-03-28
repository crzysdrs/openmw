#ifndef BIN_OP
#define BIN_OP(_name_, _print_)
#endif

BIN_OP(GT, ">")
BIN_OP(GTE,">=")
BIN_OP(LT, "<")
BIN_OP(LTE, "<=")
BIN_OP(EQ, "==")
BIN_OP(NEQ, "!=")
BIN_OP(PLUS, "+")
BIN_OP(MINUS, "-")
BIN_OP(MULT, "*")
BIN_OP(DIVIDE, "/")
BIN_OP(DOT, ".")
BIN_OP(ARROW, "->")
BIN_OP(NONE, "NONE")

#undef BIN_OP


#ifndef MW_TYPE
#define MW_TYPE(_name_, _print_)
#endif

MW_TYPE(UNDEFINED, "UNDEFINED")
MW_TYPE(FLOAT, "FLOAT")
MW_TYPE(LONG,  "LONG")
MW_TYPE(SHORT, "SHORT")
MW_TYPE(STRING, "STRING")
MW_TYPE(BOOL, "BOOL")

#undef MW_TYPE
