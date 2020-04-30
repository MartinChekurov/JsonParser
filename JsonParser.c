#include"JsonParser.h"
#include<string.h>
#include <stdio.h>
#include <stdlib.h>

typedef union {

    JsonComplex complex;
    JsonString string;
    double number;
    unsigned char boolean;

}JsonValue;

typedef struct {

    JsonComplex in;
    JsonValue* out;
    JsonString key;
    size_t arrayIndex;
    size_t arraySize;
    JsonType wrapper;
    JsonType type;
    char objectRecursion;
    char arrayRecursion;
    char found;
    char getIndex;
    char keyMatch;
    char isKey;

}JsonParser;

static JsonError jsonParse(JsonParser* parser);

static JsonError jsonSkipSpaces(JsonParser* parser)
{
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    while(parser->in.len && parser->in.data && *parser->in.data == ' ') {
        parser->in.data++;
        parser->in.len--;
    }
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    return JSON_NO_ERR;
}

static JsonError jsonCompareStrings(JsonString* str1, JsonString* str2)
{   
    if ((str1->len == str2->len) &&
        !strncmp(str1->data, str2->data, str2->len)) {
        return JSON_NO_ERR;        
    }
    return JSON_ERR;
}

static JsonError jsonGetString(JsonComplex* in, JsonString* out)
{
    if (!in->len || !*in->data) {
        return JSON_ERR;        
    }
    out->data = in->data;
    while(in->len && *in->data && *in->data != '\"') {
        out->len++;
        in->data++;
        in->len--;
    }
    if (!out->len || !in->len || !*in->data) {
        return JSON_ERR;
    }
    if (!--in->len || !*++in->data) {
        return JSON_ERR;        
    }
    return JSON_NO_ERR;
}

static JsonError jsonParseKey(JsonParser* parser, JsonString* key)
{
    JsonError status = JSON_NO_ERR;
    JsonString string = {0};
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    parser->in.data++;
    parser->in.len--;
    status = jsonGetString(&parser->in, &string);
    if (status != JSON_NO_ERR) {
        return status;
    }    
    if (parser->objectRecursion == 1) {
         *key = string;
    }
    status = jsonSkipSpaces(parser);
    if (status != JSON_NO_ERR) {
        return status;
    }
    if (*parser->in.data != ':') {
        return JSON_ERR;    
    } else {
        if (!--parser->in.len || !*++parser->in.data) {
            return JSON_ERR;
        }
        status = jsonSkipSpaces(parser);
        if (status != JSON_NO_ERR) {
            return status;
        }
    }
    return JSON_NO_ERR;
}

static JsonError jsonParseString(JsonParser* parser, unsigned char match)
{
    JsonError status = JSON_NO_ERR;
    JsonString string = {0};
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    parser->in.data++;
    parser->in.len--;
    status = jsonGetString(&parser->in, &string);
    if (status != JSON_NO_ERR) {
        return status;
    }
    if (match) {
        parser->out->string = string;
        parser->found = 1;
    }
    return JSON_NO_ERR;
}

static JsonError jsonParseObject(JsonParser* parser, unsigned char match)
{
    JsonError status = JSON_NO_ERR;
    JsonType currWrapper = JSON_TYPES;
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    if (match) {
        parser->out->complex.data = parser->in.data;
        parser->out->complex.len = 1;
    }
    currWrapper = parser->wrapper;
    parser->in.data++;
    parser->in.len--;
    parser->objectRecursion++;
    parser->wrapper = JSON_OBJECT;
    status = jsonParse(parser);
    parser->objectRecursion--;
    parser->wrapper = currWrapper;
    if (status != JSON_NO_ERR) {
        return status;
    }
    if (match) {
        parser->out->complex.len = parser->in.data - parser->out->complex.data;
        parser->found = 1;
    }
    return JSON_NO_ERR;
}

static JsonError jsonParseArray(JsonParser* parser, unsigned char match)
{
    JsonError status = JSON_NO_ERR;
    JsonType currWrapper = JSON_TYPES;
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    if (match) {
        parser->out->complex.data = parser->in.data;
        parser->out->complex.len = 1;
    }
    currWrapper = parser->wrapper;
    parser->in.data++;
    parser->in.len--;
    parser->arrayRecursion++;
    parser->wrapper = JSON_ARRAY;  
    status = jsonParse(parser);
    parser->arrayRecursion--;
    parser->wrapper = currWrapper;
    if (status != JSON_NO_ERR) {
        return status;
    }
    if (match) {
        parser->out->complex.len = parser->in.data - parser->out->complex.data;
        parser->found = 1;
    }  
    return JSON_NO_ERR;
}

