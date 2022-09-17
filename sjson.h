/* sjson.h - json parser 
 * sleepntsheep 2022 
 *
 * Usage - 
 *     sjsontokarr *toks = sjson_lex(strdup("{\"example\": \"json string\"}"));
 *     ** sjson_lex does modify the string,
 *     so you must use strdup on string literal
 *
 *     sjson *json = sjson_serialize(char *s);
 *
 *     sjson_free(sjson);
 *
 *  sjson type store everything, 
 *

    typedef struct sjson {
        struct sjson *next;
        struct sjson *prev;
        int type;
        struct sjson *childvalue;
        struct sjson *tail;
        double numbervalue;
        char *stringvalue;
        char *key;
        enum sjson_err err;
    } sjson;

 * next and prev is used for doubly linked list in object and array
 * no hashmap is used here, doubly linked list is more simple, despite being O(n) lookup
 * childvalue store the doubly linked list of all child, if type is object or array
 * tail store last child, this is used for sjson_addchild function to be O(1)
 * stringvalue store string if type is string
 * numbervalue store double number if type is number
 *
 * TODO
 *     - finish objectget arrayget objectset arrayset objectdel arraydel
 *     - (done? except unicode )fix escape character annoying problem 
 *         (both serializing and deserializing)
 *
 */

#ifndef SHEEP_SJSON_H
#define SHEEP_SJSON_H
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#define SJSON_TYPES_LIST \
    SJSON_X(SJSON_INVALID),\
    SJSON_X(SJSON_NULL),   \
    SJSON_X(SJSON_FALSE),  \
    SJSON_X(SJSON_TRUE),   \
    SJSON_X(SJSON_NUMBER), \
    SJSON_X(SJSON_STRING), \
    SJSON_X(SJSON_ARRAY),  \
    SJSON_X(SJSON_OBJECT),

#define SJSON_TOKENS_LIST \
    SJSON_X(SJSON_TKINVALID), \
    SJSON_X(SJSON_TKLBRACE), \
    SJSON_X(SJSON_TKRBRACE), \
    SJSON_X(SJSON_TKSTRINGLITERAL), \
    SJSON_X(SJSON_TKNUMBERLITERAL), \
    SJSON_X(SJSON_TKNULL), \
    SJSON_X(SJSON_TKFALSE), \
    SJSON_X(SJSON_TKTRUE), \
    SJSON_X(SJSON_TKLSQUAREBRACKET), \
    SJSON_X(SJSON_TKRSQUAREBRACKET), \
    SJSON_X(SJSON_TKCOMMA), \
    SJSON_X(SJSON_TKCOLON),

#define SJSON_X(a) a
enum sjson_type { SJSON_TYPES_LIST };
enum sjson_tokens { SJSON_TOKENS_LIST };
#undef SJSON_X
#define SJSON_X(a) #a
static const char *sjson_typenames[] = { SJSON_TYPES_LIST };
static const char *sjson_tokennames[] = { SJSON_TOKENS_LIST };
#undef SJSON_X

typedef struct sjsonbuf {
	size_t len, cap;
	char *buf;
} sjsonbuf;

typedef struct sjson {
    int type;
	struct {
		double num;
		char *str;
		struct sjson *child;
		struct sjson *tail;
	} v;
	/* for array & object child */
    struct sjson *next;
    struct sjson *prev;
	/* for object child */
    char *key;
} sjson;

typedef struct sjsontok {
    int type;
    char *start;
    char *end;
} sjsontok;

typedef struct sjsontokarr {
    size_t length, capacity;
    size_t cur;
    sjsontok *a;
} sjsontokarr;

typedef struct sjsonlexer {
	char *start, *end;
	char *c;
	sjsontokarr toks;
} sjsonlexer;

typedef struct sjsonparser {
    size_t i;
    sjsontokarr toks;
} sjsonparser;

#define sjson_foreach(json, iter) \
    for (sjson *iter = (json)->v.child; (iter) != NULL; iter = (iter)->next) 

sjson *sjson_new(int type);
void sjson_free(sjson *json);

sjson *sjson_serialize(char *s, size_t len);
sjsonbuf sjson_deserialize(sjson *json);

void sjsonlexer_init(sjsonlexer *lexer, char *s, size_t len);
bool sjsonlexer_lex(sjsonlexer *lexer);
sjson *sjson_parse(sjsontokarr *toks);

