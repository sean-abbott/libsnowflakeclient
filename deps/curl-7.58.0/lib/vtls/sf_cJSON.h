/*
  Copyright (c) 2009-2017 Dave Gamble and snowflake_libcurl_cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef snowflake_libcurl_cJSON__h
#define snowflake_libcurl_cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

/* project version */
#define SNOWFLAKE_LIBCURL_CJSON_VERSION_MAJOR 1
#define SNOWFLAKE_LIBCURL_CJSON_VERSION_MINOR 7
#define SNOWFLAKE_LIBCURL_CJSON_VERSION_PATCH 4

#include <stddef.h>

/* snowflake_libcurl_cJSON Types: */
#define snowflake_libcurl_cJSON_Invalid (0)
#define snowflake_libcurl_cJSON_False  (1 << 0)
#define snowflake_libcurl_cJSON_True   (1 << 1)
#define snowflake_libcurl_cJSON_NULL   (1 << 2)
#define snowflake_libcurl_cJSON_Number (1 << 3)
#define snowflake_libcurl_cJSON_String (1 << 4)
#define snowflake_libcurl_cJSON_Array  (1 << 5)
#define snowflake_libcurl_cJSON_Object (1 << 6)
#define snowflake_libcurl_cJSON_Raw    (1 << 7) /* raw json */

#define snowflake_libcurl_cJSON_IsReference 256
#define snowflake_libcurl_cJSON_StringIsConst 512

/* The snowflake_libcurl_cJSON structure: */
typedef struct snowflake_libcurl_cJSON
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct snowflake_libcurl_cJSON *next;
    struct snowflake_libcurl_cJSON *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct snowflake_libcurl_cJSON *child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==snowflake_libcurl_cJSON_String  and type == snowflake_libcurl_cJSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use snowflake_libcurl_cJSON_SetNumberValue instead */
    int valueint;
    /* The item's number, if type==snowflake_libcurl_cJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} snowflake_libcurl_cJSON;

typedef struct snowflake_libcurl_cJSON_Hooks
{
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} snowflake_libcurl_cJSON_Hooks;

typedef int snowflake_libcurl_cJSON_bool;

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif
#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 2 define options:

SNOWFLAKE_LIBCURL_CJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
SNOWFLAKE_LIBCURL_CJSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
SNOWFLAKE_LIBCURL_CJSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the SNOWFLAKE_LIBCURL_CJSON_API_VISIBILITY flag to "export" the same symbols the way SNOWFLAKE_LIBCURL_CJSON_EXPORT_SYMBOLS does

*/

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(SNOWFLAKE_LIBCURL_CJSON_HIDE_SYMBOLS) && !defined(SNOWFLAKE_LIBCURL_CJSON_IMPORT_SYMBOLS) && !defined(SNOWFLAKE_LIBCURL_CJSON_EXPORT_SYMBOLS)
#define SNOWFLAKE_LIBCURL_CJSON_EXPORT_SYMBOLS
#endif

#if defined(SNOWFLAKE_LIBCURL_CJSON_HIDE_SYMBOLS)
#define SNOWFLAKE_LIBCURL_CJSON_PUBLIC(type)   type __stdcall
#elif defined(SNOWFLAKE_LIBCURL_CJSON_EXPORT_SYMBOLS)
#define SNOWFLAKE_LIBCURL_CJSON_PUBLIC(type)   __declspec(dllexport) type __stdcall
#elif defined(SNOWFLAKE_LIBCURL_CJSON_IMPORT_SYMBOLS)
#define SNOWFLAKE_LIBCURL_CJSON_PUBLIC(type)   __declspec(dllimport) type __stdcall
#endif
#else /* !WIN32 */
#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(SNOWFLAKE_LIBCURL_CJSON_API_VISIBILITY)
#define SNOWFLAKE_LIBCURL_CJSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define SNOWFLAKE_LIBCURL_CJSON_PUBLIC(type) type
#endif
#endif

/* Limits how deeply nested arrays/objects can be before snowflake_libcurl_cJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef SNOWFLAKE_LIBCURL_CJSON_NESTING_LIMIT
#define SNOWFLAKE_LIBCURL_CJSON_NESTING_LIMIT 1000
#endif

/* returns the version of snowflake_libcurl_cJSON as a string */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(const char*) snowflake_libcurl_cJSON_Version(void);

