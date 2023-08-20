
#ifndef _AST_H_
#define _AST_H_

#include <stdbool.h>
#include <stddef.h>

/****************************************************/
/********************** STRUCTS *********************/
/****************************************************/

// Value representing the type of an expression
typedef enum {
	INT=0, LOG, REAL, CHAR, STR, CMPLX, REC
} type_t;

// Values representing the op used as sign or addop
typedef enum {MINUS=-1, NONE=0, PLUS=1} AST_Sign;

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

// Struct containing info for a dim array
typedef struct {
	int size;
	int *elements;
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

// Struct containing information about a field.
// When a field is a record, it holds an array of its subfields
typedef struct field {
	AST_Vars *vars; // Variables with that type
	/* The type */
	type_t type;
	int size;
	struct field **fields;
} AST_Field;

// Struct containing an array of fields
typedef struct {
	int size;
	AST_Field **elements;
} AST_Fields;

// General type that covers all data types
typedef struct {
	type_t type;
	AST_Fields *fields; // Only if type == REC
} AST_GeneralType;

// Struct for the declaration of a single id
typedef struct {
	AST_UndefVar *variable; // Many variables (SCALAR, ARRAY or LIST)
	AST_GeneralType *datatype; // The common datatype of all vars
	AST_Values *initial_value; // Same index as vars
	bool is_parameter;
} decl_t;

// Struct for declarations tree
typedef struct {
	int size;
	decl_t **declarations;
} AST_Decls;


typedef void * AST_Statements;


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
typedef struct {
	AST_Header *header;
	AST_Body *body;
} AST_Subprogram;

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
AST_Dims *ast_insert_dim_to_dims(AST_Dims *, int);
AST_UndefVar *ast_get_undef_var(AST_UndefVar_Type, char *, AST_Dims *, AST_UndefVar *);
AST_Vars *ast_insert_var_to_vars(AST_Vars *, AST_UndefVar *);
AST_Fields *ast_insert_field_to_fields(AST_Fields *, AST_Field *);
AST_Field *ast_get_field(type_t, AST_Vars *, AST_Fields *);
AST_Decls *ast_insert_decl_to_decls(AST_Decls *, type_t, AST_Fields *, AST_Vars *);
void ast_insert_init_in_decls(AST_Vals *);


// Program Functions
AST_Params *ast_insert_param_to_params(AST_Params *old_params, type_t type, AST_Vars *vars);
AST_Header *ast_get_header(subprogram_type_t subprogram_type, type_t type, bool is_list, char *id, AST_Params *params);
AST_Subprogram *ast_get_subprogram(AST_Header *header, AST_Body *body);
AST_Subprograms *ast_insert_subprogram_to_subprograms(AST_Subprograms *subprograms, AST_Subprogram *subprogram);
AST_Body *ast_get_body(AST_Decls *decls, AST_Statements *statements);
AST_Program *ast_get_program(AST_Body *main, AST_Subprograms *subprograms);



// Print Functions
void ast_print_values(AST_Values *);
void ast_print_subprogram(AST_Subprogram *subgprogram);


#endif