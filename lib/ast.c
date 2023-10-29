/****************************************************/
/************** AST IMPLEMENTATION ******************/
/****************************************************/
#include "ast.h"
#include "semantic.h"
#include "symbol_table.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

/***** Globals *****/

// Used by the ast node generation to identify if it
// currently generates the body of a certain function
char *func_id = NULL;

int statement_counter = 0;

// Global array with all references to a label
label_uses_t label_uses = { .size = 0, .elements = NULL };
unmatched_expr_uses_t unmatched_expr_uses = { .size = 0, .elements = NULL };

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
/********************* FUNCTIONS ********************/
/****************************************************/


/****** Functions for Declarations ******/

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
	new_values->elements = safe_realloc(new_values->elements,
										new_values->size * sizeof(AST_Constant *));
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
	new_vals->elements = safe_realloc(new_vals->elements,
										new_vals->size * sizeof(init_val_t *));
	// Allocate memory for InitVal struct
	new_vals->elements[new_vals->size - 1] = safe_malloc(sizeof(init_val_t));
	
	// Update the struct fields
	new_vals->elements[new_vals->size - 1]->id = id;
	new_vals->elements[new_vals->size - 1]->value_list = value_list;

	return new_vals;
}

// Create dim struct for literal value of dim
AST_Dim *ast_get_dim_literal(int val)
{
	AST_Dim *dim = safe_malloc(sizeof(AST_Dim));
	dim->type = DIM_LITERAL;
	dim->val = val;
	return dim;
}

// Create dim struct for variable dim
AST_Dim *ast_get_dim_variable(char *id)
{
	AST_Dim *dim = safe_malloc(sizeof(AST_Dim));
	dim->type = DIM_VARIABLE;
	dim->id = id;

	return dim;
}

// Create or append to dimensions
AST_Dims *ast_insert_dim_to_dims(AST_Dims *dims, AST_Dim *dim)
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
	new_dims->elements = safe_realloc(new_dims->elements,
									new_dims->size * sizeof(AST_Dim *));
	new_dims->elements[new_dims->size - 1] = dim;

	return new_dims;
}

// Create AST node for an undefined variable
AST_UndefVar *ast_get_undef_var(AST_UndefVar_Type type, char *id, AST_Dims *dims,
													AST_UndefVar *nested_undef_var)
{
	AST_UndefVar *ret_val;
	if (type == LIST) {
		ret_val = nested_undef_var;
		assert(id == NULL);
		ret_val->list_depth++;
		SEM_check_list_depth(ret_val);
	}
	else {
		ret_val = safe_malloc(sizeof(AST_UndefVar));
		assert(nested_undef_var == NULL);
		ret_val->id = id;
		ret_val->list_depth = 0;
		ret_val->type = type;
		ret_val->dims = dims;
	}


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
	new_vars->elements = safe_realloc(new_vars->elements,
									new_vars->size * sizeof(AST_UndefVar *));
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
	new_fields->elements = safe_realloc(new_fields->elements,
										new_fields->size * sizeof(AST_Field *));
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
		ret_val->fields = fields;
	}
	else {
		ret_val->fields = NULL;
	}

	return ret_val;
}

// Given an AST_Vars struct break it in 
// several devl_t structs, one for each id
AST_Decls *ast_insert_decl_to_decls(AST_Decls *old_decls, type_t type,
									AST_Fields *fields, AST_Vars *vars)
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
	
	new_decls->declarations = safe_realloc(new_decls->declarations,
										new_decls->size * sizeof(decl_t *));
	
	for (int i = 0; i < vars->size; i++) {
		SEM_check_duplicate_variable_name(vars->elements[i]->id);
		
		decl_t *new_decl = safe_malloc(sizeof(decl_t));
		new_decl->is_parameter = false;
		new_decl->datatype = safe_malloc(sizeof(AST_GeneralType));
		new_decl->datatype->type = type;
		new_decl->datatype->fields = fields;
		new_decl->variable = vars->elements[i];
		new_decl->initial_value = NULL;

		AST_Dims *dims = new_decl->variable->dims;
		if (dims != NULL) {
			SEM_check_declared_dims(dims, false);
		}

		// Insert in symbol table
		id = new_decl->variable->id;
		stbl_insert_variable(id, new_decl);
		
		new_decls->declarations[old_size + i] = new_decl;
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

	for(int i = 0; i < vals->size; i++) {
		curr_val = vals->elements[i];
		curr_id = curr_val->id;
		curr_value_list = curr_val->value_list;
		
		// Find decl of variable from symbol table
		entry = stbl_search_variable(curr_id);
		SEM_check_existing_variable(entry, curr_id);
		
		// TODO:Function not implemented
		// SEM_check_compatible_initialization(entry, curr_value_list);
		entry->initial_value = curr_value_list;
	}
}

/****** Functions for Statements ******/

// Insert label_use to label_uses array
void insert_label_use(int label, AST_Statement **statement_addr)
{
	// Create space in label_uses for new label_use
	label_uses.size++;
	label_uses.elements = safe_realloc(label_uses.elements,
									label_uses.size * sizeof(label_use_t *));
	
	// Create new label_use struct
	label_use_t *new_label_use = safe_malloc(sizeof(label_use_t));
	new_label_use->min_scope = stbl_get_curr_scope();
	new_label_use->label = label;

	// Store address of pointer
	new_label_use->statement_addr = statement_addr;

	// Assign new label_use to label_uses array last element
	label_uses.elements[label_uses.size-1] = new_label_use;

	return;
}

// When a label_use is matched to a label, delete it
void remove_label_use(int index)
{
	assert(index < label_uses.size);

	label_uses.size--;
	free(label_uses.elements[index]);
	label_uses.elements[index] = label_uses.elements[label_uses.size];
	
	if (label_uses.size > 0) {
		label_uses.elements = safe_realloc(label_uses.elements,
										label_uses.size * sizeof(label_use_t *));
	}
	else {
		free(label_uses.elements);
		label_uses.elements = NULL;
	}
}

// When closing a scope, match label uses that were created
// in that scope to labels. Otherwise reduce the min scope
// variable so they cannot be matched with labels in other 
// cousin scopes [(children of)^n (parent of)^n-1 parent]
void match_labels_to_label_uses()
{
	int curr_scope = stbl_get_curr_scope();

	for (int i = 0; i < label_uses.size; i++) {
		label_use_t *label_use = label_uses.elements[i];
		if (label_use->min_scope == curr_scope) {
			label_t *label = stbl_search_label_in_current_scope(label_use->label);

			if (label == NULL) {
				label_use->min_scope--;
			}
			else {
				*(label_use->statement_addr) = label->next_statement;
				remove_label_use(i);
			}
		}
	}
}

// Create struct for a goto with a single label
AST_Goto *ast_get_independent_goto(int label)
{
	AST_Goto *goto_statement = safe_malloc(sizeof(AST_Goto));
	goto_statement->variable = NULL;

	// Insert the goto statement as a label_use with the statement to be filled later
	insert_label_use(label, &(goto_statement->statement));
	return goto_statement;
}

// AST_Goto *ast_get_computed_goto(char *id, int *labels)
// {
// 	AST_Goto *goto_statement = safe_malloc(sizeof(AST_Goto));
// 	goto_statement->variable = stbl_search_variable();
// 	return NULL;
// }


// Create the struct of statement
AST_Statement *ast_get_statement(statement_type_t type, void *ptr)
{
	AST_Statement *stmt = safe_malloc(sizeof(AST_Statement));
	stmt->statement_id = statement_counter++;
	stmt->type = type;
	stmt->next = NULL;


	if (type == SIMPLE) {
		stmt->simple = ptr;
	}
	else {
		stmt->compound = ptr;
	}

	return stmt;
}

// Create AST node for Statements if it didn't exist or add a Statement to it
AST_Statements *ast_insert_statement_to_statements(AST_Statements *stmts,
													AST_Statement *stmt)
{
	AST_Statements *new_stmts;

	if (stmts == NULL) {
		// Case of the first value
		new_stmts = safe_malloc(sizeof(AST_Statements));
		new_stmts->head = stmt;
		new_stmts->tail = stmt;
	}
	else {
		// Case of the other values
		new_stmts = stmts;

		new_stmts->tail->next = stmt;
		new_stmts->tail = stmt;
	}

	return new_stmts;
}

