#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "argEval.h"

static const ArgumentDefinition_t* _helpArgument = NULL;
static const ArgumentDefinition_t* _argumentArray = NULL;
static size_t _argumentArrayCount = 0;
static int _enableHelpForNoArguments = 0;

static argCallback_t _extraCallback = NULL;

static char _shortOptionHelp[16] = "";
static char  _longOptionHelp[16] = "";
static char    _argumentHelp[16] = "";

static char _shortEmptyOptionHelp[16] = "";
static char  _longEmptyOptionHelp[16] = "";
static char    _argumentEmptyHelp[16] = "";
static int     _maxArgumentCount = 0;

static void _argEval_displayHelp(FILE* helpOut, const char* program_path);
static const ArgumentDefinition_t* _findShortOption(const char* arg);
static const ArgumentDefinition_t* _findLongOption(const char* arg);

static const char* _intro = "This Program supports the following command-line options:";

void argEval_enableHelpWithoutArguments(){
    _enableHelpForNoArguments = 1;
}

void argEval_setIntroText(const char* text){
    _intro = text;
}

void argEval_registerCallbackForExtraArguments(argCallback_t callback){
    _extraCallback = callback;
}

void argEval_registerArguments(const ArgumentDefinition_t* argumentArray, const size_t count, const ArgumentDefinition_t* helpArgument){
    if((argumentArray == NULL) || (count <= 0)){
        return;
    }
    size_t maxShortLen = 0;
    size_t maxLongLen = 0;
    int n;

    _argumentArray      = argumentArray;
    _argumentArrayCount = count;
    _helpArgument       = helpArgument;

    for(n = 0; n < (int)count; n++){
        if(argumentArray[n].shortOption != NULL){
            if(strlen(argumentArray[n].shortOption) > maxShortLen){
                maxShortLen = strlen(argumentArray[n].shortOption);
            }
        }
        if(argumentArray[n].longOption != NULL){
            if(strlen(argumentArray[n].longOption) > maxLongLen){
                maxLongLen = strlen(argumentArray[n].longOption);
            }
        }
        if(argumentArray[n].argumentCount > _maxArgumentCount){
            _maxArgumentCount = argumentArray[n].argumentCount;
        }
    }

    if(maxShortLen > 0){
        sprintf(_shortOptionHelp, "-%%-%ds", (int)maxShortLen);
        sprintf(_shortEmptyOptionHelp, " %%-%ds", (int)maxShortLen);
    }

    if(maxLongLen > 0){
        sprintf(_longOptionHelp, "--%%-%ds", (int)maxLongLen);
        sprintf(_longEmptyOptionHelp, "  %%-%ds", (int)maxLongLen);
    }

    if(_maxArgumentCount > 0){
        sprintf(_argumentHelp, " %%-%ds", (int)maxLongLen + 2);
        sprintf(_argumentEmptyHelp, " %%-%ds", (int)maxLongLen + 2);
    }
}

int argEval_Parse(int argc, char** argv, FILE* errorOut){
    const ArgumentDefinition_t* argument;
    int found = 0;
    int n;
    for(n = 1; n < argc; n++){
        if((argument = _findShortOption(argv[n])) == NULL){
            argument = _findLongOption(argv[n]);
        }
        if(argument != NULL){
            found++;
            if(argument->occurrences != NULL){
                (*argument->occurrences)++;
            }
            if(argument == _helpArgument){
                FILE* helpOut = errorOut;
                if(helpOut == NULL){
                    helpOut = stdout;
                }
                _argEval_displayHelp(helpOut, argv[0]);
            }
            if(argument->argumentCount == 0){
                if(argument->variablePointer != NULL && argument->variableType == vType_callback){
                    argCallback_t callback = (argCallback_t)argument->variablePointer;
                    int ret = callback(0, NULL);
                    if((ret != 0) && (errorOut != NULL)){
                        fprintf(errorOut, "Return value of callback argument %s was: %d\n", argv[n], ret);
                    }
                }else{
                    if((errorOut != NULL) && (argument->occurrences == NULL)){
                        fprintf(errorOut, "No Callback has been specified for argument %s - there is no way for the program to use this option\n", argv[n]);
                    }
                }
            }else{
                if((n + argument->argumentCount) >= argc){
                    if(errorOut != NULL){
                        fprintf(errorOut, "Not enougth parameter specified for argument %s\n", argv[n]);
                        _argEval_displayHelp(errorOut, argv[0]);
                    }else{
                        _argEval_displayHelp(stdout, argv[0]);
                    }
                }else if(argument->variableType != vType_none && argument->variablePointer != NULL){
                    char* args[argument->argumentCount];
                    int m;
                    for(m = 0; m < argument->argumentCount; m++){
                        args[m] = argv[n +m +1];
                    }

                    switch(argument->variableType){
                        case vType_callback:
                        {
                            argCallback_t callback = (argCallback_t)argument->variablePointer;
                            int ret = callback(argument->argumentCount, args);
                            if((ret != 0) && (errorOut != NULL)){
                                fprintf(errorOut, "Return value of callback argument %s was: %d\n", argv[n], ret);
                            }
                        } break;
                        case vType_string:
                        {
                            char** variable = (char**)argument->variablePointer;
                            *variable = args[0];
                        }break;
                        case vType_integer:
                        {
                            if(args[0][0] == '-'){
                                int* variable = (int*)argument->variablePointer;
                                *variable = strtol(args[0], NULL, 0);
                            }else{
                                unsigned int* variable = (unsigned int*)argument->variablePointer;
                                *variable = strtoul(args[0], NULL, 0);
                            }
                        }break;
                        case vType_float:
                        {
                            float* variable = (float*)argument->variablePointer;
                            *variable = strtof(args[0], NULL);
                        }break;
                        case vType_double:
                        {
                            double* variable = (double*)argument->variablePointer;
                            *variable = strtod(args[0], NULL);
                        }break;
                        default:
                        {
                            if(errorOut != NULL){
                                fprintf(errorOut, "Configuration Error: Unknown variable type fot option: %s\n", argv[n]);
                            }
                        }break;
                    }
                    n += argument->argumentCount;
                }else if(argument->variablePointer == NULL && argument->occurrences == NULL && errorOut != NULL){
                    fprintf(errorOut, "Configuration Error: There is no way for the program to use this option: %s\n", argv[n]);
                }else if(errorOut != NULL){
                    //TODO: error message
                }
            }
        }else{
            if(_extraCallback != NULL){
                char* args[1];
                int ret;
                args[0] = argv[n];
                ret = _extraCallback(1, args);
                if((ret != 0) && (errorOut != NULL)){
                    fprintf(errorOut, "Return value of callback for extra arguments was: %d\n", ret);
                }
            }
        }
    }

    if(found == 0 && _enableHelpForNoArguments != 0){
        FILE* helpOut = errorOut;
        if(helpOut == NULL){
            helpOut = stdout;
        }
        _argEval_displayHelp(helpOut, argv[0]);
    }
    
    return 0;
}

