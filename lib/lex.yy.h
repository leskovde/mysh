#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

#ifndef YY_STRUCT_YY_BUFFER_STATE
#define YY_STRUCT_YY_BUFFER_STATE

struct yy_buffer_state
{
    FILE *yy_input_file;
    char *yy_ch_buf;
    char *yy_buf_pos;
    int yy_buf_size;
    int yy_n_chars;
    int yy_is_our_buffer;
    int yy_is_interactive;
    int yy_at_bol;
    int yy_bs_lineno;
    int yy_bs_column;
    int yy_fill_buffer;
    int yy_buffer_status;
    #define YY_BUFFER_NEW 0
    #define YY_BUFFER_NORMAL 1
    #define YY_BUFFER_EOF_PENDING 2
};

#endif /* !YY_STRUCT_YY_BUFFER_STATE */