// Create the struct of a simple statement
AST_SimpleStatement *ast_get_simple_statement(simple_statement_type_t type, void *ptr)
{
	AST_SimpleStatement *stmt = safe_malloc(sizeof(AST_SimpleStatement));
	stmt->type = type;

	switch (type)
	{
		case ASSIGNMENT:
			stmt->assignment = (AST_Assignment *) ptr;
		case GOTO:
			stmt->goto_statement = (AST_Goto *) ptr;
			break;

		default:
			break;
	}

	return stmt;
}

/* Functions for performing backpatching */

void match_funcs_to_unmatched_exprs()
{
	for (int i = 0; i < unmatched_expr_uses.size; i++) {
		unmatched_expr_use_t *expr_use = unmatched_expr_uses.elements[i];
		if (expr_use->expr->expr_type == EXPR_BINARY) {
			AST_Expression *child1 = expr_use->expr->binary.child1;
			AST_Expression *child2 = expr_use->expr->binary.child2;

			assert(child1->datatype->type != AMBIGUOUS && child2->datatype->type != AMBIGUOUS);

			switch (expr_use->expr->binary.op)
			{
			case B_PLUS:
			case B_MINUS:
			case B_MUL:
			case B_DIV:
				if (child1->datatype->type == CMPLX || child2->datatype->type == CMPLX) {
					SEM_check_possible_types(expr_use, 	CMPLX);
					expr_use->expr->datatype->type = CMPLX;
				}
				else if (child1->datatype->type == REAL || child2->datatype->type == REAL){
					SEM_check_possible_types(expr_use, REAL);
					expr_use->expr->datatype->type = REAL;
				}
				else {
					SEM_check_possible_types(expr_use, INT);
					expr_use->expr->datatype->type = INT;
				}

				break;
			case B_POWER:
				if (child1->datatype->type == INT) {
					SEM_check_possible_types(expr_use, child2->datatype->type);
					expr_use->expr->datatype->type = child2->datatype->type;
				}
				else if (child1->datatype->type == REAL) {
					expr_use->expr->datatype->type = REAL;
				}
				else {
					SEM_check_expr_datatype(child2, O_REAL, false);
					expr_use->expr->datatype->type = CMPLX;
				}
			default:
				break;
			}
		}
		else if (expr_use->expr->expr_type == EXPR_UNARY) {
			SEM_check_possible_types(expr_use, expr_use->expr->unary.child->datatype->type);
			expr_use->expr->datatype->type = expr_use->expr->unary.child->datatype->type;
		}
		else if (expr_use->expr->expr_type == EXPR_VARIABLE) {
			if (expr_use->expr->variable->type == V_FUNC_CALL) {
				AST_Subprogram *subprogram = stbl_search_subprogram(expr_use->expr->variable->id);
				SEM_check_subprog_call_exists(subprogram, expr_use->expr->variable->id);
				SEM_check_subprogram_type(expr_use, subprogram);
				// TODO: check if it returns list (subprogram->returns_list)
				SEM_check_possible_types(expr_use, subprogram->header->ret_type);
				SEM_check_func_call_params(subprogram->header->params,
										expr_use->expr->variable->exprs,
										expr_use->expr->variable->id);

				expr_use->expr->variable->datatype->type = subprogram->header->ret_type;
				// TODO: handle the list depth
				expr_use->expr->variable->subprog = subprogram;
				expr_use->expr->variable->args = expr_use->expr->variable->exprs;
			}
			else  if (expr_use->expr->variable->type == V_LISTFUNC) {
				expr_use->expr->datatype->type = expr_use->expr->variable->list->datatype->type;
			}
		}
		else if (expr_use->expr->expr_type == EXPR_LISTEXPR) {
			type_t type = expr_use->expr->listexpr->elements[0]->datatype->type;
			for (int i = 1; i < expr_use->expr->listexpr->size; i++) {
				AST_Expression *expr = expr_use->expr->listexpr->elements[i];
				SEM_check_expr_datatype(expr, type, false);
			}
			expr_use->expr->datatype->type = type;
		}
	}
}

// Insert function call to the table of unmatched expressions
void insert_unmatched_expr_use(AST_Expression *expr, uint8_t possible_types,
							bool is_subroutine)
{
	// Create space in unmatched_expr_uses for new unmatched_expr_use
	unmatched_expr_uses.size++;
	unmatched_expr_uses.elements = safe_realloc(unmatched_expr_uses.elements,
									unmatched_expr_uses.size * sizeof(unmatched_expr_use_t));
									
	// Create new unmatched_expr_use struct
	unmatched_expr_use_t *new_unmatched_expr_use = safe_malloc(sizeof(unmatched_expr_use_t));
	new_unmatched_expr_use->expr = expr;
	new_unmatched_expr_use->possible_types = possible_types;
	new_unmatched_expr_use->is_subroutine = is_subroutine;

	// Insert element into table of unmatched variable
	unmatched_expr_uses.elements[unmatched_expr_uses.size-1] = new_unmatched_expr_use;
}

// Find a function call expression inside the unmatched expressions table
unmatched_expr_use_t *search_unmatched_expr_use(AST_Expression *expr)
{
	for (int i = 0; i < unmatched_expr_uses.size; i++) {
		if (unmatched_expr_uses.elements[i]->expr == expr)
			return unmatched_expr_uses.elements[i];
	}

	return NULL;
}

void impose_constraint_recursively(AST_Expression *expr, uint8_t possible_types)
{
	unmatched_expr_use_t *parent = search_unmatched_expr_use(expr);
	assert(parent != NULL);

	// Compute the intersection of the current and the given possible types
	parent->possible_types = parent->possible_types & possible_types;

	if (expr->expr_type == EXPR_BINARY) {
		AST_Expression *child1 = expr->binary.child1;
		AST_Expression *child2 = expr->binary.child2;
		type_t child1_type = child1->datatype->type;
		type_t child2_type = child2->datatype->type;
		
		switch (expr->binary.op)
		{
		case B_PLUS:
		case B_MINUS:
		case B_MUL:
		case B_DIV:
			if (parent->possible_types == O_INTEGER) {
				if (child1_type != AMBIGUOUS)
					SEM_check_expr_datatype(child1, O_INTEGER, false);
				else
					impose_constraint_recursively(child1, O_INTEGER);
				
				if (child2_type != AMBIGUOUS)
					SEM_check_expr_datatype(child2, O_INTEGER, false);
				else
					impose_constraint_recursively(child2, O_INTEGER);
			}
			else if (parent->possible_types == O_REAL ||
					parent->possible_types == (O_INTEGER | O_REAL)) {
				if (child1_type != AMBIGUOUS)
					SEM_check_expr_datatype(child1, O_INTEGER | O_REAL, false);
				else
					impose_constraint_recursively(child1, O_INTEGER | O_REAL);
				
				if (child2_type != AMBIGUOUS)
					SEM_check_expr_datatype(child2, O_INTEGER | O_REAL, false);
				else
					impose_constraint_recursively(child2, O_INTEGER | O_REAL);
			}
			
			break;
		case B_POWER:
			if (parent->possible_types == O_INTEGER) {
				if (child1_type != AMBIGUOUS)
					SEM_check_expr_datatype(child1, O_INTEGER, false);
				else
					impose_constraint_recursively(child1, O_INTEGER);
				
				if (child2_type != AMBIGUOUS)
					SEM_check_expr_datatype(child2, O_INTEGER, false);
				else
					impose_constraint_recursively(child2, O_INTEGER);
			}
			else if (parent->possible_types == O_REAL) {
				if (child1_type != AMBIGUOUS && child2_type == AMBIGUOUS) {
					SEM_check_expr_datatype(child1, O_INTEGER | O_REAL, false);
					
					if(child1_type == REAL)
						impose_constraint_recursively(child2, O_INTEGER | O_REAL);
					else if(child1_type == INT)
						impose_constraint_recursively(child2, O_REAL);
				}
				else if (child2_type != AMBIGUOUS && child1_type == AMBIGUOUS) {
					SEM_check_expr_datatype(child2, O_INTEGER | O_REAL, false);
					
					if(child2_type == REAL)
						impose_constraint_recursively(child1, O_INTEGER | O_REAL);
					else if(child2_type == INT)
						impose_constraint_recursively(child1, O_REAL);
				}
				else { // Both children are AMBIGUOUS
					impose_constraint_recursively(child1, O_INTEGER | O_REAL);
					impose_constraint_recursively(child2, O_INTEGER | O_REAL);
				}
			}
			else if (parent->possible_types == O_COMPLEX) {
				if (child1_type != AMBIGUOUS)
					SEM_check_expr_datatype(child1, O_COMPLEX, false);
				else
					impose_constraint_recursively(child1, O_COMPLEX);
				
				if (child2_type != AMBIGUOUS)
					SEM_check_expr_datatype(child2, O_INTEGER, false);
				else
					impose_constraint_recursively(child2, O_INTEGER);
			}
			else if (parent->possible_types == (O_INTEGER | O_REAL)) {
				if (child1_type != AMBIGUOUS)
					SEM_check_expr_datatype(child1, O_INTEGER | O_REAL, false);
				else
					impose_constraint_recursively(child1, O_INTEGER | O_REAL);
				
				if (child2_type != AMBIGUOUS)
					SEM_check_expr_datatype(child2, O_INTEGER | O_REAL, false);
				else
					impose_constraint_recursively(child2, O_INTEGER | O_REAL);
			}
			else if (parent->possible_types == (O_INTEGER | O_COMPLEX)) {
				if (child1_type != AMBIGUOUS)
					SEM_check_expr_datatype(child1, O_INTEGER | O_COMPLEX, false);
				else
					impose_constraint_recursively(child1, O_INTEGER | O_COMPLEX);
				
				if (child2_type != AMBIGUOUS)
					SEM_check_expr_datatype(child2, O_INTEGER, false);
				else
					impose_constraint_recursively(child2, O_INTEGER);
			}			
			else if (parent->possible_types == (O_REAL | O_COMPLEX)){
				if (child1_type != AMBIGUOUS && child2_type == AMBIGUOUS) {
					SEM_check_expr_datatype(child1, O_INTEGER | O_REAL, false);
					
					if(child1_type == INT)
						impose_constraint_recursively(child2, O_REAL);
					else if(child1_type == REAL)
						impose_constraint_recursively(child2, O_INTEGER | O_REAL);
					else
						impose_constraint_recursively(child2, O_INTEGER);
				}
				else if (child2_type != AMBIGUOUS && child1_type == AMBIGUOUS) {
					SEM_check_expr_datatype(child2, O_INTEGER | O_REAL, false);
					
					if(child2_type == INT)
						impose_constraint_recursively(child1, O_REAL | O_COMPLEX);
					else if(child2_type == REAL)
						impose_constraint_recursively(child1, O_INTEGER | O_REAL);
				}
			}
			
			break;
		default:
			break;
		}
	}
	else if (expr->expr_type == EXPR_UNARY) {
		assert(expr->unary.child->datatype->type == AMBIGUOUS);
		impose_constraint_recursively(expr->unary.child, parent->possible_types);
	}
	else if (expr->expr_type == EXPR_LISTEXPR) {
		for (int i = 0; i < expr->listexpr->size; i++) {
			if (expr->listexpr->elements[i]->datatype->type != AMBIGUOUS)
				SEM_check_expr_datatype(expr->listexpr->elements[i], parent->possible_types, false);
			else
				impose_constraint_recursively(expr->listexpr->elements[i], parent->possible_types);
		}
	}
	else if (expr->expr_type == EXPR_VARIABLE) {
		if (expr->variable->type == V_LISTFUNC) {
			assert(expr->variable->list->datatype->type == AMBIGUOUS);
			impose_constraint_recursively(expr->variable->list, parent->possible_types);
		}
	}

	return;
}