static const ArgumentDefinition_t* _findShortOption(const char* arg){
    if(arg[0] == '-' && arg[1] != '-'){
        const char* pure = &arg[1];
        size_t n;
        for(n = 0; n < _argumentArrayCount; n++){
            if(_argumentArray[n].shortOption != NULL){
                if(strcmp(pure, _argumentArray[n].shortOption) == 0){
                    return &_argumentArray[n];
                }
            }
        }
    }
    return NULL;
}

static const ArgumentDefinition_t* _findLongOption(const char* arg){
    const ArgumentDefinition_t* ret = NULL;
    if(arg[0] == '-' && arg[1] == '-' && arg[2] != '-'){
        const char* pure = &arg[2];
        size_t n;
        for(n = 0; n < _argumentArrayCount; n++){
            if(_argumentArray[n].longOption != NULL){
                if(strcmp(pure, _argumentArray[n].longOption) == 0){
                    return &_argumentArray[n];
                }
            }
        }
    }
    return ret;
}

static void _argEval_displayHelp(FILE* helpOut, const char* program_path){
    size_t n;
    printf(_intro, program_path);
    printf("\n");
    for(n = 0; n < _argumentArrayCount; n++){
        fprintf(helpOut, "    ");
        if(_argumentArray[n].shortOption != NULL){
            fprintf(helpOut, _shortOptionHelp, _argumentArray[n].shortOption);
        }else{
            fprintf(helpOut, _shortEmptyOptionHelp, "");
        }
        fprintf(helpOut, "  ");
        if(_argumentArray[n].longOption != NULL){
            fprintf(helpOut, _longOptionHelp, _argumentArray[n].longOption);
        }else{
            fprintf(helpOut, _longEmptyOptionHelp, "");
        }
        fprintf(helpOut, " ");
        if(_maxArgumentCount > 0){
            int argCount, argLen;
            const char* argName;
            if(_argumentArray[n].longOption != NULL){
                argName = _argumentArray[n].longOption;
            }else if(_argumentArray[n].shortOption != NULL){
                argName = _argumentArray[n].shortOption;
            }else{
                argName = "arg";
            }
            argLen = strlen(argName) + 4;
            for(argCount = 0; argCount < _argumentArray[n].argumentCount; argCount++){
                char argText[argLen];
                sprintf(argText, "%%%s%%", argName);
                fprintf(helpOut, _argumentHelp, argText);
            }
            for(; argCount < _maxArgumentCount; argCount++){
                fprintf(helpOut, _argumentEmptyHelp, "");
            }
        }
        fprintf(helpOut, "    ");
        if(_argumentArray[n].helpText != NULL){
            if(_argumentArray[n].variablePointer != NULL && NULL != strstr(_argumentArray[n].helpText, "{}")){
                int helpTestLen = strlen(_argumentArray[n].helpText);
                char tmp[helpTestLen +10];
                memcpy(tmp, _argumentArray[n].helpText, helpTestLen +1);
                
                char* split = strstr(tmp, "{}");
                split[0] = '\0';
                split = &split[2];

                fprintf(helpOut, "%s", tmp);

                switch(_argumentArray[n].variableType){
                    case vType_callback:fprintf(helpOut, "{}"); break;
                    case vType_string:
                    {
                        char* value = *((char**)_argumentArray[n].variablePointer);
                        if(value == NULL){
                            fprintf(helpOut, "{null}");
                        }else{
                            fprintf(helpOut, "\"%s\"", value);
                        }
                    } break;
                    case vType_integer: fprintf(helpOut, "%d", *((int*)_argumentArray[n].variablePointer)); break;
                    case vType_float:   fprintf(helpOut, "%f", *((float*)_argumentArray[n].variablePointer)); break;
                    case vType_double:  fprintf(helpOut, "%f", *((double*)_argumentArray[n].variablePointer)); break;
                    default:            fprintf(helpOut, "{no value}"); break;
                }

                fprintf(helpOut, "%s\n", split);
            }else{
                fprintf(helpOut, "%s\n", _argumentArray[n].helpText);
            }
        }else{
            fprintf(helpOut, "%s\n", "");
        }
    }
    exit(EXIT_SUCCESS);
}