sjson *sjson_array_get(sjson *json, size_t i);
bool sjson_array_set(sjson *json, char *key, sjson *value);
sjson *sjson_object_get(sjson *json, char *key);
bool sjson_object_set(sjson *json, char *key, sjson *value);
//bool sjson_object_del(sjson *json, char *key);
//bool sjson_array_del(sjson *json, char *key, sjson *value);

bool sjson_addchild(sjson *json, sjson *child);
bool sjson_deletechild(sjson *json, sjson *child);

#define sjson_array_push sjson_addchild

void sjson_register_logger(int (*logger)(char*, ...));

static int sjson_logger_dummy(char *fmt, ...) {return 0;}

static int (*logger)(char*, ...) = sjson_logger_dummy;

#endif /* SHEEP_SJSON_H */

#ifdef SHEEP_SJSON_IMPLEMENTATION 

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#define SJSON_LOG(errnum) puts(#errnum) //logger("%s\n", sjson_errnames[errnum])
                                        
static sjsontokarr sjsontokarr_new() {
    return (sjsontokarr) {
        .length = 0,
        .capacity = 4,
        .cur = 0,
        .a = malloc(sizeof(sjsontok) * 4),
    };
}

static void sjsontokarr_push(sjsontokarr *arr, sjsontok tok) {
    if (arr->length == arr->capacity)
        arr->a = realloc(arr->a, sizeof(sjsontok) * (arr->capacity *= 2));
    arr->a[arr->length++] = tok;
}

void sjsontokarr_debug(sjsontokarr arr){ 
    for (int i =0 ;i < arr.length;i++)
    {
        printf("%s\n", sjson_tokennames[arr.a[i].type]);
    }
}

static sjsontok sjsontokarr_advance(sjsontokarr *arr) {
    if (arr->cur < arr->length - 1)
        return arr->a[arr->cur++];
    return (sjsontok){0};
}

static sjsontok sjsontokarr_peek(sjsontokarr *arr) {
    return arr->a[arr->cur];
}

static void sjsonlexer_pushtok(sjsonlexer *lexer, int type, char *start, char *end) {
    sjsontokarr_push(&lexer->toks, (sjsontok){type, start, end});
}

static void sjsonbuf_init(sjsonbuf *buf) {
    buf->cap = 128;
    buf->len = 0;
    buf->buf = malloc(buf->cap);
}

static void sjsonbuf_push(sjsonbuf *buf, void *s, size_t len) {
    while (buf->cap - buf->len <= len + 1)
        buf->cap *= 2;
    buf->buf = realloc(buf->buf, buf->cap);
    memcpy(buf->buf + buf->len, s, len);
    buf->len += len;
    buf->buf[buf->len] = '\x0';
}

void sjson_register_logger(int (*newlogger)(char*, ...)) {
    logger = newlogger;
}

static char sjsonlexer_advance(sjsonlexer *lexer) {
    if (lexer->c <= lexer->end)
        return (lexer->c++)[0];
    return -1;
}

static char sjsonlexer_peek(sjsonlexer *lexer) {
    return (lexer->c)[0];
}

static bool sjsonlexer_isend(sjsonlexer *lexer) {
    return lexer->c >= lexer->end;
}

void sjsonlexer_init(sjsonlexer *lexer, char *s, size_t len) {
    lexer->start = s,
    lexer->end = s + len,
    lexer->c = s,
    lexer->toks = sjsontokarr_new();
}

static bool sjsonlexer_lexnumber(sjsonlexer *lexer) {
    char *numberstart = lexer->c;
    bool didpoint = false, didsign = false;
    sjsonbuf buf;
    sjsonbuf_init(&buf);

    while (!sjsonlexer_isend(lexer)) {
        char c = sjsonlexer_peek(lexer);
        if (!isdigit(c)) {
            if (c == '+' || c == '-') {
                if (didsign)
                    goto bad;
                didsign = true;
            }
            else if (c == '.') {
                if (didpoint) goto bad;
                didpoint = true;
            }
            else {
                break;
            }
        }
        sjsonlexer_advance(lexer);
    }
    lexer->c--;

    /* avoid modification of original source */
    sjsonbuf_push(&buf, numberstart, lexer->c - numberstart + 1);

    sjsonlexer_pushtok(lexer, SJSON_TKNUMBERLITERAL, buf.buf, lexer->c);

    return true;
bad:
    return false;
}