// Search for an id that is a field in a record and return the field and
// the position/index of the id in the AST_Vars array of the field
AST_Field* ast_search_field_in_record(AST_GeneralType *datatype, char *field_id,
									int *position_in_vars)
{
	assert(datatype->fields != NULL);

	// Search in all fields of RECORD
	for (int i = 0; i < datatype->fields->size; i++) {
		AST_Field *field = datatype->fields->elements[i];

		// Search in all vars of each field
		for (int j = 0; j < field->vars->size; j++) {
			AST_UndefVar *var = field->vars->elements[j];

			// Compare strings
			if (strcmp(var->id, field_id) == 0) {
				// Return the field and the index of the var
				*position_in_vars = j;
				return field;
			}
		}
	}

	return NULL;
}

// Transforms variables from id to decl
int ast_variable_id2decl(AST_Variable *variable, bool can_be_func)
{	
	decl_t *decl = stbl_search_variable(variable->id);

	// Variable has been declared
	if (decl) {
		variable->type = V_DECL;
		variable->decl = decl;
		variable->list_depth = decl->variable->list_depth;
		variable->datatype = decl->datatype;
		variable->dims = decl->variable->dims;

		return 0;
	}
	// Undeclared variable which cannot be a function
	else if (!can_be_func) {
		SEM_check_existing_variable(decl, variable->id);
	}

	return 1;
}

// Create a record access node
AST_Variable *ast_get_variable_rec_access(AST_Variable *rec, char *field_id)
{
	int position_in_vars;
	char *rec_id;
	
	// If the rec is in ID form change it to decl
	if (rec->type == V_ID)
		ast_variable_id2decl(rec, false);

	/* Variable rec cannot be an ID (it should have become a DECL) or a
	   a FUNC_CALL since functions cannot return a record */
	SEM_check_variable_type(rec, O_DECL | O_ARRAY_ACCESS | O_REC_ACCESS | O_LISTFUNC);
	
	// Find the record's id for use in debugging messages
	if (rec->type == V_DECL)
		rec_id = rec->decl->variable->id;
	else if (rec->type == V_REC_ACCESS)
		rec_id = rec->field_var->id;
	else if (rec->type == V_ARRAY_ACCESS)
		rec_id = rec->array->id;
	else // rec->type == V_LISTFUNC
		rec_id = NULL;

	// TODO: We don't need SEM_typecheck_variable(rec, REC, rec_id) as well
	SEM_check_decl_datatype_simple(rec->datatype, REC, rec_id);
	AST_Field* field = ast_search_field_in_record(rec->datatype,
											field_id, &position_in_vars);
	SEM_check_existing_record_field(field, rec_id, field_id);

	// Create new node for new_variable
	AST_Variable *new_variable = safe_malloc(sizeof(AST_Variable));
	
	// Fill the new_variable contents
	new_variable->type = V_REC_ACCESS;
	new_variable->record = rec;

	// Copy datatype of variable
	AST_GeneralType *new_datatype = safe_malloc(sizeof(AST_GeneralType));
	new_datatype->type = field->type;
	new_datatype->fields = field->fields;

	// Store the variable field
	new_variable->field_var = field->vars->elements[position_in_vars];
	new_variable->dims = new_variable->field_var->dims;
	
	new_variable->datatype = new_datatype;
	new_variable->list_depth = new_variable->field_var->list_depth;

	return new_variable;
}


AST_Variable *ast_get_variable_array_access(AST_Variable *variable, AST_Expressions *exprs)
{
	/* Ensure the variable is either an ID (which might be found in the sumbol table)
	   or a record access (which can possibly be an array field) */
	SEM_check_variable_type(variable, O_ID | O_REC_ACCESS);

	// If variable is an ID, try to find an array declaration
	if (variable->type == V_ID) {
		// if there is not an array declaration, it can only be a function call
		if (ast_variable_id2decl(variable, true) != 0) {
			variable->type = V_FUNC_CALL;
			AST_GeneralType *new_datatype = safe_malloc(sizeof(AST_GeneralType));
			new_datatype->type = AMBIGUOUS;
			new_datatype->fields = NULL;
			variable->datatype = new_datatype;
			variable->exprs = exprs;
		}
		else {
			SEM_check_valid_array_access(variable->decl->variable, exprs);
			variable->type = V_ARRAY_ACCESS;
			variable->array = variable->decl->variable;
			variable->parent_rec = NULL;
			variable->indices = exprs;
		}
	}
	else if (variable->type == V_REC_ACCESS) {
		SEM_check_valid_array_access(variable->field_var, exprs);
		AST_Variable *new_variable = safe_malloc(sizeof(AST_Variable));
		new_variable->type = V_ARRAY_ACCESS;
		new_variable->datatype = variable->datatype;
		new_variable->list_depth = variable->list_depth;
		new_variable->dims = variable->dims;
		new_variable->parent_rec = variable;
		new_variable->array = variable->field_var;
		new_variable->indices = exprs;

		return new_variable;
	}

	return variable;
}

// Get a variable struct
AST_Variable *ast_get_variable_id(char *id)
{	
	AST_Variable *new_variable = safe_malloc(sizeof(AST_Variable));
	new_variable->type = V_ID;
	new_variable->datatype = NULL;
	new_variable->list_depth = 0;
	new_variable->dims = NULL;
	new_variable->id = id;
	new_variable->exprs = NULL;
	
	return new_variable;
}