/* Supply malloc, realloc and free functions to snowflake_libcurl_cJSON */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_InitHooks(snowflake_libcurl_cJSON_Hooks* hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of snowflake_libcurl_cJSON_Parse (with snowflake_libcurl_cJSON_Delete) and snowflake_libcurl_cJSON_Print (with stdlib free, snowflake_libcurl_cJSON_Hooks.free_fn, or snowflake_libcurl_cJSON_free as appropriate). The exception is snowflake_libcurl_cJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a snowflake_libcurl_cJSON object you can interrogate. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_Parse(const char *value);
/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match snowflake_libcurl_cJSON_GetErrorPtr(). */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_ParseWithOpts(const char *value, const char **return_parse_end, snowflake_libcurl_cJSON_bool require_null_terminated);

/* Render a snowflake_libcurl_cJSON entity to text for transfer/storage. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(char *) snowflake_libcurl_cJSON_Print(const snowflake_libcurl_cJSON *item);
/* Render a snowflake_libcurl_cJSON entity to text for transfer/storage without any formatting. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(char *) snowflake_libcurl_cJSON_PrintUnformatted(const snowflake_libcurl_cJSON *item);
/* Render a snowflake_libcurl_cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(char *) snowflake_libcurl_cJSON_PrintBuffered(const snowflake_libcurl_cJSON *item, int prebuffer, snowflake_libcurl_cJSON_bool fmt);
/* Render a snowflake_libcurl_cJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: snowflake_libcurl_cJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_PrintPreallocated(snowflake_libcurl_cJSON *item, char *buffer, const int length, const snowflake_libcurl_cJSON_bool format);
/* Delete a snowflake_libcurl_cJSON entity and all subentities. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_Delete(snowflake_libcurl_cJSON *c);

/* Returns the number of items in an array (or object). */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(int) snowflake_libcurl_cJSON_GetArraySize(const snowflake_libcurl_cJSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_GetArrayItem(const snowflake_libcurl_cJSON *array, int index);
/* Get item "string" from object. Case insensitive. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_GetObjectItem(const snowflake_libcurl_cJSON * const object, const char * const string);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_GetObjectItemCaseSensitive(const snowflake_libcurl_cJSON * const object, const char * const string);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_HasObjectItem(const snowflake_libcurl_cJSON *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when snowflake_libcurl_cJSON_Parse() returns 0. 0 when snowflake_libcurl_cJSON_Parse() succeeds. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(const char *) snowflake_libcurl_cJSON_GetErrorPtr(void);

/* Check if the item is a string and return its valuestring */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(char *) snowflake_libcurl_cJSON_GetStringValue(snowflake_libcurl_cJSON *item);

/* These functions check the type of an item */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_IsInvalid(const snowflake_libcurl_cJSON * const item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_IsFalse(const snowflake_libcurl_cJSON * const item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_IsTrue(const snowflake_libcurl_cJSON * const item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_IsBool(const snowflake_libcurl_cJSON * const item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_IsNull(const snowflake_libcurl_cJSON * const item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_IsNumber(const snowflake_libcurl_cJSON * const item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_IsString(const snowflake_libcurl_cJSON * const item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_IsArray(const snowflake_libcurl_cJSON * const item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_IsObject(const snowflake_libcurl_cJSON * const item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_IsRaw(const snowflake_libcurl_cJSON * const item);

/* These calls create a snowflake_libcurl_cJSON item of the appropriate type. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateNull(void);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateTrue(void);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateFalse(void);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateBool(snowflake_libcurl_cJSON_bool boolean);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateNumber(double num);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateString(const char *string);
/* raw json */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateRaw(const char *raw);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateArray(void);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateObject(void);

/* Create a string where valuestring references a string so
 * it will not be freed by snowflake_libcurl_cJSON_Delete */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateStringReference(const char *string);
/* Create an object/arrray that only references it's elements so
 * they will not be freed by snowflake_libcurl_cJSON_Delete */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateObjectReference(const snowflake_libcurl_cJSON *child);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateArrayReference(const snowflake_libcurl_cJSON *child);

/* These utilities create an Array of count items. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateIntArray(const int *numbers, int count);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateFloatArray(const float *numbers, int count);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateDoubleArray(const double *numbers, int count);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_CreateStringArray(const char **strings, int count);

/* Append item to the specified array/object. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_AddItemToArray(snowflake_libcurl_cJSON *array, snowflake_libcurl_cJSON *item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_AddItemToObject(snowflake_libcurl_cJSON *object, const char *string, snowflake_libcurl_cJSON *item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the snowflake_libcurl_cJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & snowflake_libcurl_cJSON_StringIsConst) is zero before
 * writing to `item->string` */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_AddItemToObjectCS(snowflake_libcurl_cJSON *object, const char *string, snowflake_libcurl_cJSON *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing snowflake_libcurl_cJSON to a new snowflake_libcurl_cJSON, but don't want to corrupt your existing snowflake_libcurl_cJSON. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_AddItemReferenceToArray(snowflake_libcurl_cJSON *array, snowflake_libcurl_cJSON *item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_AddItemReferenceToObject(snowflake_libcurl_cJSON *object, const char *string, snowflake_libcurl_cJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_DetachItemViaPointer(snowflake_libcurl_cJSON *parent, snowflake_libcurl_cJSON * const item);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_DetachItemFromArray(snowflake_libcurl_cJSON *array, int which);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_DeleteItemFromArray(snowflake_libcurl_cJSON *array, int which);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_DetachItemFromObject(snowflake_libcurl_cJSON *object, const char *string);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_DetachItemFromObjectCaseSensitive(snowflake_libcurl_cJSON *object, const char *string);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_DeleteItemFromObject(snowflake_libcurl_cJSON *object, const char *string);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_DeleteItemFromObjectCaseSensitive(snowflake_libcurl_cJSON *object, const char *string);

/* Update array items. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_InsertItemInArray(snowflake_libcurl_cJSON *array, int which, snowflake_libcurl_cJSON *newitem); /* Shifts pre-existing items to the right. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_ReplaceItemViaPointer(snowflake_libcurl_cJSON * const parent, snowflake_libcurl_cJSON * const item, snowflake_libcurl_cJSON * replacement);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_ReplaceItemInArray(snowflake_libcurl_cJSON *array, int which, snowflake_libcurl_cJSON *newitem);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_ReplaceItemInObject(snowflake_libcurl_cJSON *object,const char *string,snowflake_libcurl_cJSON *newitem);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_ReplaceItemInObjectCaseSensitive(snowflake_libcurl_cJSON *object,const char *string,snowflake_libcurl_cJSON *newitem);

/* Duplicate a snowflake_libcurl_cJSON item */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON *) snowflake_libcurl_cJSON_Duplicate(const snowflake_libcurl_cJSON *item, snowflake_libcurl_cJSON_bool recurse);
/* Duplicate will create a new, identical snowflake_libcurl_cJSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two snowflake_libcurl_cJSON items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON_bool) snowflake_libcurl_cJSON_Compare(const snowflake_libcurl_cJSON * const a, const snowflake_libcurl_cJSON * const b, const snowflake_libcurl_cJSON_bool case_sensitive);


SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_Minify(char *json);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON*) snowflake_libcurl_cJSON_AddNullToObject(snowflake_libcurl_cJSON * const object, const char * const name);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON*) snowflake_libcurl_cJSON_AddTrueToObject(snowflake_libcurl_cJSON * const object, const char * const name);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON*) snowflake_libcurl_cJSON_AddFalseToObject(snowflake_libcurl_cJSON * const object, const char * const name);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON*) snowflake_libcurl_cJSON_AddBoolToObject(snowflake_libcurl_cJSON * const object, const char * const name, const snowflake_libcurl_cJSON_bool boolean);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON*) snowflake_libcurl_cJSON_AddNumberToObject(snowflake_libcurl_cJSON * const object, const char * const name, const double number);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON*) snowflake_libcurl_cJSON_AddStringToObject(snowflake_libcurl_cJSON * const object, const char * const name, const char * const string);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON*) snowflake_libcurl_cJSON_AddRawToObject(snowflake_libcurl_cJSON * const object, const char * const name, const char * const raw);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON*) snowflake_libcurl_cJSON_AddObjectToObject(snowflake_libcurl_cJSON * const object, const char * const name);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(snowflake_libcurl_cJSON*) snowflake_libcurl_cJSON_AddArrayToObject(snowflake_libcurl_cJSON * const object, const char * const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define snowflake_libcurl_cJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the snowflake_libcurl_cJSON_SetNumberValue macro */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(double) snowflake_libcurl_cJSON_SetNumberHelper(snowflake_libcurl_cJSON *object, double number);
#define snowflake_libcurl_cJSON_SetNumberValue(object, number) ((object != NULL) ? snowflake_libcurl_cJSON_SetNumberHelper(object, (double)number) : (number))

/* Macro for iterating over an array or object */
#define snowflake_libcurl_cJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with snowflake_libcurl_cJSON_InitHooks */
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void *) snowflake_libcurl_cJSON_malloc(size_t size);
SNOWFLAKE_LIBCURL_CJSON_PUBLIC(void) snowflake_libcurl_cJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
