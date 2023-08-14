/****************************************************/
/************** AST IMPLEMENTATION ******************/
/****************************************************/
#include "ast.h"
#include "semantic.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

// Malloc with safety
void *safe_malloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL) {
		perror("Internal error.");
		exit(EXIT_FAILURE);
	}
	return ptr;
}

// Malloc with safety
void *safe_realloc(void *ptr, size_t size)
{
	void *new_ptr = realloc(ptr,size);
	if (new_ptr == NULL) {
		perror("Internal error.");
		exit(EXIT_FAILURE);
	}
	return new_ptr;
}

/****************************************************/
/************** FUNCTIONS FOR CONSTANTS *************/
/****************************************************/


/****** Functions for Constants ******/

// Create ast node for integer constant
AST_Constant *ast_get_ICONST(int val)
{
	AST_Constant *ret_val = safe_malloc(sizeof(AST_Constant));
	ret_val->intval = val;
	ret_val->type = INT;

	return ret_val;
}

// Create ast node for real constant
AST_Constant *ast_get_RCONST(double val)
{
	AST_Constant *ret_val = safe_malloc(sizeof(AST_Constant));
	ret_val->rval = val;
	ret_val->type = REAL;

	return ret_val;
}

// Create ast node for character constant
AST_Constant *ast_get_CCONST(char val)
{
	AST_Constant *ret_val = safe_malloc(sizeof(AST_Constant));
	ret_val->charval = val;
	ret_val->type = CHAR;

	return ret_val;
}

// Create ast node for logical constant
AST_Constant *ast_get_LCONST(bool val)
{
	AST_Constant *ret_val = safe_malloc(sizeof(AST_Constant));
	ret_val->lval = val;
	ret_val->type = LOG;

	return ret_val;
}

// Create ast node for logical constant
AST_Constant *ast_get_CMPLX(double re, AST_Sign im_sign, double im)
{
	AST_Constant *ret_val = safe_malloc(sizeof(AST_Constant));
	ret_val->cmplxval.re = re;
	if (im_sign != NONE) {
		ret_val->cmplxval.im = im * im_sign;
	}
	else {
		ret_val->cmplxval.im = im;
	}
	ret_val->type = CMPLX;

	return ret_val;
}

/****** Functions for Sign ******/

// Create ast node for sign
AST_Sign ast_get_sign(AST_Sign sign)
{
	return sign;
}

// Create ast node for constant value
AST_Constant *ast_get_value(AST_Sign sign, AST_Constant *constant)
{
	// Check if the constant and the sign are valid
	SEM_signable_constant(sign, constant);
	
	// Update the constant struct if the sign is minus
	if (sign == MINUS) {
		if (constant->type == INT) {
			constant->intval = -constant->intval;
		}
		else if (constant->type == REAL) {
			constant->rval = -constant->rval;
		}
		else {
			constant->cmplxval.re = -constant->cmplxval.re;
			constant->cmplxval.im = -constant->cmplxval.im;
		}
	}

	return constant;
}

// Create ast node for string value
AST_Constant *ast_get_string(char *strval)
{
	AST_Constant *ret_val = safe_malloc(sizeof(AST_Constant));
	ret_val->strval = strval;
	ret_val->type = STR;
	
	return ret_val;
}

AST_Values *ast_insert_value_to_values(AST_Values *values, AST_Constant *value)
{
	AST_Values *new_values;

	if (values == NULL) {
		// Case of the first value
		new_values = safe_malloc(sizeof(AST_Values));
		new_values->size = 1;
		new_values->elements = NULL;
	}
	else {
		// Case of the other values
		new_values = values;
		new_values->size++;
	}

	// Extend the array with the new element
	new_values->elements = safe_realloc(new_values->elements, new_values->size * sizeof(AST_Constant *));
	new_values->elements[new_values->size - 1] = value;

	return new_values;
}

AST_Vals *ast_insert_val_to_vals(AST_Vals *vals, char *id, AST_Values *value_list)
{
	AST_Vals *new_vals;

	// Use hashtbl utilities to check if the variable has been declared
	// SEM_declaration_check(char *id);

	if(vals == NULL) {
		// Case of the first value
		new_vals = safe_malloc(sizeof(AST_Vals));
		new_vals->size = 1;
		new_vals->elements = NULL;
	}
	else {
		// Case of the other values
		new_vals = vals;
		new_vals->size++;
	}
	
	// Extend pointer array
	new_vals->elements = safe_realloc(new_vals->elements, new_vals->size * sizeof(init_val_t *));
	// Allocate memory for InitVal struct
	new_vals->elements[new_vals->size - 1] = safe_malloc(sizeof(init_val_t));
	
	// Update the struct fields
	new_vals->elements[new_vals->size - 1]->id = id;
	new_vals->elements[new_vals->size - 1]->value_list = value_list;

	return new_vals;
}


AST_Dims *ast_insert_dim_to_dims(AST_Dims *dims, int dim)
{
	AST_Dims *new_dims;

	if (dims == NULL) {
		// Case of the first value
		new_dims = safe_malloc(sizeof(AST_Dims));
		new_dims->size = 1;
		new_dims->elements = NULL;
	}
	else {
		// Case of the other values
		new_dims = dims;
		new_dims->size++;
	}

	// Extend the array with the new element
	new_dims->elements = safe_realloc(new_dims->elements, new_dims->size * sizeof(int));
	new_dims->elements[new_dims->size - 1] = dim;

	return new_dims;
}