AST_Variable *ast_get_variable_listfunc(AST_Listfunc *listfunc, AST_Expression *list)
{
	AST_Variable *new_variable = safe_malloc(sizeof(AST_Variable));
	new_variable->type = V_LISTFUNC;
	new_variable->datatype = list->datatype;

	SEM_check_expression_list(list);

	if (list->expr_type == EXPR_LISTEXPR) {
		SEM_check_valid_listexpr_hops(list->listexpr, listfunc->hops);
		for (int i = 0; i < listfunc->hops; i++) {
			list->listexpr->elements[i] = list->listexpr->elements[listfunc->hops + i];
		}
		list->listexpr->size -= listfunc->hops;
		list->listexpr->elements = safe_realloc(list->listexpr->elements,
										list->listexpr->size * sizeof(AST_Expression));
	}

	if (listfunc->access == true)
		new_variable->list_depth = list->list_depth - 1;
	else
		new_variable->list_depth = list->list_depth;

	new_variable->listfunc = listfunc;
	new_variable->list = list;
	
	return new_variable;
}


/* Functions for creating expression nodes */

// Create an expression node that is a variable
AST_Expression *ast_get_expression_var(AST_Variable *variable)
{	
	// At this point only scalar variables can be of ID type
	if (variable->type == V_ID)
		ast_variable_id2decl(variable, false);

	AST_Expression *new_expr = safe_malloc(sizeof(AST_Expression));
	new_expr->expr_type = EXPR_VARIABLE;
	new_expr->variable = variable;
	new_expr->datatype = variable->datatype;
	new_expr->list_depth = variable->list_depth;
	new_expr->dims = variable->dims;

	// Do not insert a listfunc that returns an address in the unmatched table
	// TODO: Think of how the concept of addresses will affect the datatype mechanism
	if (variable->type == V_LISTFUNC && variable->listfunc->access == false)
			return new_expr;

	// Any expression that can only be/uses a function call should be stored in the unmatched table
	if (variable->datatype->type == AMBIGUOUS) {
		uint8_t possible_types = O_INTEGER | O_LOGICAL | O_REAL | O_CHARACTER | O_COMPLEX;
		insert_unmatched_expr_use(new_expr, possible_types, false);
	}

	return new_expr;
}

// Get a constant struct
AST_Expression *ast_get_constant_expr(AST_Constant *constant)
{
	AST_Expression *new_expr = safe_malloc(sizeof(AST_Expression));
	new_expr->expr_type = EXPR_CONSTANT;
	new_expr->constant = constant;
	new_expr->datatype = safe_malloc(sizeof(AST_GeneralType));
	new_expr->datatype->type = constant->type;
	new_expr->datatype->fields = NULL;
	new_expr->dims = NULL;

	return new_expr;
}

/* Functions used for typecasting number constants */

/* if expr1 is an integer constant and expr2 is a real number
   typecast expr1 to a real constant */
void ast_typecast_int2real(AST_Expression *expr1, AST_Expression *expr2)
{
	if (expr2->datatype->type == REAL &&
		expr1->datatype->type == INT &&
		expr1->expr_type == EXPR_CONSTANT) {
		expr1->constant->type = REAL;
		expr1->constant->rval = (double) expr1->constant->intval;
	}
}

/* if expr1 is an integer constant and expr2 is a cmplx number
   typecast expr1 to a cmplx constant */
void ast_typecast_int2cmplx(AST_Expression *expr1, AST_Expression *expr2)
{
	if (expr2->datatype->type == CMPLX &&
		expr1->datatype->type == INT &&
		expr1->expr_type == EXPR_CONSTANT) {
		expr1->constant->type = CMPLX;
		expr1->constant->cmplxval.re = (double) expr1->constant->intval;
		expr1->constant->cmplxval.im = 0.0;
	}
}

/* if expr1 is an real constant and expr2 is a cmplx number
   typecast expr1 to a cmplx constant */
void ast_typecast_real2cmplx(AST_Expression *expr1, AST_Expression *expr2)
{
	if (expr2->datatype->type == CMPLX &&
		expr1->datatype->type == REAL &&
		expr1->expr_type == EXPR_CONSTANT) {
		expr1->constant->type = CMPLX;
		expr1->constant->cmplxval.re = expr1->constant->rval;
		expr1->constant->cmplxval.im = 0.0;
	}
}

// Creates an AST node for a unary expression
AST_Expression *ast_get_expression_unary(unary_op_t op_type, AST_Expression *child)
{
	AST_Expression *new_expr = safe_malloc(sizeof(AST_Expression));
	new_expr->expr_type = EXPR_UNARY;
	new_expr->unary.op = op_type;
	new_expr->unary.child = child;

	switch (op_type)
	{
	case U_PLUS:
	case U_MINUS:
		new_expr->datatype = child->datatype;
		new_expr->list_depth = 0;
		if (child->datatype->type != AMBIGUOUS) {
			SEM_check_expression_not_list(child);
			SEM_check_expr_datatype(child, O_INTEGER | O_REAL | O_COMPLEX, true);
		}
		else {
			insert_unmatched_expr_use(new_expr, O_INTEGER | O_REAL | O_COMPLEX, false);
			impose_constraint_recursively(child, O_INTEGER | O_REAL | O_COMPLEX);
		}
		
		break;
	case U_NOT: 
		if (child->datatype->type != AMBIGUOUS) {
			SEM_check_expression_not_list(child);
			SEM_check_expr_datatype(child, O_LOGICAL, true);
			new_expr->datatype = child->datatype;
			new_expr->list_depth = 0;
		}
		else {
			new_expr->datatype = safe_malloc(sizeof(AST_GeneralType));
			new_expr->datatype->type = LOG;
			new_expr->datatype->fields = NULL;
			new_expr->list_depth = 0;
			impose_constraint_recursively(child, O_LOGICAL);
		}
		break;
	case U_LENGTH:
		SEM_check_expression_list(child);
		new_expr->datatype = safe_malloc(sizeof(AST_GeneralType));
		new_expr->datatype->type = INT;
		new_expr->datatype->fields = NULL;
		new_expr->list_depth = 0;
		break;
	case U_NEW:
		new_expr->datatype = child->datatype;
		new_expr->list_depth = child->list_depth + 1;
		if (child->datatype->type == AMBIGUOUS) {
			unmatched_expr_use_t *unmatched_child = search_unmatched_expr_use(child);
			assert(unmatched_child == NULL);
			insert_unmatched_expr_use(new_expr, unmatched_child->possible_types, false);
		}
		break;
	default:
		break;
	}

	return new_expr;
}

// Create unary expression for notop
AST_Expression *ast_get_expression_unary_notop(AST_Expression *child)
{
	return ast_get_expression_unary(U_NOT, child);
}

// Create unary expression for addop
AST_Expression *ast_get_expression_unary_addop(AST_Expression *child, AST_Sign sign)
{
	if (sign == 1)
		return ast_get_expression_unary(U_PLUS, child);
	else
		return ast_get_expression_unary(U_MINUS, child);
}

// Create unary expression for addop
AST_Expression *ast_get_expression_unary_length(AST_Expression *child)
{
	return ast_get_expression_unary(U_LENGTH, child);
}

// Create unary expression for addop
AST_Expression *ast_get_expression_unary_new(AST_Expression *child)
{
	return ast_get_expression_unary(U_NEW, child);
}

