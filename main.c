/*
    MIT License

    Copyright (c) 2022 ib-dev-cpp

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

/*
*
* This calculator proudly made by me and it's free and opensource 
*
*/

#define NDEBUG

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define MAX_LENGHT 20


// this variable used to track the open and close brace in the expression
static int track = 0;

// made my own is blank function because the original one have some problemes
bool misblank ( int __C );

// token enum
typedef enum _Tokens {
    OPBRACE,
    CLBRACE,
    NUM,
    PLUS,
    MINUS,
    MUL,
    DIV,
    MODULOS,
    END
} Tokens;

// struct Token
typedef struct _Token {

    unsigned char sz;
    char tokenstr [MAX_LENGHT];
    Tokens type;

} Token;

// static list to hold tokens in the string
// and it is global so it can be accessed by all the functions
static Token * tkns;

#ifndef NDEBUG

void printtok ( Token t ) {

    printf ( "Token.sz = %d\nToken.str = %s\nToken.type = %d\n\n\n", ( int ) t.sz, t.tokenstr, t.type );
}

void printall ( void ) {

    for ( size_t i = 0; tkns [i].type != END; ++i ) {

        printtok ( tkns [i] );
    }
    return;
}

#endif

// the tokenizer: convert text into Token struct so it cna be used by the execute function
Token Tokenizer ( char ** str );

// simple function to check if the current operator is the first (respecting operator order)
bool isFirst ( Tokens __T ) {

    if ( __T == PLUS || __T == MINUS ) return false;

    return true;
}

// simple function to check if the token passed is an Operator
bool isOperator ( Tokens __T ) {

    if ( __T == PLUS || __T == MINUS || __T == MUL || __T == MODULOS || __T == DIV ) return true;

    return false;
}

// function can do the math of to number and return the result as a double and it's the simple version of
// the execute function
Token executeSimple ( Token num1, Token op, Token num2 );

// remove an item from the global list
int mRemove ( size_t pos );

// replace three token by one from the global List
int replace ( size_t pos, Token t );

// defining execute as _execute because the parameter in the first are known 
#define execute() _execute ( 0, false, false, 0 )

// the execute function: execute the expression and return the result as a double
// the parameter: start: the position where to start the execution in the global list
//              doprev: when there is a close brace, we check the value and go back to execute the
//              previous expression if needed
//              shEnd: if there is an open brace we set the value, otherwise we just continue 
//               (it is needed in too many places in the code) 
//              prevbrace: keep track how many open braces we have in the expression (help in nested prenthesis)
int _execute ( size_t start, bool doprev, bool shEnd, int prevbrace );

int main ( int argc, char ** argv ) {

    char * str;
    size_t sz;
    if ( argc >= 2 ) {
        /* getting the size of a file */
        struct stat FileInfo;
        stat ( argv [1], &FileInfo );
        sz = FileInfo.st_size;
        
        if ( sz == 0 ) {
            fprintf ( stderr, "The given file \"%s\" is void\n", argv [1] );
            abort ();
            return 1;
        }

        str = (char *) malloc ( (sz + 2) * sizeof ( char ) ); // allocating our string to hold the file text

        /* read the file*/
        FILE * srcCode = fopen ( argv [1], "r" );
        if ( srcCode == NULL ) {

            fprintf ( stderr, "Invalid given file: %s\n", strerror ( errno ) );
            abort ();
            exit (1);
        }

        for ( unsigned int i = 0; i < sz; ++i ) {
            
            str [i] = fgetc ( srcCode );
            continue;
        }
        str [sz] = 0;
        fclose ( srcCode );
    } else {

        // getting the string from stdin
        printf ( "Write your math expression (don't exceed 100 char)\n=> " );
        str = (char *) malloc ( 101 * sizeof ( char ) );
        fgets ( str, 100, stdin );
        str [strlen ( str ) +1] = 0;
    }


    /* setting up our array of tokens */
    sz = 1; // reuse this variable

    tkns = malloc ( sizeof ( struct _Token ) );

    while ( *str != 0 ) {

        tkns = realloc ( tkns, sz * sizeof ( struct _Token ) );
        tkns [ sz - 1 ] = Tokenizer ( &str );
        sz++;

        // check the track variable again
        if ( track < 0 ) {

            fprintf ( stderr, "Unexpected Error there is %d ')' more than '('\n",  -1 * track );
            exit ( 1 );
        }
    }
    // we don't need the str anymore
    free ( str );

    /* insert the END token incase we haven't */
    tkns = realloc ( tkns, sz * sizeof ( Token ) );
    tkns [ sz - 1 ].type = END;

    // check the track variable again
    if ( track > 0 ) {

        fprintf ( stderr, "Unexpected Error there is %d '(' more than ')'\n", track );
        abort ();
        exit ( 1 );
    }
 
    // start to execute the expression
    execute ();

    // print the result
    printf ( "\n= %s\n", tkns [0].tokenstr );
    
    // free the memory
    free ( tkns );

    return 0;
}

