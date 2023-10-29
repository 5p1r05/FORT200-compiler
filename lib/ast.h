
#ifndef _AST_H_
#define _AST_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/****************************************************/
/********************** STRUCTS *********************/
/****************************************************/

/** Declaration structs **/

// Value representing the type of an expression
typedef enum {
	INT=0, LOG, REAL, CHAR, STR, CMPLX, REC, AMBIGUOUS
} type_t;

// Storing a bool for the type of a list func (i.e access a list node / return the address of the next node)
// and the number of hops from the head of the list to get to the target node
typedef struct {
	bool access;
	int hops;
} AST_Listfunc;

// Values representing the op used as sign or addop
typedef enum {MINUS=-1, NONE=0, PLUS=1} AST_Sign;

// Values representing different relops
typedef enum {R_GT=0, R_GE, R_LT, R_LE, R_EQ, R_NE} AST_Relop;

// Representation of complex as two reals
typedef struct {
	double re;
	double im;
} complex_t;

// Struct representing a generic expression's type and value 
typedef struct {
	// Value of the expression, accessing the field
	// dictated by the type
	union  {
		int  intval;
		bool  lval;
		double rval;
		char charval;
		char *strval;
		complex_t cmplxval;
	};
	// Type of the expression's result
	type_t type;
} AST_Constant;

typedef type_t AST_Type;

// Struct containing an array of pointers to AST_Constant
// used to represent values and value_list
typedef struct {
	int size;
	AST_Constant **elements;
} AST_Values;

// Struct representing the intialization of a variable
typedef struct {
	char *id;
	AST_Values *value_list;
} init_val_t;

// Struct containing array of pointers to AST_Values
// used to represent vals
typedef struct {
	int size;
	init_val_t **elements;
} AST_Vals;

typedef struct decl decl_t;

typedef enum {DIM_LITERAL=0, DIM_VARIABLE} dim_type_t;

// Struct for dim that can either be literal or variable
typedef struct {
	dim_type_t type;
	union {
		// if type is DIM_VARIABLE
		decl_t *decl; // Stores the stbl entry for the id after it is declared
		char *id; // Used for temporarily storing a possibly undeclared id
		// if type is DIM_LITERAL
		int val;
	};
} AST_Dim;

// Struct containing info for a dim array
typedef struct {
	int size;
	AST_Dim **elements;
} AST_Dims;

// Value representing the type of an undefined variable
typedef enum {SCALAR=0, ARRAY, LIST} AST_UndefVar_Type;

// Struct representing an undefined variable. It contains its type, its dimensions
//  and a pointer to the same struct type. The last is used to create a list
typedef struct undef_var_t {
	AST_UndefVar_Type type; // Is never LIST
	AST_Dims *dims;
	int list_depth; // No need for linked-list of UndefVars, just keep a counter
	char *id;
} AST_UndefVar;

// Struct containing an array of pointers to AST_UndefVar
// used to represent vars
typedef struct {
	int size;
	AST_UndefVar **elements;
} AST_Vars;

typedef struct fields AST_Fields;

// Struct containing information about a field.
// When a field is a record, it holds an array of its subfields
typedef struct field {
	AST_Vars *vars; // Variables with that type
	/* The type */
	type_t type;
	/* if type == REC store its fields*/
	AST_Fields *fields;
} AST_Field;

// Struct containing an array of fields
struct fields {
	int size;
	AST_Field **elements;
};

// General type that covers all data types
typedef struct {
	type_t type;
	AST_Fields *fields; // Only if type == REC
} AST_GeneralType;

// Struct for the declaration of a single id
struct decl {
	AST_UndefVar *variable; // Many variables (SCALAR, ARRAY or LIST)
	AST_GeneralType *datatype; // The common datatype of all vars
	AST_Values *initial_value; // Same index as vars
	bool is_parameter; // Used to identify function parameters
};

// Struct for declarations tree
typedef struct {
	int size;
	decl_t **declarations;
} AST_Decls;