// Creates an AST node for a binary expression
AST_Expression *ast_get_expression_binary(binary_op_t operation,
									AST_Expression *child1, AST_Expression *child2)
{
	AST_Expression *new_expr = safe_malloc(sizeof(AST_Expression));
	new_expr->expr_type = EXPR_BINARY;
	new_expr->binary.child1 = child1;
	new_expr->binary.child2 = child2;
	new_expr->binary.op = operation;

	new_expr->datatype = safe_malloc(sizeof(AST_GeneralType));
	new_expr->datatype->fields = NULL;

	SEM_check_expression_not_list(child1);
	SEM_check_expression_not_list(child2);

	switch (operation)
	{
	case B_OR:
	case B_AND:
		if (child1->datatype->type != AMBIGUOUS)
			SEM_check_expr_datatype(child1, O_LOGICAL, true);
		else
			impose_constraint_recursively(child1, O_LOGICAL);

		if (child2->datatype->type != AMBIGUOUS)
			SEM_check_expr_datatype(child2, O_LOGICAL, true);
		else
			impose_constraint_recursively(child2, O_LOGICAL);

		new_expr->datatype->type = LOG;
		break;
	case B_PLUS:
	case B_MINUS:
	case B_MUL:
	case B_DIV:
		// Check valid datatypes of children or impose a new constraint if they are ambiguous
		if (child1->datatype->type != AMBIGUOUS)
			SEM_check_expr_datatype(child1, O_INTEGER | O_REAL | O_COMPLEX, true);
		else
			impose_constraint_recursively(child1, O_INTEGER | O_REAL | O_COMPLEX);

		if (child2->datatype->type != AMBIGUOUS)
			SEM_check_expr_datatype(child2, O_INTEGER | O_REAL | O_COMPLEX, true);
		else
			impose_constraint_recursively(child2, O_INTEGER | O_REAL | O_COMPLEX);


		// Find the new_expr's datatype. Insert it in the unmatched table if it's ambiguous
		if (child1->datatype->type == CMPLX || child2->datatype->type == CMPLX) {
			new_expr->datatype->type = CMPLX;
		}
		else if (child1->datatype->type != AMBIGUOUS && child2->datatype->type != AMBIGUOUS) {
			if (child1->datatype->type == REAL || child2->datatype->type == REAL)
				new_expr->datatype->type = REAL;
			else
				new_expr->datatype->type = INT;
		}
		else {
			new_expr->datatype->type = AMBIGUOUS;
			insert_unmatched_expr_use(new_expr, O_INTEGER | O_REAL | O_COMPLEX, false);
		}

		break;
	case B_POWER:
		new_expr->datatype->type = AMBIGUOUS;

		// Ensure the base is either ambiguous or a number
		if (child1->datatype->type != AMBIGUOUS) {
			SEM_check_expr_datatype(child1, O_INTEGER | O_REAL | O_COMPLEX, true);

			// Examine all type combinations of the children
			if (child1->datatype->type == CMPLX) {
				if (child2->datatype->type != AMBIGUOUS)
					SEM_check_expr_datatype(child2, O_INTEGER, true);
				else
					impose_constraint_recursively(child2, O_INTEGER);

				new_expr->datatype->type = CMPLX;
			}
			else { // child1->datatype->type == INT | REAL
				if (child2->datatype->type != AMBIGUOUS)
					SEM_check_expr_datatype(child2, O_INTEGER | O_REAL, true);
				else
					impose_constraint_recursively(child2, O_INTEGER | O_REAL);

				if (child1->datatype->type == REAL || child2->datatype->type == REAL)
					new_expr->datatype->type = REAL;
				else if (child2->datatype->type == INT)
					insert_unmatched_expr_use(new_expr, O_INTEGER, false);
				else
					insert_unmatched_expr_use(new_expr, O_INTEGER | O_REAL, false);
			}
		}
		else { // child1->datatype->type == AMBIGUOUS
			if (child2->datatype->type != AMBIGUOUS) {
				SEM_check_expr_datatype(child2, O_INTEGER | O_REAL, true);
				if (child2->datatype->type == INT) {
					impose_constraint_recursively(child1, O_INTEGER | O_REAL | O_COMPLEX);
					insert_unmatched_expr_use(new_expr, O_INTEGER | O_REAL | O_COMPLEX, false);	
				}
				else { // child2->datatype->type == REAL
					impose_constraint_recursively(child1, O_INTEGER | O_REAL);
					new_expr->datatype->type = REAL;
				}
			}
			else { // child2->datatype->type == AMBIGUOUS
				impose_constraint_recursively(child1, O_INTEGER | O_REAL | O_COMPLEX);
				impose_constraint_recursively(child2, O_INTEGER | O_REAL);
				insert_unmatched_expr_use(new_expr, O_INTEGER | O_REAL | O_COMPLEX, false);
			}
		}

		break;
	case B_GT:
	case B_GE:
	case B_LT:
	case B_LE:
		// Final expr datatype is always logical
		new_expr->datatype->type = LOG;

		// Check valid pairs of datatypes of children and typecast if necessary
		if (child1->datatype->type != AMBIGUOUS) {
			SEM_check_expr_datatype(child1, O_INTEGER | O_REAL | O_CHARACTER, true);
			
			if (child1->datatype->type == CHAR) {
				if (child2->datatype->type != AMBIGUOUS)
					SEM_check_expr_datatype(child2, O_CHARACTER, true);
				else
					impose_constraint_recursively(child2, O_CHARACTER);
			}
			else {
				if (child2->datatype->type != AMBIGUOUS) {
					SEM_check_expr_datatype(child2, O_INTEGER | O_REAL, true);
					ast_typecast_int2real(child1, child2);
					ast_typecast_int2real(child2, child1);
				}
				else {
					impose_constraint_recursively(child2, O_INTEGER | O_REAL);
				}
			}
		}
		else { // child1->datatype->type == AMBIGUOUS
			if (child2->datatype->type != AMBIGUOUS) {
				SEM_check_expr_datatype(child2, O_INTEGER | O_REAL | O_CHARACTER, true);

				if (child2->datatype->type == CHAR)
					impose_constraint_recursively(child1, O_CHARACTER);
				else
					impose_constraint_recursively(child1, O_INTEGER | O_REAL);
			}
			else {
				impose_constraint_recursively(child1, O_INTEGER | O_REAL | O_CHARACTER);
				impose_constraint_recursively(child2, O_INTEGER | O_REAL | O_CHARACTER);
			}
		}

		break;
	case B_EQ:
	case B_NE:
		// Final expr datatype is always logical
		new_expr->datatype->type = LOG;

		// Check valid pairs of datatypes of children and typecast if necessary
		if (child1->datatype->type != AMBIGUOUS) {
			SEM_check_expr_datatype(child1, O_INTEGER | O_REAL | O_CHARACTER | O_COMPLEX, true);
			
			if (child1->datatype->type == CHAR) {
				if (child2->datatype->type != AMBIGUOUS)
					SEM_check_expr_datatype(child2, O_CHARACTER, true);
				else
					impose_constraint_recursively(child2, O_CHARACTER);
			}
			else {
				if (child2->datatype->type != AMBIGUOUS) {
					SEM_check_expr_datatype(child2, O_INTEGER | O_REAL | O_COMPLEX, true);
					ast_typecast_int2real(child1, child2);
					ast_typecast_int2real(child2, child1);
					ast_typecast_int2cmplx(child1, child2);
					ast_typecast_int2cmplx(child2, child1);
					ast_typecast_real2cmplx(child1, child2);
					ast_typecast_real2cmplx(child2, child1);
				}
				else {
					impose_constraint_recursively(child2, O_INTEGER | O_REAL | O_COMPLEX);
				}
			}
		}
		else { // child1->datatype->type == AMBIGUOUS
			if (child2->datatype->type != AMBIGUOUS) {
				SEM_check_expr_datatype(child2, O_INTEGER | O_REAL | O_CHARACTER | O_COMPLEX, true);

				if (child2->datatype->type == CHAR)
					impose_constraint_recursively(child1, O_CHARACTER);
				else
					impose_constraint_recursively(child1, O_INTEGER | O_REAL | O_COMPLEX);
			}
			else {
				impose_constraint_recursively(child1, O_INTEGER | O_REAL | O_CHARACTER | O_COMPLEX);
				impose_constraint_recursively(child2, O_INTEGER | O_REAL | O_CHARACTER | O_COMPLEX);
			}
		}

		break;
	case B_CMPLX:
		// Final expr datatype is always complex
		new_expr->datatype->type = CMPLX;

		if (child1->datatype->type != AMBIGUOUS && child2->datatype->type != AMBIGUOUS) {
			SEM_check_expr_datatype(child1, O_INTEGER | O_REAL, true);
			SEM_check_expr_datatype(child2, O_INTEGER | O_REAL, true);

			ast_typecast_int2real(child1, child2);
			ast_typecast_int2real(child2, child1);
		}
		else if (child1->datatype->type != AMBIGUOUS) {
			SEM_check_expr_datatype(child1, O_INTEGER | O_REAL, true);
			impose_constraint_recursively(child2, O_INTEGER | O_REAL);
		}
		else if (child2->datatype->type != AMBIGUOUS) {
			SEM_check_expr_datatype(child2, O_INTEGER | O_REAL, true);
			impose_constraint_recursively(child1, O_INTEGER | O_REAL);
		}
		else {
			impose_constraint_recursively(child1, O_INTEGER | O_REAL);
			impose_constraint_recursively(child2, O_INTEGER | O_REAL);
		}

		break;
	default:
		break;
	}

	return new_expr;
}

// Wrapper of ast_get_expression_binary for orop
AST_Expression *ast_get_expression_binary_orop(AST_Expression *child1,
												AST_Expression *child2)
{
	return ast_get_expression_binary(B_OR, child1, child2);
}

