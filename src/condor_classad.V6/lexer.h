/*********************************************************************
 *
 * Condor ClassAd library
 * Copyright (C) 1990-2003, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI and Rajesh Raman.
 *
 * This source code is covered by the Condor Public License, which can
 * be found in the accompanying LICENSE file, or online at
 * www.condorproject.org.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * AND THE UNIVERSITY OF WISCONSIN-MADISON "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, OF SATISFACTORY QUALITY, AND FITNESS
 * FOR A PARTICULAR PURPOSE OR USE ARE DISCLAIMED. THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS AND THE UNIVERSITY OF WISCONSIN-MADISON
 * MAKE NO MAKE NO REPRESENTATION THAT THE SOFTWARE, MODIFICATIONS,
 * ENHANCEMENTS OR DERIVATIVE WORKS THEREOF, WILL NOT INFRINGE ANY
 * PATENT, COPYRIGHT, TRADEMARK, TRADE SECRET OR OTHER PROPRIETARY
 * RIGHT.
 *
 *********************************************************************/

#ifndef __LEXER_H__
#define __LEXER_H__

#include "common.h"
#include "value.h"
#include "lexerSource.h"

BEGIN_NAMESPACE( classad )


// the lexical analyzer class
class Lexer
{
	public:
		enum TokenType
		{
			LEX_TOKEN_ERROR,
			LEX_END_OF_INPUT,
			LEX_TOKEN_TOO_LONG,
			LEX_INTEGER_VALUE,
			LEX_REAL_VALUE,
			LEX_BOOLEAN_VALUE,
			LEX_STRING_VALUE,
			LEX_UNDEFINED_VALUE,
			LEX_ERROR_VALUE,
			LEX_IDENTIFIER,
			LEX_SELECTION,
			LEX_MULTIPLY,
			LEX_DIVIDE,
			LEX_MODULUS,
			LEX_PLUS,
			LEX_MINUS,
			LEX_BITWISE_AND,
			LEX_BITWISE_OR,
			LEX_BITWISE_NOT,
			LEX_BITWISE_XOR,
			LEX_LEFT_SHIFT,
			LEX_RIGHT_SHIFT,
			LEX_URIGHT_SHIFT,
			LEX_LOGICAL_AND,
			LEX_LOGICAL_OR,
			LEX_LOGICAL_NOT,
			LEX_LESS_THAN,
			LEX_LESS_OR_EQUAL,
			LEX_GREATER_THAN,
			LEX_GREATER_OR_EQUAL,
			LEX_EQUAL,
			LEX_NOT_EQUAL,
			LEX_META_EQUAL,
			LEX_META_NOT_EQUAL,
			LEX_BOUND_TO,
			LEX_QMARK,
			LEX_COLON,
			LEX_COMMA,
			LEX_SEMICOLON,
			LEX_OPEN_BOX,
			LEX_CLOSE_BOX,
			LEX_OPEN_PAREN,
			LEX_CLOSE_PAREN,
			LEX_OPEN_BRACE,
			LEX_CLOSE_BRACE,
			LEX_BACKSLASH,
			LEX_ABSOLUTE_TIME_VALUE,
			LEX_RELATIVE_TIME_VALUE
		};

		class TokenValue
		{
			public:
				TokenValue( ) {
					tt                   = LEX_TOKEN_ERROR;
					factor               = Value::NO_FACTOR;
					intValue             = 0;
					realValue            = 0.0;
					boolValue            = false;
					relative_secs        = 0;
					absolute_secs.secs   = 0;
					absolute_secs.offset = 0;
				}

				~TokenValue( ) {
				}

				void SetTokenType( TokenType t ) {
					tt = t;
				}

				void SetIntValue( int i, Value::NumberFactor f) {
					intValue = i;
					factor = f;
				}

				void SetRealValue( double r, Value::NumberFactor f ) {
					realValue = r;
					factor = f;
				}

				void SetBoolValue( bool b ) {
					boolValue = b;
				}

				void SetStringValue( const std::string &str) {
					strValue = str;
				}

				void SetAbsTimeValue( abstime_t asecs ) {
					absolute_secs = asecs;
				}

				void SetRelTimeValue( time_t rsecs ) {
					relative_secs = rsecs;
				}

				TokenType GetTokenType( ) {
					return tt;
				}

				void GetIntValue( int& i, Value::NumberFactor& f) {
					i = intValue;
					f = factor;
				}

				void GetRealValue( double& r, Value::NumberFactor& f ) {
					r = realValue;
					f = factor;
				}

				void GetBoolValue( bool& b ) {
					b = boolValue;
				}

				void GetStringValue( std::string &str ) {
					str = strValue;	
				}	

				void GetAbsTimeValue( abstime_t& asecs ) {
					asecs = absolute_secs;
				}

				void GetRelTimeValue( time_t& rsecs ) {
					rsecs = relative_secs;
				}

				void CopyFrom( TokenValue &tv ) {
					tt = tv.tt;
					factor = tv.factor;
					intValue = tv.intValue;
					realValue = tv.realValue;
					boolValue = tv.boolValue;
					relative_secs = tv.relative_secs;
					absolute_secs = tv.absolute_secs;
					strValue = tv.strValue;
				}
					
			private:
				TokenType 			tt;
				Value::NumberFactor factor;
				int 				intValue;
				double 				realValue;
				bool 				boolValue;
				std::string			strValue;
				time_t				relative_secs;
				abstime_t           absolute_secs;
		};

		// ctor/dtor
		Lexer ();
		~Lexer ();

		// initialize methods
		bool Initialize(LexerSource *source);
		bool Reinitialize( );

		// cleanup function --- purges strings from string space
		void FinishedParse();
		
		// the 'extract token' functions
		TokenType PeekToken( TokenValue* = 0 );
		TokenType ConsumeToken( TokenValue* = 0 );

		// internal buffer for token accumulation
		std::string lexBuffer;					    // the buffer itselfw

		// miscellaneous functions
		static char *strLexToken (int);				// string rep'n of token

		// set debug flag 
		void SetDebug( bool d ) { debug = d; }

	private:
			// grant access to FunctionCall --- for tokenize{Abs,Rel}Time fns
		friend class FunctionCall;
		friend class ClassAdXMLParser;

		// internal state of lexical analyzer
		TokenType	tokenType;             		// the integer id of the token
		LexerSource *lexSource;
		int    		markedPos;              	// index of marked character
		char   		savedChar;          		// stores character when cut
		int    		ch;                     	// the current character
		int			lexBufferCount;				// current offset in lexBuffer
		bool		inString;					// lexing a string constant
		bool		accumulating;				// are we in a token?
		int 		debug; 						// debug flag

		// cached last token
		TokenValue 	yylval;						// the token itself
		bool		tokenConsumed;				// has the token been consumed?

		// internal lexing functions
		void 		wind(void);					// consume character from source
		void 		mark(void);					// mark()s beginning of a token
		void 		cut(void);					// delimits token

		// to tokenize the various tokens
		int 		tokenizeNumber (void);		// integer or real
		int 		tokenizeAlphaHead (void);	// identifiers/reserved strings
		int 		tokenizePunctOperator(void);// punctuation and operators
		int         tokenizeString(char delim);//string constants
};

END_NAMESPACE // classad

#endif //__LEXER_H__
