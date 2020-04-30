/*
*	Author: Martin Chekurov
*/

#ifndef JSON_PARSER_H_
#define JSON_PARSER_H_

#include <stdlib.h>

typedef enum {

    JSON_NO_ERR,
    JSON_ERR,

}JsonError;

typedef enum {

    JSON_STRING,
    JSON_NUMBER,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_BOOLEAN,
    JSON_NULL,
    JSON_TYPES

}JsonType;

typedef struct {

    char* data;
    size_t len;
    
}JsonString;

typedef struct {

    char* data;
    size_t len;
    size_t size;
    
}JsonComplex;

JsonError jsonObjectGetString (JsonComplex* object, char* key, JsonString* string);
JsonError jsonObjectGetNumber (JsonComplex* object, char* key, double* number);
JsonError jsonObjectGetObject (JsonComplex* object, char* key, JsonComplex* obj);
JsonError jsonObjectGetArray  (JsonComplex* object, char* key, JsonComplex* array);
JsonError jsonObjectGetBoolean(JsonComplex* object, char* key, unsigned char* boolean);

JsonError jsonArrayGetString (JsonComplex* object, size_t index, JsonString* string);
JsonError jsonArrayGetNumber (JsonComplex* object, size_t index, double* number);
JsonError jsonArrayGetObject (JsonComplex* object, size_t index, JsonComplex* obj);
JsonError jsonArrayGetArray  (JsonComplex* object, size_t index, JsonComplex* array);
JsonError jsonArrayGetBoolean(JsonComplex* object, size_t index, unsigned char* boolean);

#endif