/** Statement structs **/
typedef enum {EXPR_UNARY, EXPR_BINARY, EXPR_VARIABLE, EXPR_CONSTANT, EXPR_LISTEXPR} expr_type_t;
typedef enum {U_PLUS, U_MINUS, U_NOT, U_LENGTH, U_NEW} unary_op_t ;
typedef enum {
	// DO NOT THINK OF CHANGING THE ORDER
	B_PLUS=0, B_MINUS, B_AND, B_OR,
	B_GT, B_GE, B_LT, B_LE, B_EQ,
	B_NE, B_MUL, B_DIV, B_POWER, B_CMPLX
} binary_op_t;

// Forward declaration of structs
typedef struct variable AST_Variable;
typedef struct expressions AST_Expressions;
typedef struct subprogram AST_Subprogram;

// Expression struct
typedef struct expression {
	expr_type_t expr_type;
	AST_GeneralType *datatype;
	int list_depth; // 0 if not list
	AST_Dims *dims;

	union {
		// Unary: Expressions with one expression child
		struct {
			unary_op_t op;
			struct expression *child;
		} unary;

		// Binary: Expressions with two expression childs
		struct {
			binary_op_t op;
			struct expression *child1;
			struct expression *child2;
		} binary;
		
		// Expressions is variable
		AST_Variable *variable;

		// Expressions is constant
		AST_Constant *constant;
		
		// Initialization of list as a list of expressions
		AST_Expressions *listexpr;
	};
} AST_Expression;

// Array of expressions
struct expressions {
	int size;
	AST_Expression **elements;
};

typedef enum {V_ID, V_DECL, V_FUNC_CALL, V_ARRAY_ACCESS, V_REC_ACCESS, V_LISTFUNC} variable_type_t;

// Variable Struct
struct variable {
	variable_type_t type;
	AST_GeneralType *datatype;
	int list_depth;
	AST_Dims *dims;

	union {
		// V_ID
		struct {
			char *id; // Used initially and transformed to another type when reused
			AST_Expressions *exprs; // Used temporarily by a yet unidentified func call
		};
		
		// V_DECL
		decl_t *decl;

		// V_FUNC_CALL
		struct {
			AST_Subprogram *subprog;
			AST_Expressions *args;
		};

		// V_ARRAY_ACCESS
		struct {
			AST_Variable *parent_rec; // used for a record field array 
			AST_UndefVar *array;
			AST_Expressions *indices;
		};

		// V_REC_ACCESS
		struct {
			AST_Variable *record;
			AST_UndefVar *field_var; // Pointer to record in record.datatype.fields.elements[]
		};

		// V_LISTFUNC
		struct {
			AST_Listfunc *listfunc;
			AST_Expression *list;
		};
	};
};


// Assignment types
typedef enum {AS_EXPRESSION, AS_STRING} assignment_type_t;

// Struct for assignment node
typedef struct {
	AST_Variable *variable;
	assignment_type_t type;
	union {
		// If type==AS_EXPRESSION
		AST_Expression *expression;
		// If type==AS_STRING
		char *string;
	};
} AST_Assignment;


// Forward declaration of structs
typedef struct statements AST_Statements;
typedef struct statement AST_Statement;

// Struct for goto, connecting a goto with a statement
// or a statement list (if variable != NULL)
typedef struct {
	decl_t *variable;
	union {
		AST_Statement *statement;
		AST_Statements *statement_list;
	};
} AST_Goto;

typedef enum {ASSIGNMENT=0, GOTO, IF, CALL_SUBROUTINE, IO, CONTINUE, RETURN, STOP}
simple_statement_type_t;

// Struct for simple statement node
typedef struct {
	simple_statement_type_t type;
	union {
		// If type==ASSIGNMENT
		AST_Assignment *assignment;
		// If type==GOTO
		AST_Goto *goto_statement;
	};
}  AST_SimpleStatement;

typedef void * AST_CompoundStatement;

typedef enum {SIMPLE, COMPOUND} statement_type_t;

// Wrapper for statements that combines
// simple and compound statements in a struct
struct statement {
	statement_type_t type;
	int statement_id; // Just for prints

	union {
		AST_SimpleStatement *simple;
		AST_CompoundStatement *compound;
	};
	struct statement *next;
};

// Serial list of statements
struct statements {
	AST_Statement *head;
	AST_Statement *tail;
};

