Philosophy
=====
Most Json parsers come with a bunch of functions for parsing and extracting the Json data.They even require a lot of additional memory(dynamic memory or external buffers).Why so complicated?

This parser is designed to be very simple to use and use as little memory as possible.The only required memory is the Json data itself.


Features
=====
* lightweight, no additional memory required(no dynamic memory allocation, no external buffers) 
* no dependencies(only standart libraries)
* simple to use

Importing in your project
====
Download `JsonParser.c`, `JsonParser.h`, include them in your project, done.

Usage
====
* String values:
```
typedef struct {

//pointer to the value in the json itself
    char* data;
//length of the value
    size_t len;
    
}JsonString;
```
* Number values:
```
double
```
* Array and Object values
```
typedef struct {

//pointer to the value in the json itself
    char* data;
//length of the value
    size_t len;
//number of elements in the value, currently only for arrays
    size_t size;
    
}JsonComplex;
```
* Boolean values:
```
unsigned char
```

All values are described by `JsonType`:
```
typedef enum {

    JSON_STRING,
    JSON_NUMBER,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_BOOLEAN,
    JSON_NULL,
    JSON_TYPES

}JsonType;
```
Object values are extracted with `jsonObjectGet...()`.

Array elements are extracted with `jsonArrayGet...()`.

Example
===
```
    JsonComplex root = {0};
    root.data = "{  \"string\"  :  \"some string\",    "
                        "   \"object\"  :  {                   "
                        "         \"boolean\"  :  false        "
                        "   },                                 "
                        "   \"number\"  :  1234,               "
                        "   \"array\"   :  [                   "
                        "          12.34,                      "
                        "          0.0023                      "
                        "   ]                                  "
                        "}";
    root.len = strlen(root.data);

    JsonString string;
    jsonObjectGetString(&root, "string", &string);
    printf("\nstring: %.*s", string.len, string.data);

    JsonComplex object;
    jsonObjectGetObject(&root, "object", &object);
    printf("\nobject: %.*s", object.len, object.data);

    unsigned char boolean;
    jsonObjectGetBoolean(&object, "boolean", &boolean);
    printf("\nboolean: %d", boolean);

    double number;
    jsonObjectGetNumber(&root, "number", &number);
    printf("\nnumber: %f", number);

    JsonComplex array;
    jsonObjectGetArray(&root, "array", &array);
    printf("\narray: %.*s, size; %d", array.len, array.data, array.size);

    double index0;
    jsonArrayGetNumber(&array, 0, &index0);
    printf("\narray[0]: %f", index0);

    double index1;
    jsonArrayGetNumber(&array, 1, &index1);
    printf("\narray[1]: %f", index1);
```
**note**: error handling is skipped for simlpicity