// Wrapper of ast_get_expression_binary for andop
AST_Expression *ast_get_expression_binary_andop(AST_Expression *child1,
												AST_Expression *child2)
{	
	return ast_get_expression_binary(B_AND, child1, child2);
}

// Wrapper of ast_get_expression_binary for relop
AST_Expression *ast_get_expression_binary_relop(AST_Relop op,
									AST_Expression *child1, AST_Expression *child2)
{
	return ast_get_expression_binary(B_GT + op, child1, child2);
}

// Wrapper of ast_get_expression_binary for addop
AST_Expression *ast_get_expression_binary_addop(AST_Sign sign,
									AST_Expression *child1, AST_Expression *child2)
{
	if (sign == 1)
		return ast_get_expression_binary(B_PLUS, child1, child2);
	else
		return ast_get_expression_binary(B_MINUS, child1, child2);
}

// Wrapper of ast_get_expression_binary for mulop
AST_Expression *ast_get_expression_binary_mulop(AST_Expression *child1,
												AST_Expression *child2)
{
	return ast_get_expression_binary(B_MUL, child1, child2);
}

// Wrapper of ast_get_expression_binary for divop
AST_Expression *ast_get_expression_binary_divop(AST_Expression *child1,
												AST_Expression *child2)
{
	return ast_get_expression_binary(B_DIV, child1, child2);
}

// Wrapper of ast_get_expression_binary for pwrop
AST_Expression *ast_get_expression_binary_pwrop(AST_Expression *child1,
												AST_Expression *child2)
{
	return ast_get_expression_binary(B_POWER, child1, child2);
}

// Create unary expression for addop
AST_Expression *ast_get_expression_binary_cmplx(AST_Expression *child1,
												AST_Expression *child2)
{
	return ast_get_expression_binary(B_CMPLX, child1, child2);
}

// Create AST node for Expressions if it didn't exist or add a Expression to it
AST_Expressions *ast_insert_expression_to_expressions(AST_Expressions *exprs,
													AST_Expression *expr)
{
	AST_Expressions *new_exprs;

	if (exprs == NULL) {
		// Case of the first value
		new_exprs = safe_malloc(sizeof(AST_Expressions));
		new_exprs->size = 1;
		new_exprs->elements = NULL;
	}
	else {
		// Case of the other values
		new_exprs = exprs;
		new_exprs->size++;
	}

	// Extend the array with the new element
	new_exprs->elements = safe_realloc(new_exprs->elements,
									new_exprs->size * sizeof(AST_Expression *));
	new_exprs->elements[new_exprs->size - 1] = expr;

	return new_exprs;
}

// Create a list initialization
AST_Expression *ast_get_listexpression(AST_Expressions *exprs)
{
	AST_Expression *new_expr = safe_malloc(sizeof(AST_Expression));
	int element_list_depth;

	new_expr->expr_type = EXPR_LISTEXPR;
	new_expr->listexpr = exprs;

	// Case of an empty list
	if (exprs == NULL) {
		new_expr->list_depth = 1;
		new_expr->datatype = NULL;
		return new_expr;
	}

	// The list's datatype is the same as its first non-ambiguous element
	AST_GeneralType *datatype = safe_malloc(sizeof(AST_GeneralType));
	datatype->type = AMBIGUOUS;
	datatype->fields = NULL;
	// TODO: Cover the case of functions that return a list 
	element_list_depth = 0;

	for (int i = 0; i < exprs->size; i++) {
		AST_Expression *expr = exprs->elements[i];
		SEM_check_expression_not_array(expr);

		if (expr->datatype->type != AMBIGUOUS) {
			datatype->type = expr->datatype->type;
			element_list_depth = expr->list_depth;
			break;
		}
	}

	new_expr->list_depth = element_list_depth + 1;
	new_expr->datatype = datatype;

	// For a non-ambiguous list, force every ambiguous element to inherit its datatype
	if (datatype->type != AMBIGUOUS) {
		for (int i = 0; i < exprs->size; i++) {
			AST_Expression *expr = exprs->elements[i];
			SEM_check_expression_not_array(expr);
			SEM_check_same_list_depth(element_list_depth, expr->list_depth);
			
			if (expr->datatype->type != AMBIGUOUS)
				SEM_check_same_datatypes(datatype, expr->datatype);
			else
				impose_constraint_recursively(expr, 1 << datatype->type);
		}
	}
	else { // List is an ambiguous expression, so insert it in the unmatched table
		uint8_t possible_types = O_INTEGER | O_LOGICAL | O_REAL |
								O_CHARACTER | O_COMPLEX | O_RECORD;
		insert_unmatched_expr_use(new_expr, possible_types, false);
	}

	return new_expr;
}

/* Functions for assignments */

// Create an assignment to string
AST_Assignment *ast_get_assignment_expression(AST_Variable *variable,
											AST_Expression *expression)
{
	// TODO: The use of a function's name as a variable inside that function
	// should be handled by maybe making a new variable type... or using V_FUNC_CALL
	if (variable->type == V_ID) {
		if (func_id == NULL || strcmp(func_id, variable->id) != 0)
			ast_variable_id2decl(variable, false);
	}
	
	AST_Assignment *asmt = safe_malloc(sizeof(AST_Assignment));

	asmt->variable = variable;
	asmt->type = AS_EXPRESSION;
	asmt->expression = expression;

	return asmt;
}

// Create an assignment to string
AST_Assignment *ast_get_assignment_string(AST_Variable *variable, char *string)
{
	if (variable->type == V_ID)
		ast_variable_id2decl(variable, false);
		
	SEM_typecheck_variable(variable, STR, "Left hand of assignment");

	AST_Assignment *asmt = safe_malloc(sizeof(AST_Assignment));
	
	asmt->variable = variable;
	asmt->type = AS_STRING;
	asmt->string = string;

	return asmt;
}


/****** Functions for Program & Subprograms ******/

// Given an AST_Vars struct break it in 
// several decl_t structs, one for each id
AST_Params *ast_insert_param_to_params(AST_Params *old_params,
									type_t type, AST_Vars *vars)
{
	AST_Params *new_params;

	if (old_params == NULL) {
		new_params = safe_malloc(sizeof(AST_Params));
		new_params->size = 0;
		new_params->elements = NULL;
	}
	else {
		new_params = old_params;
	}

	// Extend the parameters by the number of ids in vars
	int old_size = new_params->size;
	
	new_params->size += vars->size;
	new_params->elements = safe_realloc(new_params->elements,
										new_params->size * sizeof(decl_t *));
	
	for (int i = 0; i < vars->size; i++) {
		SEM_check_duplicate_variable_name(vars->elements[i]->id);
		decl_t *new_param = safe_malloc(sizeof(decl_t));
		new_param->is_parameter = true;
		new_param->datatype = safe_malloc(sizeof(AST_GeneralType));
		new_param->datatype->type = type;
		new_param->datatype->fields = NULL; // No field as parameter
		new_param->initial_value = NULL;
		new_param->variable = vars->elements[vars->size - i - 1];
		new_params->elements[old_size + i] = new_param;
	}

	return new_params;
}

// Ensure that the dimensions of any parameter arrays have been declared
void ast_check_params(AST_Params *params)
{
	decl_t *tmp;
	char *id;

	// Reverse the parameters for them to be in right order
	for (int i = 0; i < params->size / 2; i++) {
		tmp = params->elements[i];
		params->elements[i] = params->elements[params->size - i - 1];
		params->elements[params->size - i - 1] = tmp;
	}

	for (int i = 0; i < params->size; i++) {
		// Ensure that existing dimensions are declared
		AST_Dims *dims = params->elements[i]->variable->dims;
		if (dims != NULL) {
			SEM_check_declared_dims(dims, true);
		}
		
		// Insert the parameter ids in the symbol table
		id = params->elements[i]->variable->id;
		stbl_insert_variable(id, params->elements[i]);
	}
}

// Create a struct for the header of a function
AST_Header *ast_get_header(subprogram_type_t subprogram_type, type_t type,
									bool is_list, char *id, AST_Params *params)
{
	// Semantic checks
	if (subprogram_type == FUNCTION) {
		SEM_check_existing_arguments(params, id);
	}
	SEM_check_duplicate_subprogram_name(id);
	
	// Struct creation
	AST_Header *header = safe_malloc(sizeof(AST_Header));
	header->subprogram_type = subprogram_type;
	header->id = id;
	header->params = params;

	if (subprogram_type == FUNCTION) {
		header->returns_list = is_list;
		header->ret_type = type;
	}

	func_id = id;

	return header;
}