// Struct containing the information from a label usage by a goto/if-statament
typedef struct {
	int min_scope; // The used label can be found in the interval [min_scope, 0] of scopes
	int label; // The actual integer label
	AST_Statement **statement_addr; // The address of the statement reference inside the goto/if-statement AST node
} label_use_t;

// An array of label uses for keeping all goto/if-statament references to currently undefined labels
typedef struct {
	int size;
	label_use_t **elements;
} label_uses_t;

typedef struct {
	AST_Expression *expr; // The unmatched expression
	uint8_t possible_types; // All valid types described by the type_t enum 
	bool is_subroutine; // Flag which constrains the variable to be a subroutine
} unmatched_expr_use_t;

typedef struct {
	int size;
	unmatched_expr_use_t **elements;
} unmatched_expr_uses_t;
 

/** Program structs **/

typedef enum {SUBROUTINE=0, FUNCTION} subprogram_type_t;

// Struct for parameters of a function
typedef struct {
	int size;
	decl_t **elements;
} AST_Params;

// Struct for the header of a subprogram
typedef struct {
	subprogram_type_t subprogram_type;
	bool returns_list;
	type_t ret_type;
	AST_Params *params;
	char *id;
} AST_Header;

// Struct consisting of the declarations and statements of AST_Body
typedef struct {
	AST_Decls *declarations;
	AST_Statements *statements;
} AST_Body;

// Subprogram with the header and body of a subprogram
struct subprogram {
	AST_Header *header;
	AST_Body *body;
};

// Array of subprograms packed with its size
typedef struct {
	int size;
	AST_Subprogram **elements;
} AST_Subprograms;

// Struct for keeping track of a program with a main
// and an arbitrary number of subprograms
typedef struct {
	AST_Body *main;
	AST_Subprograms *subprograms;
} AST_Program;



/****************************************************/
/********************* FUNCTIONS ********************/
/****************************************************/
void *safe_malloc(size_t);
void *safe_realloc(void *, size_t);

// Declarations Functions
AST_Constant *ast_get_ICONST(int);
AST_Constant *ast_get_RCONST(double);
AST_Constant *ast_get_CCONST(char);
AST_Constant *ast_get_LCONST(bool);
AST_Constant *ast_get_CMPLX(double, AST_Sign, double);
AST_Sign ast_get_sign(AST_Sign);
AST_Constant *ast_get_value(AST_Sign, AST_Constant *);
AST_Constant *ast_get_string(char *);
AST_Values *ast_insert_value_to_values(AST_Values *, AST_Constant *);
AST_Vals *ast_insert_val_to_vals(AST_Vals *, char *, AST_Values *);
AST_Dim *ast_get_dim_literal(int);
AST_Dim *ast_get_dim_variable(char *);
AST_Dims *ast_insert_dim_to_dims(AST_Dims *, AST_Dim *);
AST_UndefVar *ast_get_undef_var(AST_UndefVar_Type, char *, AST_Dims *, AST_UndefVar *);
AST_Vars *ast_insert_var_to_vars(AST_Vars *, AST_UndefVar *);
AST_Fields *ast_insert_field_to_fields(AST_Fields *, AST_Field *);
AST_Field *ast_get_field(type_t, AST_Vars *, AST_Fields *);
AST_Decls *ast_insert_decl_to_decls(AST_Decls *, type_t, AST_Fields *, AST_Vars *);
void ast_insert_init_in_decls(AST_Vals *);

// Statements Functions
void match_labels_to_label_uses();
AST_Goto *ast_get_independent_goto(int);
AST_Statements *ast_insert_statement_to_statements(AST_Statements *, AST_Statement *);
AST_Statement *ast_get_statement(statement_type_t, void *);
AST_SimpleStatement *ast_get_simple_statement(simple_statement_type_t, void *);
void ast_check_params(AST_Params *params);

// Assign
AST_Assignment *ast_get_assignment_expression(AST_Variable *variable, AST_Expression *expression);
AST_Assignment *ast_get_assignment_string(AST_Variable *variable, char *string);

