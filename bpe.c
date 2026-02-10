#define CHAOS_IMPLEMENTATION
#include <chaos.h>

typedef struct {
  int *items;
  size_t count;
  size_t capacity;
} Tokens;

typedef struct {
  int left;
  int right;
  int token;
} Merge;

typedef struct {
  Merge *items;
  size_t count;
  size_t capacity;
} Merges;

static void free_table(Table *tb) {
  if (!tb->items)
    return;

  for (size_t i = 0; i < tb->count; ++i) {
    chaos_Bucket *b = &tb->items[i];
    for (size_t j = 0; j < b->count; ++j) {
      free(b->items[j].value);
    }
    free(b->items);
  }
  free(tb->items);

  tb->items = NULL;
  tb->count = 0;
  tb->capacity = 0;
}

static void decode_token(int token, Merges *merges, String_Builder *out) {
  if (token < 256) {
    da_append(out, (char)token);
    return;
  }

  for (size_t i = merges->count; i-- > 0;) {
    if (merges->items[i].token == token) {
      decode_token(merges->items[i].left, merges, out);
      decode_token(merges->items[i].right, merges, out);
      return;
    }
  }
}

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "Usage %s <file>\n", argv[0]);
    return 1;
  }

  String_Builder sb = {0};
  read_file(argv[1], &sb);

  Tokens tokens = {0};
  for (size_t i = 0; i < sb.count; ++i) {
    da_append(&tokens, (unsigned char)sb.items[i]);
  }

  Merges merges = {0};
  int next_token = 256;

  for (;;) {
    Table tb = {0};

    for (size_t i = 0; i + 1 < tokens.count; ++i) {
      char *key = strdup(
          temp_sprintf("%03d%03d", tokens.items[i], tokens.items[i + 1]));
      table_append(&tb, key, 6);
    }

    chaos_KV *best = NULL;
    for (size_t i = 0; i < tb.count; ++i) {
      chaos_Bucket *b = &tb.items[i];
      for (size_t j = 0; j < b->count; ++j) {
        if (!best || b->items[j].freq > best->freq) {
          best = &b->items[j];
        }
      }
    }

    if (!best || best->freq <= 1) {
      free_table(&tb);
      break;
    }

    char a_buf[4] = {0};
    char b_buf[4] = {0};
    memcpy(a_buf, best->value, 3);
    memcpy(b_buf, best->value + 3, 3);

    int A = atoi(a_buf);
    int B = atoi(b_buf);

    Tokens next = {0};
    for (size_t i = 0; i < tokens.count;) {
      if (i + 1 < tokens.count && tokens.items[i] == A &&
          tokens.items[i + 1] == B) {
        da_append(&next, next_token);
        i += 2;
      } else {
        da_append(&next, tokens.items[i]);
        i += 1;
      }
    }

    da_append(&merges, ((Merge){
                           .left = A,
                           .right = B,
                           .token = next_token,
                       }));

    free(tokens.items);
    tokens = next;
    next_token++;

    free_table(&tb);
  }

  printf("Final token count: %zu\n", tokens.count);
  printf("Vocab size: %d\n", next_token);
  printf("Merges: %zu\n", merges.count);

  String_Builder decoded = {0};
  for (size_t i = 0; i < tokens.count; ++i) {
    decode_token(tokens.items[i], &merges, &decoded);
  }
  sb_append_null(&decoded);

  printf("%s\n", decoded.items);
}