bool misblank ( int __C ) {

   // check if the character is a blank
    switch ( __C ) {

        case '\n':
            return true;
        case '\t':
            return true;
        case ' ':
            return true;
        case '\r':
            return true;
        default:
            return false;
    }

    return false;
}

Token Tokenizer ( char ** str ) {

    Token t;

    // skip whitespaces
    while ( misblank ( **str ) )
        (*str)++;

    
    // getting the numbers
    bool isfloat = false, isnum = false; // helper variables
    t.sz = 0;

    while ( isdigit ( **str ) || ( **str == '.' && !isfloat ) ) {

        isnum = true;
        if ( t.sz == ( MAX_LENGHT - 1 ) ) {
            
            fprintf ( stderr, "number of digit exceded in a number\n" );
            abort ();
            break;
        }
        
        if ( **str == '.' ) isfloat = true;

        t.tokenstr [t.sz] = **str;
        (*str)++;
        ++t.sz;
    }

    // just checking for extra '.'
    // to fix error
    if ( isnum && **str == '.' ) {

        fprintf ( stderr, "UNEXPECTED Token '.'\n" );
        abort ();
        exit (1);

    } else if ( isnum ) {

        t.tokenstr [t.sz] = 0;
        t.type = NUM;
        return t;
    }

    t.tokenstr [1] = 0;
    t.sz = 1;
    switch ( **str ) {

        case 0:
            t.type = END;
            return t;
            break;
        case '(':
            strncpy ( t.tokenstr, "(", 1 );
            t.type = OPBRACE;
            ++track;
            break;
        case ')':
            strncpy ( t.tokenstr, ")", 1 );
            t.type = CLBRACE;
            --track;
            break;
        case '+':
            strncpy ( t.tokenstr, "+", 1 );
            t.type = PLUS;
            break;
        case '-':
            strncpy ( t.tokenstr, "-", 1 );
            t.type = MINUS;
            break;
        case '*':
            strncpy ( t.tokenstr, "*", 1 );
            t.type = MUL;
            break;
        case '/':
            strncpy ( t.tokenstr, "/", 1 );
            t.type = DIV;
            break;
        case '%':
            strncpy ( t.tokenstr, "%%", 1 );
            t.type = MODULOS;
            break;
        default:
            fprintf ( stderr, "Unexpected ERROR: Invalid Token\n" );
            abort ();
            exit (1);
            break;
    }

    (*str)++;
    return t;
}

