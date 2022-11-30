#ifndef _LEXER_H_
#define _LEXER_H_

enum {
	// EOF
	T_EOF = 0,

	// Keywords
	T_FUNCTION,
	T_SUBROUTINE,
	T_END,
	T_INTEGER,
	T_REAL,
	T_LOGICAL,
	T_CHARACTER,
	T_COMPLEX,
	T_RECORD,
	T_ENDREC,
	T_LIST,
	T_DATA,
	T_CONTINUE,
	T_GOTO,
	T_CALL,
	T_READ,
	T_WRITE,
	T_NEW,
	T_LENGTH,
	T_IF, 
	T_THEN, 
	T_ELSE,
	T_ENDIF,
	T_DO,
	T_ENDDO,
	T_STOP,
	T_RETURN,
	
	// ID
	T_ID,
	
	// Constants
	T_ICONST,
	T_RCONST,
	T_LCONST,
	T_CCONST,

	// Operators
	T_OROP,
	T_ANDOP,
	T_NOTOP,
	T_RELOP,
	T_ADDOP,
	T_MULOP,
	T_DIVOP,
	T_POWEROP,

	// List Functions
	T_LISTFUNC,
	
	// String
	T_STRING,
	
	//Other
	T_LPAREN,
	T_RPAREN,
	T_COMMA,
	T_ASSIGN,
	T_DOT,
	T_COLON,
	T_LBRACK,
	T_RBRACK	
};

#endif