// Create a subprogram structure
AST_Subprogram *ast_get_subprogram(AST_Header *header, AST_Body *body)
{
	AST_Subprogram *subprogram = safe_malloc(sizeof(AST_Subprogram));
	subprogram->header = header;
	subprogram->body = body;
	func_id = NULL;
	return subprogram;
}

// Create or append to an AST_Subprograms struct with a new AST_Subprogram element
AST_Subprograms *ast_insert_subprogram_to_subprograms(AST_Subprograms *subprograms,
														AST_Subprogram *subprogram)
{
	AST_Subprograms *new_subprograms;

	if (subprograms == NULL) {
		// Case of the first value
		new_subprograms = safe_malloc(sizeof(AST_Subprogram));
		new_subprograms->size = 1;
		new_subprograms->elements = NULL;
	}
	else {
		// Case of the other values
		new_subprograms = subprograms;
		new_subprograms->size++;
	}

	// Extend the array with the new element
	new_subprograms->elements = safe_realloc(new_subprograms->elements,
									new_subprograms->size*sizeof(AST_Subprogram *));
	new_subprograms->elements[new_subprograms->size - 1] = subprogram;

	char *id = new_subprograms->elements[new_subprograms->size - 1]->header->id;
	stbl_insert_subprogram(id, new_subprograms->elements[new_subprograms->size - 1]);

	return new_subprograms;
}

// Create a body structure
AST_Body *ast_get_body(AST_Decls *decls, AST_Statements *statements)
{
	AST_Body *body = safe_malloc(sizeof(AST_Body));
	body->declarations = decls;
	body->statements = statements;
	return body;
}

// Create the high level structure of a program
AST_Program *ast_get_program(AST_Body *main, AST_Subprograms *subprograms)
{
	AST_Program *prog = safe_malloc(sizeof(AST_Program));
	prog->main = main;
	prog->subprograms = subprograms;
	return prog;
}

/****************************************************/
/******************* DEBUG & PRINTS *****************/
/****************************************************/

// Returns a string with one more tab than the input string
char *more_tabs(char *tabs)
{
	size_t size = strlen(tabs);
	char *new_tabs = safe_malloc(size+2);
	if (size!=0)
		strcpy(new_tabs, tabs);
	new_tabs[size] = '\t';
	new_tabs[size+1] = '\0';

	return new_tabs;
}

char *functype_str[] = {"SUBROUTINE", "FUNCTION"};
char *type_str[] = {
	"INTEGER", "LOGICAL", "REAL", "CHARACTER", "STRING", "COMPLEX", "RECORD"
};
char *var_type_str[] = {
	"ID", "DECL", "FUNC_CALL", "ARRAY_ACCESS", "REC_ACCESS", "LISTFUNC"
};

// Prints the dimensions of an array
void ast_print_dims(AST_Dims *dims)
{
	if (dims == NULL)
		return;

	printf("(");

	// Print all dims except the last one 
	for (int j = 0; j < dims->size - 1; j++) {
		AST_Dim *dim = dims->elements[j];

		if (dim->type == DIM_LITERAL) {
			// Some dims are stored as a literal
			printf("%d, ", dim->val);
		}
		else {
			// Some others are stored as a decl_t
			printf("%s, ", dim->decl->variable->id);
		}
	}
	
	// Print the last one seperately to close the parentheses
	AST_Dim *last_dim = dims->elements[dims->size - 1];
	if (last_dim->type == DIM_LITERAL) {
		printf("%d)", last_dim->val);
	}
	else {
		printf("%s)", last_dim->decl->variable->id);
	}
}

// Print Info about the header of a unit
void ast_print_header(AST_Header *header)
{
	printf("%s ", functype_str[header->subprogram_type]); 
	if (header->subprogram_type == FUNCTION) {
		printf("%s ", header->returns_list ? "LIST" : type_str[header->ret_type]);
	}
	printf("%s\n", header->id);

	if (header->params != NULL) {
		
		// Iterate over the parameters
		for (int i = 0; i < header->params->size; i++) {
			decl_t *param = header->params->elements[i];
			
			printf("\t%s %s ", type_str[param->datatype->type], param->variable->id);
			
			if (param->variable->list_depth)
				printf("[list depth: %d] ", param->variable->list_depth);

			ast_print_dims(param->variable->dims);
			
			printf("\n");
		}
	}

	return;
}

// Dummy for printing fields
void ast_print_fields(AST_Fields *fields, char *tabs)
{
	for (int i = 0; i < fields->size; i++) {
		AST_Field *field = fields->elements[i];
		printf("%s%s ", tabs, type_str[field->type]);
		
		if (field->type != REC) {

			AST_Vars *vars = field->vars;
			for (int j = 0; j < vars->size; j++) {
				AST_UndefVar *uvar = vars->elements[j];
				if (uvar->list_depth)
					printf("<LDEPTH-%d> ", uvar->list_depth);

				if (uvar->type == ARRAY) {
					printf("%s", uvar->id);
					ast_print_dims(uvar->dims);
					putchar(' ');
				}
				else {
					printf("%s ", uvar->id);
				}
			}
			putchar('\n');
		}
		else {
			char *mtabs = more_tabs(tabs);
			putchar('\n');
			ast_print_fields(field->fields, mtabs);
			printf("%s", tabs);

			AST_Vars *vars = field->vars;
			for (int j = 0; j < vars->size; j++) {
				AST_UndefVar *uvar = vars->elements[j];
				if (uvar->list_depth)
					printf("<LDEPTH-%d> ", uvar->list_depth);

				if (uvar->type == ARRAY) {
					printf("%s", uvar->id);
					ast_print_dims(uvar->dims);
					putchar(' ');
				}
				else {
					printf("%s ", uvar->id);
				}
			}
			putchar('\n');
		}
	}
	return;
}

// Prints type without \n unless it is record
// for records it prints its fields one line each
void ast_print_datatype(AST_GeneralType *datatype, char *tabs)
{
	char *mtabs = more_tabs(tabs);
	
	printf("%s%s ", tabs, type_str[datatype->type]);
	if (datatype->type == REC) {
		printf("\n");
		ast_print_fields(datatype->fields, mtabs);
		printf("%s", tabs);
	}
}

void ast_print_undefVar(AST_UndefVar *variable)
{
	if (variable->list_depth)
		printf("<LDEPTH-%d>  ", variable->list_depth);

	printf("%s", variable->id);
	if (variable->type == ARRAY)
		ast_print_dims(variable->dims);
	printf("\n");
}

void ast_print_func_call(AST_Header *header, AST_Expressions *args, char *tabs)
{
	printf("%s%s%s FUNCTION %s :\n", tabs, type_str[header->ret_type],
			(header->returns_list) ? "LIST" : "", header->id);

	tabs = more_tabs(tabs);
	for (int i = 0; i < args->size; i++) {
		ast_print_datatype(header->params->elements[i]->datatype, tabs);
		
		for (int j = 0; j < header->params->elements[i]->variable->list_depth; j++)
			printf(" LIST");
		
		printf(" %s", header->params->elements[i]->variable->id);
		ast_print_dims(header->params->elements[i]->variable->dims);
		printf(" =\n");
		ast_print_expression(args->elements[i], more_tabs(tabs));
	}

	printf("\n");
}

void ast_print_array_access(AST_Variable *variable, char *tabs)
{
	AST_Variable *parent_rec = variable->parent_rec;
	AST_UndefVar *array = variable->array;
	AST_Expressions *indices = variable->indices;
	AST_GeneralType *datatype = variable->datatype;
	
	if (parent_rec != NULL) {
		printf("%s%s\n", tabs, array->id);
		ast_print_expressions(indices, more_tabs(tabs));
		ast_print_variable_access(parent_rec->record, tabs);
	}
	else {
		ast_print_datatype(datatype, tabs);
		ast_print_undefVar(array);
		printf("%sARRAY ACCESS INDICES: \n", tabs);
		ast_print_expressions(indices, more_tabs(tabs));
	}
}