// Expression functions
AST_Expression *ast_get_expression_var(AST_Variable *variable);
AST_Expression *ast_get_constant_expr(AST_Constant *constant);

AST_Expression *ast_get_expression_unary(unary_op_t op_type, AST_Expression *child);
AST_Expression *ast_get_expression_unary_notop(AST_Expression *child);
AST_Expression *ast_get_expression_unary_addop(AST_Expression *child, AST_Sign sign);
AST_Expression *ast_get_expression_unary_length(AST_Expression *child);
AST_Expression *ast_get_expression_unary_new(AST_Expression *child);

AST_Expression *ast_get_expression_binary(binary_op_t operation, AST_Expression *child1, AST_Expression *child2);
AST_Expression *ast_get_expression_binary_orop(AST_Expression *child1, AST_Expression *child2);
AST_Expression *ast_get_expression_binary_andop(AST_Expression *child1, AST_Expression *child2);
AST_Expression *ast_get_expression_binary_relop(AST_Relop op, AST_Expression *child1, AST_Expression *child2);
AST_Expression *ast_get_expression_binary_addop(AST_Sign sign, AST_Expression *child1, AST_Expression *child2);
AST_Expression *ast_get_expression_binary_mulop(AST_Expression *child1, AST_Expression *child2);
AST_Expression *ast_get_expression_binary_divop(AST_Expression *child1, AST_Expression *child2);
AST_Expression *ast_get_expression_binary_pwrop(AST_Expression *child1, AST_Expression *child2);
AST_Expression *ast_get_expression_binary_cmplx(AST_Expression *child1, AST_Expression *child2);

AST_Expression *ast_get_listexpression(AST_Expressions *exprs);

AST_Expressions *ast_insert_expression_to_expressions(AST_Expressions *exprs,
													AST_Expression *expr);

// Variable
AST_Variable *ast_get_variable_rec_access(AST_Variable *rec, char *field_id);
AST_Variable *ast_get_variable_array_access(AST_Variable *variable, AST_Expressions *exprs);
AST_Variable *ast_get_variable_id(char *id);
AST_Variable *ast_get_variable_listfunc(AST_Listfunc *listfunc, AST_Expression *list);

// Program Functions
AST_Params *ast_insert_param_to_params(AST_Params *, type_t, AST_Vars *);
AST_Header *ast_get_header(subprogram_type_t , type_t, bool, char *, AST_Params *);
AST_Subprogram *ast_get_subprogram(AST_Header *, AST_Body *);
AST_Subprograms *ast_insert_subprogram_to_subprograms(AST_Subprograms *, AST_Subprogram *);
AST_Body *ast_get_body(AST_Decls *, AST_Statements *);
AST_Program *ast_get_program(AST_Body *, AST_Subprograms *);

// Print Functions
void ast_print_func_call(AST_Header *header, AST_Expressions *args, char *tabs);
void ast_print_array_access(AST_Variable *variable, char *tabs);
void ast_print_variable_access(AST_Variable *variable, char *tabs);

void ast_print_datatype(AST_GeneralType *datatype, char *tabs);
void ast_print_undefVar(AST_UndefVar *variable);
void ast_print_dims(AST_Dims *dims);
void ast_print_fields(AST_Fields *fields, char *tabs);
void ast_print_constant(AST_Constant* constant, char *tabs);
void ast_print_initial_value(AST_Values *values, char *tabs);
void ast_print_decl(decl_t *decl, char *tabs);
void ast_print_decls(AST_Decls *decls, char *tabs);

void ast_print_statements(AST_Statements *statements, char *tabs);
void ast_print_simple_statement(AST_SimpleStatement *statement, char  *tabs);
void ast_print_expressions(AST_Expressions *listexpr, char *tabs);
void ast_print_expression(AST_Expression *expression, char *tabs);
void ast_print_compound_statement(AST_CompoundStatement *statement, char  *tabs);
void ast_print_body(AST_Body *, char *);
void ast_print_subprogram(AST_Subprogram *);
void ast_print_header(AST_Header *header);
void ast_print_values(AST_Values *);

void impose_constraint_recursively(AST_Expression *expr, uint8_t possible_types);
void match_funcs_to_unmatched_exprs();

#endif