static bool sjsonlexer_lexstring(sjsonlexer *lexer) {
    if (sjsonlexer_advance(lexer) != '\"')
        return false; /* not string, cope */
    char *stringend = lexer->c;

    /* find terminating double quote, need to escape \" */
    while (stringend[0] != '\"') {
        if (stringend > lexer->end)
            goto bad;
        if (stringend[0] == '\\')
            stringend++;
        stringend++;
    }

    /* can't just push new token, escape character reasons */
    sjsonbuf buf;
    sjsonbuf_init(&buf);
    while (sjsonlexer_peek(lexer) != '"'
            && lexer->c < stringend) {
        char ch = sjsonlexer_advance(lexer);
        if (ch == '\\') {
            int advance_ret = sjsonlexer_advance(lexer);
            if (advance_ret == -1)
                return false;
            /* deal with escape character */
            switch (advance_ret) {
                case '\\':
                    sjsonbuf_push(&buf, "\\", 1);
                    break;
                case 'n':
                    sjsonbuf_push(&buf, "\n", 1);
                    break;
                case 'f':
                    sjsonbuf_push(&buf, "\f", 1);
                    break;
                case 'r':
                    sjsonbuf_push(&buf, "\r", 1);
                    break;
                case 't':
                    sjsonbuf_push(&buf, "\t", 1);
                    break;
                case '\"':
                    sjsonbuf_push(&buf, "\"", 1);
                    break;
                case '/':
                    sjsonbuf_push(&buf, "/", 1);
                    break;
                case 'u':
                    /* WILL IMPLEMENT LATER, HATETHIS */
                    break;
            }
        } else {
            sjsonbuf_push(&buf, &ch, 1);
        }
    }
    sjsonlexer_pushtok(lexer, SJSON_TKSTRINGLITERAL, buf.buf, buf.buf + buf.len);
    return true;
bad:
    return false;
}


bool sjsonlexer_lexprimitive(sjsonlexer *lexer) {
    int type;
    size_t len;
    switch (sjsonlexer_peek(lexer)) {
        case 't':
            type = SJSON_TKTRUE;
            len = 4;
            break;
        case 'f':
            type = SJSON_TKFALSE;
            len = 5;
            break;
        case 'n':
            type = SJSON_TKNULL;
            len = 4;
            break;
        default:
            goto bad;
    }
    if (lexer->end - lexer->start + 1 < len)
        goto bad;
    sjsonlexer_pushtok(lexer, type, lexer->c, lexer->c + len - 1);
    lexer->c += len - 1;

    return true;
bad:
    return false;
}

bool sjsonlexer_lex(sjsonlexer *lexer) {
    while (!sjsonlexer_isend(lexer)) {
        switch (sjsonlexer_peek(lexer)) {
            case '[':
                sjsonlexer_pushtok(lexer, SJSON_TKLSQUAREBRACKET, lexer->c, lexer->c);
                break;
            case ']': 
                sjsonlexer_pushtok(lexer, SJSON_TKRSQUAREBRACKET, lexer->c, lexer->c);
                break;
            case '{': 
                sjsonlexer_pushtok(lexer, SJSON_TKLBRACE, lexer->c, lexer->c);
                break;
            case '}': 
                sjsonlexer_pushtok(lexer, SJSON_TKRBRACE, lexer->c, lexer->c);
                break;
            case ',':
                sjsonlexer_pushtok(lexer, SJSON_TKCOMMA, lexer->c, lexer->c);
                break;
            case ':':
                sjsonlexer_pushtok(lexer, SJSON_TKCOLON, lexer->c, lexer->c);
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '+':
            case '-':
                if (!sjsonlexer_lexnumber(lexer))
                    goto bad;
                break;
            case '\"':
                if (!sjsonlexer_lexstring(lexer))
                    goto bad;
                break;
            case 'n':
            case 't':
            case 'f':
                if (!sjsonlexer_lexprimitive(lexer))
                    goto bad;
                break;
            case ' ':
            case '\t':
            case '\n':
                break;
            default:
                logger("unknown char");
                break;
        }
        sjsonlexer_advance(lexer);
    }
    return true;
bad:
    return false;
}

sjson *sjson_new(int type) {
    sjson *json = calloc(1, sizeof(*json));
    json->type = type;
    return json;
}