static JsonError jsonParseNumber(JsonParser* parser, unsigned char match)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value = {0};
    char *end = NULL;
    double number = 0;
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    number = strtod(parser->in.data, &end);
    if (parser->in.data == end) {
        return JSON_ERR;
    }
    parser->in.len -= end - parser->in.data;
    parser->in.data = end;
    if (match) {
        parser->out->number = number;
        parser->found = 1;
    }
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    return JSON_NO_ERR;
}

static JsonError jsonParseBoolean(JsonParser* parser, unsigned char match)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value = {0};
    const unsigned char* true = "true";
    const unsigned char* false = "false";
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    if (*parser->in.data == 't' && parser->in.len > strlen(true)) {
        if (!strncmp(parser->in.data, "true", strlen(true))) {
            if (match) {
                parser->out->boolean = 1;
                parser->found = 1;
                return JSON_NO_ERR;
            }
            parser->in.len -= strlen(true);
            parser->in.data += strlen(true);
        } else {
            return JSON_ERR;
        }
    } else if (*parser->in.data == 'f' && parser->in.len > strlen(false)) {
        if (!strncmp(parser->in.data, "false", strlen(false))) {
            if (match) {
                parser->out->boolean = 0;
                parser->found = 1;
                return JSON_NO_ERR;
            }
            parser->in.len -= strlen(false);
            parser->in.data += strlen(false);
        } else {
            return JSON_ERR;
        }
    } else {
        return JSON_ERR;
    }
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    return JSON_NO_ERR;
}

static JsonError jsonParseObjectValue(JsonParser* parser, JsonString* key, JsonType type)
{
    JsonError status = JSON_NO_ERR;
    unsigned char match = 0;
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    if (parser->objectRecursion == 1 &&
        jsonCompareStrings(&parser->key, key) == JSON_NO_ERR &&
        parser->type == type) {
        match = 1;
        parser->keyMatch = 1;
    }
    switch(type) {
        case JSON_OBJECT:
            status = jsonParseObject(parser, match);
            if (status != JSON_NO_ERR) {
                return status;
            }
            break;
        case JSON_ARRAY:
            status = jsonParseArray(parser, match);
            if (status != JSON_NO_ERR) {
                return status;
            }
            break;
        case JSON_STRING:
            status = jsonParseString(parser, match);
            if (status != JSON_NO_ERR) {
                return status;
            }
            break;
        case JSON_NUMBER:
            status = jsonParseNumber(parser, match);
            if (status != JSON_NO_ERR) {
                return status;
            }
            break;        
        case JSON_BOOLEAN:
            status = jsonParseBoolean(parser, match);
            if (status != JSON_NO_ERR) {
                return status;
            }
            break;       
    }
    if (parser->found) {
        return JSON_NO_ERR;
    }
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    status = jsonSkipSpaces(parser);
    if (status != JSON_NO_ERR) {
        return status;
    }
    if (*parser->in.data == ',') {
        if (!--parser->in.len || !*++parser->in.data) {
            return JSON_ERR;
        }
        status = jsonSkipSpaces(parser);
        if (status != JSON_NO_ERR) {
            return status;
        }
        if (*parser->in.data != '\"') {
            return JSON_ERR;
        }
    } else if (*parser->in.data != '}') {
        return JSON_ERR;
    }
    return JSON_NO_ERR;
}