// Create AST node for an undefined variable
AST_UndefVar *ast_get_undef_var(AST_UndefVar_Type type, char *id, AST_Dims *dims, AST_UndefVar *nested_undef_var)
{
	AST_UndefVar *ret_val;
	if (type == LIST) {
		ret_val = nested_undef_var;
		assert(id == NULL);
		ret_val->list_depth++;
	}
	else {
		ret_val = safe_malloc(sizeof(AST_UndefVar));
		assert(nested_undef_var == NULL);
		ret_val->id = id;
		ret_val->list_depth = 0;
	}

	ret_val->type = type;
	ret_val->dims = dims;

	return ret_val;
}

// Create AST node for Vars if it didn't exist or add an UndefVar to it
AST_Vars *ast_insert_var_to_vars(AST_Vars *vars, AST_UndefVar *var)
{
	AST_Vars *new_vars;

	if (vars == NULL) {
		// Case of the first value
		new_vars = safe_malloc(sizeof(AST_Vars));
		new_vars->size = 1;
		new_vars->elements = NULL;
	}
	else {
		// Case of the other values
		new_vars = vars;
		new_vars->size++;
	}

	// Extend the array with the new element
	new_vars->elements = safe_realloc(new_vars->elements, new_vars->size * sizeof(AST_UndefVar *));
	new_vars->elements[new_vars->size - 1] = var;

	return new_vars;
}

// Create AST node for fields if it didn't exist or add a field to it
AST_Fields *ast_insert_field_to_fields(AST_Fields *fields, AST_Field *field)
{
	AST_Fields *new_fields;

	if (fields == NULL) {
		// Case of the first value
		new_fields = safe_malloc(sizeof(AST_Fields));
		new_fields->size = 1;
		new_fields->elements = NULL;
	}
	else {
		// Case of the other values
		new_fields = fields;
		new_fields->size++;
	}

	// Extend the array with the new element
	new_fields->elements = safe_realloc(new_fields->elements, new_fields->size * sizeof(AST_Field *));
	new_fields->elements[new_fields->size - 1] = field;

	return new_fields;
}

// Get an ast node for a record's field
AST_Field *ast_get_field(type_t type, AST_Vars *vars, AST_Fields *fields)
{
	AST_Field *ret_val = safe_malloc(sizeof(AST_Field));
	ret_val->type = type;
	ret_val->vars = vars;
	
	// if field is a record it contains subfields that reside in fields
	if (type == REC && fields != NULL) {
		ret_val->size = fields->size;
		ret_val->fields = fields->elements;
		free(fields);
	}
	else {
		ret_val->size = 0;
		ret_val->fields = NULL;
	}

	return ret_val;
}

// Given an AST_Vars struct break it in 
// several devl_t structs, one for each id
AST_Decls *ast_insert_decl_to_decls(AST_Decls *old_decls, type_t type, AST_Fields *fields, AST_Vars *vars)
{
	AST_Decls *new_decls;
	char *id;
	if (old_decls == NULL) {
		new_decls = safe_malloc(sizeof(AST_Decls));
		new_decls->size = 0;
		new_decls->declarations = NULL;
	}
	else{
		new_decls = old_decls;
	}

	// Extend the declarations by the number of ids in vars
	int old_size = new_decls->size;
	new_decls->size += vars->size;
	new_decls->declarations = safe_realloc(new_decls->declarations, new_decls->size * sizeof(decl_t *));
	
	for (int i = 0; i < vars->size; i++) {
		new_decls->declarations[old_size + i] = safe_malloc(sizeof(decl_t));
		new_decls->declarations[old_size + i]->datatype = safe_malloc(sizeof(AST_GeneralType));
		new_decls->declarations[old_size + i]->datatype->type = type;
		new_decls->declarations[old_size + i]->datatype->fields = fields;
		new_decls->declarations[old_size + i]->variable = vars->elements[i];
		
		// Insert in symbol table
		id = new_decls->declarations[old_size + i]->variable->id;
		stbl_insert_variable(id, new_decls->declarations[old_size + i]);
	}

	return new_decls;
}

// Insert initialization value of variable to its declaration struct
void ast_insert_init_in_decls(AST_Vals *vals)
{
	init_val_t *curr_val;
	AST_Values *curr_value_list;
	char *curr_id;
	decl_t *entry;
	int error;

	for(int i = 0; i < vals->size; i++){
		curr_val = vals->elements[i];
		curr_id = curr_val->id;
		curr_value_list = curr_val->value_list;
		
		// Find decl of variable from symbol table
		entry = stbl_search_variable(curr_id);
		error = SEM_check_existing_variable(entry, curr_id);
		
		// Function not implemented
		if (!error)
			error =	SEM_check_compatible_initialization(entry->datatype, curr_value_list);
		if (!error)
			entry->initial_value = curr_value_list;
	}
}


void ast_print_decls(AST_Decls *)
{
	return;
}


// Print an AST_Values structure
void ast_print_values(AST_Values *values)
{
	printf("#-----Values-----#\n");
	for (int i = 0; i < values->size; i++) {
		AST_Constant *constant = values->elements[i];
		switch (constant->type) {
			case INT:
				printf("Integer value %d\n", constant->intval); break;
			case LOG:
				printf("Logical value %s\n", (constant->lval) ? "True" : "False"); break;
			case REAL:
				printf("Real value %lf\n", constant->rval); break;
			case CHAR:
				printf("Character value %c\n", constant->charval); break;
			case STR:
				printf("String value %s\n", constant->strval); break;
			case CMPLX:
				printf("Complex value with Re = %lf and Im = %lf\n",
					constant->cmplxval.re, constant->cmplxval.im); break;
			default:
				printf("Value of unknown type\n");
		}
	}
	printf("#------------------#\n");
}