void sjson_free(sjson *json) {
    if (json->type == SJSON_OBJECT || 
            json->type == SJSON_ARRAY)
        if (json->v.child != NULL)
            sjson_free(json->v.child);
    if (json->next != NULL)
        sjson_free(json->next);
    free(json);
}

static sjson *sjson_parseobject(sjsontokarr *toks) {
    sjson *obj = sjson_new(SJSON_OBJECT);

    if (sjsontokarr_advance(toks).type != SJSON_TKLBRACE)
        goto bad;

    while (sjsontokarr_peek(toks).type != SJSON_TKRBRACE) {
        char *key = sjsontokarr_advance(toks).start;
        sjson *child;
        if (sjsontokarr_advance(toks).type != SJSON_TKCOLON)
            goto bad;
        if ((child = sjson_parse(toks)) == NULL)
            goto bad;
        child->key = key;
        if (!sjson_addchild(obj, child))
            goto bad;
        sjsontok next = sjsontokarr_peek(toks);
        if (next.type == SJSON_TKCOMMA) {
            if (sjsontokarr_advance(toks).type == SJSON_TKINVALID) {
                logger("trailing comma");
                break;
            }
            continue;
        }
        else if (next.type == SJSON_TKRBRACE) {
            break;
        }
        else {
            goto bad;
        }
    }
    return obj;
bad:
    return NULL;
}

static sjson *sjson_parsearray(sjsontokarr *toks) {
    sjson *arr = sjson_new(SJSON_ARRAY);
    
    if (sjsontokarr_advance(toks).type != SJSON_TKLSQUAREBRACKET)
        goto bad;

    while (sjsontokarr_peek(toks).type != SJSON_TKRSQUAREBRACKET) {
        sjson *child = sjson_parse(toks);
        if (child == NULL) {
            logger("error parsing array child");
        } else {
            sjson_addchild(arr, child);
        }
        sjsontok next = sjsontokarr_peek(toks);
        if (next.type == SJSON_TKCOMMA) {
            if (sjsontokarr_advance(toks).type == 0)
                goto bad;
        } else if (next.type != SJSON_TKRSQUAREBRACKET) {
            logger("unknown token while parsing array %d", next.type);
            goto bad;
        }
    }
    return arr;
bad:
    return NULL;
}

sjson *sjson_parse(sjsontokarr *toks) {
    sjson *ret = NULL;
    switch (sjsontokarr_peek(toks).type) {
        case SJSON_TKTRUE:
            ret = sjson_new(SJSON_TRUE);
            break;
        case SJSON_TKFALSE:
            ret = sjson_new(SJSON_FALSE);
            break;
        case SJSON_TKNULL:
            ret = sjson_new(SJSON_NULL);
            break;
        case SJSON_TKLBRACE:
            ret = sjson_parseobject(toks);
            break;
        case SJSON_TKLSQUAREBRACKET:
            ret = sjson_parsearray(toks);
            break;
        case SJSON_TKNUMBERLITERAL:
            ret = sjson_new(SJSON_NUMBER);
            ret->v.num = strtod(sjsontokarr_peek(toks).start, NULL);
            break;
        case SJSON_TKSTRINGLITERAL:
            ret = sjson_new(SJSON_STRING);
            ret->v.str = sjsontokarr_peek(toks).start;
            break;
        default:
            logger("parse: unknown token %s %s", sjson_tokennames, sjsontokarr_peek(toks).type, sjsontokarr_peek(toks).start);
            return NULL;
    }
    sjsontokarr_advance(toks);
    if (ret == NULL)
        ret = sjson_new(SJSON_INVALID);
    return ret;
}

bool sjson_deletechild(sjson *json, sjson *child) {
    if (json->v.child == NULL)
        return false;
    if (json->v.tail == child) {
        if (json->v.child == child)
            json->v.tail = json->v.child = NULL;
        else
            json->v.tail = child->prev;
    } else if (json->v.child == child) {
        json->v.child = child->next;
    }
    if (child->prev)
        child->prev->next = child->next;
    if (child->next)
        child->next->prev = child->prev;
    return false;
}

sjson *sjson_object_get(sjson *json, char *key) {
    if (json->type != SJSON_OBJECT)
        goto bad;
    sjson_foreach(json, iter)
        if (!strcmp(iter->key, key))
            return iter;
bad:
    return NULL;
}

bool sjson_object_del(sjson *json, char *key) {
    if (json->type != SJSON_OBJECT)
        goto bad;
    sjson_foreach(json, iter)
        if (!strcmp(iter->key, key))
            sjson_deletechild(json, iter);
    return true;
bad:
    return false;
}