static JsonError jsonParseArrayValue(JsonParser* parser, JsonType type)
{
    JsonError status = JSON_NO_ERR;
    unsigned char match = 0;
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    if (parser->arrayRecursion == 1 && (parser->keyMatch || parser->getIndex)) {
        if (parser->arrayIndex == parser->arraySize &&
            parser->getIndex) {
            match = 1;
        }
        parser->arraySize++;
    }
    switch(type) {
        case JSON_OBJECT:
            status = jsonParseObject(parser, match);
            if (status != JSON_NO_ERR) {
                return status;
            }
            break;
        case JSON_ARRAY:
            status = jsonParseArray(parser, match);
            if (status != JSON_NO_ERR) {
                return status;
            }
            break;
        case JSON_STRING:
            status = jsonParseString(parser, match);
            if (status != JSON_NO_ERR) {
                return status;
            }
            break;
        case JSON_NUMBER:
            status = jsonParseNumber(parser, match);
            if (status != JSON_NO_ERR) {
                return status;
            }
            break;   
        case JSON_BOOLEAN:
            status = jsonParseBoolean(parser, match);
            if (status != JSON_NO_ERR) {
                return status;
            }
            break;    
    }
    if (parser->found) {
        return JSON_NO_ERR;
    }
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    status = jsonSkipSpaces(parser);
    if (status != JSON_NO_ERR) {
        return status;
    }
    if (*parser->in.data == ',') {
        if (!--parser->in.len || !*++parser->in.data) {
            return JSON_ERR;
        }
        status = jsonSkipSpaces(parser);
        if (status != JSON_NO_ERR) {
            return status;
        }
        // if (*parser->in.data != '[') {
        //     return JSON_ERR;
        // }
    } else if (*parser->in.data != ']') {
        return JSON_ERR;
    }
    return JSON_NO_ERR;
}

static JsonError jsonParseValue(JsonParser* parser, JsonString* key, JsonType type)
{
    JsonError status = JSON_NO_ERR;
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    if (parser->wrapper == JSON_OBJECT) {
        status = jsonParseObjectValue(parser, key, type);
        if (status != JSON_NO_ERR) {
            return status;
        }
        parser->isKey = 1;
    } else if (parser->wrapper == JSON_ARRAY) {
        status = jsonParseArrayValue(parser, type);
        if (status != JSON_NO_ERR) {
            return status;
        }
    } else {
        status = JSON_ERR;
    }
    return status;
}

static JsonError jsonParse(JsonParser* parser)
{
    JsonError status = JSON_NO_ERR;
    JsonString key = {0};
    if (!parser->in.len || !*parser->in.data) {
        return JSON_ERR;
    }
    parser->isKey = 1;
    status = jsonSkipSpaces(parser);
    if (status != JSON_NO_ERR) {
        return status;
    }
    while(parser->in.len && *parser->in.data) {
        switch(*parser->in.data) {
            case '\"':;
                if (parser->wrapper == JSON_OBJECT) {
                    if (parser->isKey) {
                        status = jsonParseKey(parser, &key);
                        if (status != JSON_NO_ERR) {
                            return status;
                        }
                        parser->isKey = 0;
                    } else {
                        status = jsonParseObjectValue(parser, &key, JSON_STRING);
                        if (status != JSON_NO_ERR) {
                            return status;
                        }
                        parser->isKey = 1;
                    }
                } else if (parser->wrapper == JSON_ARRAY) {
                    status = jsonParseArrayValue(parser, JSON_STRING);
                    if (status != JSON_NO_ERR) {
                        return status;
                    }
                } else {
                    status = JSON_ERR;
                }
                break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                status = jsonParseValue(parser, &key, JSON_NUMBER);
                if (status != JSON_NO_ERR) {
                    return status;
                }      
                break;
            case 't': case 'f':
                status = jsonParseValue(parser, &key, JSON_BOOLEAN);
                if (status != JSON_NO_ERR) {
                    return status;
                }      
                break;           
            case '[':
                status = jsonParseValue(parser, &key, JSON_ARRAY);
                if (status != JSON_NO_ERR) {
                    return status;
                }
                break;
            case ']':
                parser->in.data++;
                parser->in.len--;
                return JSON_NO_ERR;
            case '{':
                status = jsonParseValue(parser, &key, JSON_OBJECT);
                if (status != JSON_NO_ERR) {
                    return status;
                }
                break;
            case '}':
                parser->in.data++;
                parser->in.len--;
                return JSON_NO_ERR;
            default:
                return JSON_ERR;
        }
        if (parser->found) {
            return JSON_NO_ERR;
        }  
    }
    return JSON_ERR;
}

static JsonError jsonObjectGetValue(JsonComplex* object, char* key, JsonType type, JsonValue* value)
{
    JsonParser parser = {0};
    JsonError status = JSON_NO_ERR;
    if (!object || !object->data || !object->len ||
        !key || !strlen(key) || !value ||
        *object->data != '{') {
        return JSON_ERR;
    }
    memset(value, 0, sizeof(*value));
    parser.in = *object;
    parser.out = value;
    parser.key.data = key;
    parser.key.len = strlen(key);
    parser.wrapper = JSON_OBJECT;
    parser.type = type;
    status = jsonParse(&parser);
    if (type == JSON_ARRAY) {
        parser.out->complex.size = parser.arraySize;
    }
    return status;
}