void ast_print_variable_access(AST_Variable *variable, char *tabs) 
{	
	char *variable_type_str[] = {
		"ID", "DECL", "FUNC_CALL", "ARRAY_ACCESS", "REC_ACCESS", "LISTFUNC"
	};

	static bool initial_rec_access = true;
	char *mtabs;

	if (variable->type != V_REC_ACCESS)
		printf("\n%sVariable type: %s\n", tabs, variable_type_str[variable->type]);

	switch(variable->type) {
		case V_DECL:
			ast_print_decl(variable->decl, tabs);
			break;
		case V_REC_ACCESS:
			if (initial_rec_access) {
				mtabs = more_tabs(tabs);
				initial_rec_access = false;
			}
			else {
				mtabs = tabs;
			}

			printf("\n%sVariable type: %s\n", mtabs, variable_type_str[variable->type]);
			printf("%sField: %s\n%sRecord:\n", mtabs, variable->field_var->id, mtabs);
			ast_print_variable_access(variable->record, mtabs);
			
			break;
		case V_ARRAY_ACCESS:
			ast_print_array_access(variable, tabs);
			break;
		case V_FUNC_CALL:
			ast_print_func_call(variable->subprog->header, variable->args, tabs);
			break;
		case V_LISTFUNC:
			mtabs = more_tabs(tabs);
			printf("%sLISTFUNC:\n", tabs);

			if (variable->listfunc->access)
				printf("%sDo %d hops and get list node data\n", mtabs, variable->listfunc->hops);
			else
				printf("%sDo %d hops and get list node address\n", mtabs, variable->listfunc->hops);
			
			printf("%sLIST:\n", tabs);
			ast_print_expression(variable->list, mtabs);
			
			break;
		default:
			break;
	}
}


void ast_print_constant(AST_Constant* constant, char *tabs)
{
	char *mtabs = more_tabs(tabs);

	switch (constant->type)
	{
	case INT:
		printf("%sINT(%d)\n", mtabs, constant->intval);
		break;
	case REAL:
		printf("%sREAL(%lf)\n", mtabs, constant->rval);
		break;
	case LOG:
		printf("%sLOG(%d)\n", mtabs, constant->lval);
		break;
	case CHAR:
		printf("%sCHAR(%c)\n", mtabs, constant->charval);
		break;
	case CMPLX:
		printf("%sCMPLX(%lf, %lf)\n", mtabs, constant->cmplxval.re,
											constant->cmplxval.im);
	default:
		break;
	}
}

void ast_print_initial_value(AST_Values *values, char *tabs)
{
	char *mtabs = more_tabs(tabs);
	for (int i = 0; i < values->size; i++) {
		ast_print_constant(values->elements[i], mtabs);
	}
	return;
}

void ast_print_decl(decl_t *decl, char *tabs)
{
	ast_print_datatype(decl->datatype, tabs);
	ast_print_undefVar(decl->variable);
	if (decl->initial_value != NULL)
		ast_print_initial_value(decl->initial_value, tabs);
	printf("\n");
}

void ast_print_decls(AST_Decls *decls, char *tabs)
{
	if (decls)
		for (int i = 0; i < decls->size; i++) {
			ast_print_decl(decls->declarations[i], tabs);
		}
}

void ast_print_statements(AST_Statements *statements, char *tabs)
{
	AST_Statement *ptr;
	char *mtabs = more_tabs(tabs);
	
	for (ptr = statements->head; ptr != NULL; ptr = ptr->next) {
		printf("%s<stmt-%d>\n", tabs, ptr->statement_id);
		if (ptr->type == SIMPLE) {
			ast_print_simple_statement(ptr->simple, mtabs);
		}
		else if (ptr->type == COMPOUND) {
			ast_print_compound_statement(ptr->compound, mtabs);
		}
		printf("\n");
	}
}

void ast_print_simple_statement(AST_SimpleStatement *statement, char *tabs)
{
	char *simple_stmt_type_str[] = {
		"ASSIGNMENT", "GOTO", "IF", "CALL_SUBROUTINE",
		"IO", "CONTINUE", "RETURN", "STOP"
	};
	char *mtabs = more_tabs(tabs);
	
	printf("%s%s statement\n", tabs, simple_stmt_type_str[statement->type]);

	switch (statement->type)
	{
	case GOTO:
		if (statement->goto_statement->variable == NULL) {
			printf("%sGOTO -> stmt %d\n", tabs,
							statement->goto_statement->statement->statement_id);
		}
		// else {
		// 	printf("%s GOTO depends on variable:\n", tabs);

		// 	ast_print_variable_access(statement->variable, mtabs);
		// 	printf("%s statement id list:", tabs);
		// 	for (AST_Statement *ptr; ptr != NULL; )

		// 	for 
		// }
		break;
	case ASSIGNMENT:
		printf("%sAssign to variable: \n", tabs);
		ast_print_variable_access(statement->assignment->variable, mtabs);
		
		if (statement->assignment->type == AS_STRING) {
			printf("%s The string \"%s\"", tabs, statement->assignment->string);
		}
		else if (statement->assignment->type == AS_EXPRESSION) {
			ast_print_expression(statement->assignment->expression, mtabs);
		}
		break;
	default:
		break;
	}
}

// Print an array of expressions
void ast_print_expressions(AST_Expressions *listexpr, char *tabs)
{
	char *mtabs = more_tabs(tabs);

	for (int i = 0; i < listexpr->size; i++) {
		printf("%s<subexpr-%d>\n", tabs, i);
		ast_print_expression(listexpr->elements[i], mtabs);
	}
}

// Print a single expressions
void ast_print_expression(AST_Expression *expression, char *tabs)
{
	char *unary_op_str[] =  {"PLUS", "MINUS", "NOT", "LENGTH", "NEW"};
	char *binary_op_str[] = {
		"PLUS","MINUS", "AND", "OR",
		"GT", "GE", "LT", "LE", "EQ",
		"NE", "MUL", "DIV", "POWER", "CMPLX"
	};
	char *mtabs = more_tabs(tabs);

	switch (expression->expr_type)
	{
	case EXPR_UNARY:
		printf("%sop: %s\n", tabs, unary_op_str[expression->unary.op]);
		printf("%schild: \n", tabs);
		ast_print_expression(expression->unary.child, mtabs);
		break;
	case EXPR_BINARY:
		printf("%sop: %s\n", tabs, binary_op_str[expression->binary.op]);
		printf("%schild1: \n", tabs);
		ast_print_expression(expression->binary.child1, mtabs);
		printf("%schild2: \n", tabs);
		ast_print_expression(expression->binary.child2, mtabs);
		break;
	case EXPR_VARIABLE:
		printf("%sExpression reduces to variable: \n", tabs);
		ast_print_variable_access(expression->variable, mtabs);
		break;
	case EXPR_CONSTANT:
		printf("%sExpression reduces to simple_constant: \n", tabs);
		ast_print_constant(expression->constant, tabs);
		break;
	case EXPR_LISTEXPR:
		printf("%sExpression reduces to list expression: \n", tabs);
		printf("%s[\n", tabs);
		ast_print_expressions(expression->listexpr, tabs);
		printf("%s]\n", tabs);
		break;
	default:
		printf("%s(Unsupported printing for expression type %d)\n",
										tabs, expression->expr_type);
		break;
	}
}

void ast_print_compound_statement(AST_CompoundStatement *statement, char  *tabs)
{
	return;
}

void ast_print_body(AST_Body *body, char *tabs)
{
	char *mtabs = more_tabs(tabs);

	printf("%sDeclarations:\n", tabs);
	ast_print_decls(body->declarations, mtabs);
	printf("%sStatements:\n", tabs);
	ast_print_statements(body->statements, mtabs);
	// printf("\"%s\"\n", mtabs);
	
	free(mtabs);
}

// Function for printing a subprogram ast
void ast_print_subprogram(AST_Subprogram *subgprogram)
{
	printf("+++++++++++++++++++++++++++++++++++++\n");
	ast_print_header(subgprogram->header);
	ast_print_body(subgprogram->body, "");
	printf("+++++++++++++++++++++++++++++++++++++\n");
}

// Print an AST_Values structure
void ast_print_values(AST_Values *values)
{
	printf("#-----Values-----#\n");
	for (int i = 0; i < values->size; i++) {
		AST_Constant *constant = values->elements[i];
		switch (constant->type) {
			case INT:
				printf("Integer value %d\n", constant->intval);
				break;
			case LOG:
				printf("Logical value %s\n", (constant->lval)?"True":"False");
				break;
			case REAL:
				printf("Real value %lf\n", constant->rval); 
				break;
			case CHAR:
				printf("Character value %c\n", constant->charval);
				break;
			case STR:
				printf("String value %s\n", constant->strval);
				break;
			case CMPLX:
				printf("Complex value with Re = %lf and Im = %lf\n",
					constant->cmplxval.re, constant->cmplxval.im);
				break;
			default:
				printf("Value of unknown type\n");
		}
	}
	printf("#------------------#\n");
}