bool sjson_object_set(sjson *json, char *key, sjson *value) {
    if (json->type != SJSON_OBJECT)
        goto bad;
    sjson_object_del(json, key);
    value->key = key;
    sjson_addchild(json, value);
    return true;
bad:
    return false;
}

bool sjson_addchild(sjson *json, sjson *child) {
    if (child == NULL) {
        goto bad;
    }
    if (json->type != SJSON_OBJECT && json->type != SJSON_ARRAY) {
        goto bad;
    }
    if (json->v.tail == NULL && json->v.child == NULL) {
        json->v.tail = json->v.child = child;
    } else {
        json->v.tail->next = child;
        child->prev = json->v.tail;
        json->v.tail = child;
    }
    return true;
bad:
    logger("failed addchild");
    return false;
}

sjson *sjson_array_get(sjson *json, size_t i) {
    size_t x = 0;
    sjson_foreach(json, iter)
        if (x++ == i)
            return iter;
bad:
    return NULL;
}

sjson *sjson_serialize(char *s, size_t len) {
    sjsonlexer lexer;
    sjson *json;
    sjsonlexer_init(&lexer, s, len);
    if (!sjsonlexer_lex(&lexer))
        goto bad;
    if ((json = sjson_parse(&lexer.toks)) == NULL)
        goto bad;
    return json;
bad:
    free(lexer.toks.a);
    return NULL;
}

sjsonbuf sjson_deserialize(sjson *json) {
    char buf[1024];
    sjsonbuf s;
    sjsonbuf_init(&s);
    switch (json->type) {
        case SJSON_NUMBER:
        {
            size_t len = snprintf(buf, sizeof buf, "%lf", json->v.num);
            sjsonbuf_push(&s, buf, len);
            break;
        }
        case SJSON_STRING:
            sjsonbuf_push(&s, "\"", 1);
            for (char *c = json->v.str; *c; c++) {
                switch (*c) {
                    case '\"':
                        sjsonbuf_push(&s, "\\\"", 2);
                        break;
                    case '\n':
                        sjsonbuf_push(&s, "\\n", 2);
                        break;
                    case '\r':
                        sjsonbuf_push(&s, "\\r", 2);
                        break;
                    case '\f':
                        sjsonbuf_push(&s, "\\f", 2);
                        break;
                    case '\t':
                        sjsonbuf_push(&s, "\\t", 2);
                        break;
                    case '\\':
                        sjsonbuf_push(&s, "\\\\", 2);
                        break;
                    case '/':
                        sjsonbuf_push(&s, "\\/", 2);
                        break;
                    default:
                        sjsonbuf_push(&s, c, 1);
                        break;
                }
            }
            sjsonbuf_push(&s, "\"", 1);
            break;
        case SJSON_NULL:
            sjsonbuf_push(&s, "null", 4);
            break;
        case SJSON_TRUE:
            sjsonbuf_push(&s, "true", 4);
            break;
        case SJSON_FALSE:
            sjsonbuf_push(&s, "false", 5);
            break;
        case SJSON_OBJECT:
            sjsonbuf_push(&s, "{", 1);
            sjson_foreach(json, it) {
                sjsonbuf_push(&s, "\"", 1);
                sjsonbuf_push(&s, it->key, strlen(it->key));
                sjsonbuf_push(&s, "\"", 1);
                sjsonbuf_push(&s, ":", 1);
                sjsonbuf childbuf = sjson_deserialize(it);
                sjsonbuf_push(&s, childbuf.buf, childbuf.len);
                free(childbuf.buf);
                if (it->next != NULL)
                    sjsonbuf_push(&s, ",", 1);
            }
            sjsonbuf_push(&s, "}", 1);
            break;
        case SJSON_ARRAY:
            sjsonbuf_push(&s, "[", 1);
            sjson_foreach(json, it) {
                sjsonbuf childbuf = sjson_deserialize(it);
                sjsonbuf_push(&s, childbuf.buf, childbuf.len);
                free(childbuf.buf);
                if (it->next != NULL)
                    sjsonbuf_push(&s, ",", 1);
            }
            sjsonbuf_push(&s, "]", 1);
            break;
        case SJSON_INVALID:
        default:
            break;
    }
    return s;
}

#endif /* SHEEP_SJSON_IMPLEMENTATION */