Token executeSimple ( Token num1, Token op, Token num2 ) {

    long double n1 = strtold ( num1.tokenstr, NULL ), n2 = strtold ( num2.tokenstr, NULL );

    long double res;

    // execute operation
    if ( op.type == PLUS ) res = n1 + n2;
    else if ( op.type == MINUS ) res = n1 - n2;
    else if ( op.type == MUL ) res = n1 * n2;
    else if ( op.type == DIV ) res = n1 / n2;
    else if ( op.type == MODULOS ) {

        // this operator work only with integer type
        int r = (int) n1 % (int) n2;
        Token v;
        v.type = NUM;
        sprintf ( v.tokenstr, "%i", r );
        return v;
    }

    Token t;
    t.type = NUM;
    sprintf ( t.tokenstr, "%LF", res );
    
    return t;
}

int replace ( size_t pos, Token t ) {

    tkns [pos] = t;
    size_t i = pos + 1;

    do {

        tkns [i] = tkns [ i + 2 ];
        ++i;
    } while ( tkns [i].type != END );

    return 0;
}

int mRemove ( size_t pos ) {

    size_t i = pos;
    do
    {
        tkns [i] = tkns [ i + 1 ];
        ++i;
    } while ( tkns [i].type != END );
    
}

int _execute ( size_t start, bool doprev, bool shEnd, int prevbrace ) {

    size_t cur = start;

    /* it's really complicated to explain */

    while ( tkns [cur].type != END ) {

        if ( tkns [cur].type == NUM ) {

            if ( isOperator ( tkns [cur + 1].type ) ) {

                if ( tkns [ cur + 3 ].type == END || tkns [ cur + 3 ].type == CLBRACE ) {

                   

                    if ( tkns [ cur + 2 ].type == NUM ) {

                        replace ( cur, executeSimple ( tkns [cur], tkns [ cur + 1 ], tkns [ cur + 2 ] ) );
                    } // add error handler

                    if  ( tkns [ cur + 1 ].type == CLBRACE ) {

                        if ( doprev ) return 0;
                        else if ( prevbrace ) {
                            --prevbrace;
                            return 0;
                        }
                        mRemove ( cur + 1 );
                        _execute ( cur, false, shEnd, prevbrace);
                    }

                    return 0;
                } else if ( isFirst ( tkns [cur + 1].type ) ) {

                    if ( tkns [ cur + 2 ].type == OPBRACE ) {

                        mRemove ( cur + 2 );
                        _execute ( cur + 2, false, true, (shEnd? ++prevbrace: prevbrace) );
                    } if ( tkns [ cur + 2 ].type != NUM ) {

                        fprintf ( stderr, "Unexpected token '%s'\n", tkns [ cur + 2 ].tokenstr );
                        abort ();
                        exit (1);
                    } else 
                        replace ( cur, executeSimple ( tkns [cur], tkns [ cur + 1 ], tkns [ cur + 2 ] ) );

                } else if ( tkns [cur + 2 ].type == OPBRACE ) {

                    mRemove ( cur + 2); 
                    _execute ( cur + 2, false, true, (shEnd? ++prevbrace: prevbrace) );
                } else {

                    _execute ( cur + 2, shEnd, shEnd, prevbrace );
                }
            } else if ( tkns [ cur + 1 ].type == END ) {

                return 0;
            } else if ( tkns [ cur + 1 ].type == CLBRACE ) {

                if ( doprev ) return 0;
                else if ( prevbrace ) {
                    --prevbrace;
                    return 0;
                }

                mRemove ( cur + 1 );

                _execute ( cur, false, false, false );
            }
        } else if ( tkns [cur].type == OPBRACE ) {

            mRemove ( cur ); 
            _execute ( cur, false, true, (shEnd)? ++prevbrace: prevbrace );
        } else if ( tkns [cur].type == CLBRACE ) {
            
            if ( doprev ) return 0;
            else if ( prevbrace ) {
                --prevbrace;
                return 0;
            }

            mRemove ( cur );
            return 0;
        } else if ( tkns [cur].type == END ) {

            return 0;
        } else {

            fprintf ( stderr, "Unexpected Token '%s', num '%d'\n", tkns [cur].tokenstr, tkns [cur].type );
            abort ();
            exit (1);
        }
    }
    
}
