#ifndef _ARG_EVALUATOR_H_
#define _ARG_EVALUATOR_H_

/*
MIT License

Copyright (c) 2023 LokleLama

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

typedef int (*argCallback_t)(int argc, char** argv);

typedef enum{
    vType_none,
    vType_callback,
    vType_string,
    vType_integer,
    vType_float,
    vType_double
} vType_t;

typedef struct{
    const char* shortOption;
    const char* longOption;
    const char* helpText;

    int argumentCount;
    vType_t variableType;
    void* variablePointer; // this can only be applied when argumentCount = 1;
    int* occurrences;
} ArgumentDefinition_t;

void argEval_enableHelpWithoutArguments();
void argEval_setIntroText(const char* text);
void argEval_registerArguments(const ArgumentDefinition_t* argumentArray, const size_t count, const ArgumentDefinition_t* helpArgument);
void argEval_registerCallbackForExtraArguments(argCallback_t callback);
int argEval_Parse(int argc, char** argv, FILE* errorOut);

#endif //_ARG_EVALUATOR_H_

/* Example: */
/*
#include <stdlib.h>
#include <stdio.h>

#include "argEval.h"


struct config_t{
	char*       outFileName;

	int         verbose;
} _config = {
	NULL,
	
	0,
};

int setOutFileFunction(int argc, char** argv);
int extraArgumentFunction(int argc, char** argv);

 * The ArgumentDefinition_t structure
 ************************************
 *
 * 1: String for the short option (eg: program -v [...])
 * 2: String for the long option (eg: program --verbose [...])
 * 3: Help string - a string that describes what the option does (maybe even includes the default value)
 * 4: Number of parameters that follow the option
 * 5: Type of the parameters that follow the option. This can be one of the following:
 *         vType_none       - This is for the case when no parameter are following the option or any parameter should get ignored
 *         vType_callback   - This is for the case when the programmer wants to make it's own parameter evaluation routine. Must be followed by a pointer to a routine of typedef int (*argCallback_t)(int argc, char** argv).
 *         vType_string     - This is for the case when the option expects a string. Must be followed by a pointer to a string variable.
 *         vType_integer    - This is for the case when the option expects an integer. Must be followed by a pointer to an integer variable.
 *         vType_float      - This is for the case when the option expects a float. Must be followed by a pointer to an float variable.
 *         vType_double     - This is for the case when the option expects a double. Must be followed by a pointer to an double variable.
 * 6: The pointer specified by the last field. Can be NULL in case of vType_none
 * 7: This field expects a pointer to an integer variable which will count how many times the option has been put in the command line. This can be NULL if not used.
 *

static ArgumentDefinition_t _argArray[] = {
  {"h", "help"   , "shows this screen"                      , 0, vType_none    , NULL              , NULL},
  {"i", "inFile" , "specifies an input file (default = {})" , 1, vType_string  , NULL              , NULL},
  {"o", "outFile", "specifies an output file"               , 1, vType_callback, setOutFileFunction, NULL},
  {"v", "verbose", "enables debug messages"                 , 0, vType_none    , NULL              , &_config.verbose},
};

int main(int argc, char** argv)
{
  char* inFileName = NULL;

  _argArray[1].variablePointer = &inFileName;

  argEval_enableHelpWithoutArguments();
  argEval_registerCallbackForExtraArguments(extraArgumentFunction);
  argEval_registerArguments(_argArray, sizeof (_argArray) / sizeof (ArgumentDefinition_t), &_argArray[0]);
  argEval_Parse(argc, argv, stderr);

  printf("Input File = %s\n", inFileName);
  printf("Output File= %s\n", _config.outFileName);
  if(_config.verbose != 0){
    printf("Debug Messages enabled...\n");
  }

  return (EXIT_SUCCESS);
}

int setOutFileFunction(int argc, char** argv){
    _config.outFileName = argv[0];
    return 0;
}

int extraArgumentFunction(int argc, char** argv){
    printf("Got an extra Argument: %s\n", argv[0]);
    return -1;
}
*/