static JsonError jsonArrayGetValueIndex(JsonComplex* array, size_t index, JsonType type, JsonValue* element)
{
    JsonParser parser = {0};
    JsonError status = JSON_NO_ERR;
    if (!array || !array->data || !array->len ||
        index >= array->size || !element ||
        *array->data != '[') {
        return JSON_ERR;
    }
    memset(element, 0, sizeof(*element));
    parser.in = *array;
    parser.out = element;
    parser.wrapper = JSON_ARRAY;
    parser.type = type;
    parser.arrayIndex = index;
    parser.getIndex = 1;
    status = jsonParse(&parser);
    if (type == JSON_ARRAY) {
        parser.out->complex.size = parser.arraySize;
    } else {
        parser.out->complex.size = 1;
    }
    return status;
}

JsonError jsonObjectGetString(JsonComplex* object, char* key, JsonString* string)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value;
    if (!string) {
        return JSON_ERR;
    }
    status = jsonObjectGetValue(object, key, JSON_STRING, &value);
    if (status != JSON_NO_ERR) {
        return status;
    }
    *string = value.string;
    return status;
}

JsonError jsonObjectGetNumber(JsonComplex* object, char* key, double* number)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value;
    if (!number) {
        return JSON_ERR;
    }
    status = jsonObjectGetValue(object, key, JSON_NUMBER, &value);
    if (status != JSON_NO_ERR) {
        return status;
    }
    *number = value.number;
    return status;    
}

JsonError jsonObjectGetObject(JsonComplex* object, char* key, JsonComplex* obj)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value;
    if (!obj) {
        return JSON_ERR;
    }
    status = jsonObjectGetValue(object, key, JSON_OBJECT, &value);
    if (status != JSON_NO_ERR) {
        return status;
    }
    *obj = value.complex;
    return status;       
}

JsonError jsonObjectGetArray(JsonComplex* object, char* key, JsonComplex* array)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value;
    if (!array) {
        return JSON_ERR;
    }
    status = jsonObjectGetValue(object, key, JSON_ARRAY, &value);
    if (status != JSON_NO_ERR) {
        return status;
    }
    *array = value.complex;
    return status;       
}

JsonError jsonObjectGetBoolean(JsonComplex* object, char* key, unsigned char* boolean)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value;
    if (!boolean) {
        return JSON_ERR;
    }
    status = jsonObjectGetValue(object, key, JSON_BOOLEAN, &value);
    if (status != JSON_NO_ERR) {
        return status;
    }
    *boolean = value.boolean;
    return status;      
}

JsonError jsonArrayGetString(JsonComplex* object, size_t index, JsonString* string)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value;
    if (!string) {
        return JSON_ERR;
    }
    status = jsonArrayGetValueIndex(object, index, JSON_STRING, &value);
    if (status != JSON_NO_ERR) {
        return status;
    }
    *string = value.string;
    return status;
}

JsonError jsonArrayGetNumber(JsonComplex* object, size_t index, double* number)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value;
    if (!number) {
        return JSON_ERR;
    }
    status = jsonArrayGetValueIndex(object, index, JSON_NUMBER, &value);
    if (status != JSON_NO_ERR) {
        return status;
    }
    *number = value.number;
    return status;    
}

JsonError jsonArrayGetObject(JsonComplex* object, size_t index, JsonComplex* obj)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value;
    if (!obj) {
        return JSON_ERR;
    }
   status = jsonArrayGetValueIndex(object, index, JSON_OBJECT, &value);
    if (status != JSON_NO_ERR) {
        return status;
    }
    *obj = value.complex;
    return status;       
}

JsonError jsonArrayGetArray(JsonComplex* object, size_t index, JsonComplex* array)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value;
    if (!array) {
        return JSON_ERR;
    }
   status = jsonArrayGetValueIndex(object, index, JSON_ARRAY, &value);
    if (status != JSON_NO_ERR) {
        return status;
    }
    *array = value.complex;
    return status;       
}

JsonError jsonArrayGetBoolean(JsonComplex* object, size_t index, unsigned char* boolean)
{
    JsonError status = JSON_NO_ERR;
    JsonValue value;
    if (!boolean) {
        return JSON_ERR;
    }
    status = jsonArrayGetValueIndex(object, index, JSON_BOOLEAN, &value);
    if (status != JSON_NO_ERR) {
        return status;
    }
    *boolean = value.boolean;
    return status;      
}