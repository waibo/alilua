#include "main.h"

static char tbuf_4096[4096];

int lua_check_timeout ( lua_State *L )
{
    if ( !lua_isuserdata ( L, 1 ) ) {
        luaL_error ( L, "miss epd!" );
        return 0;
    }

    epdata_t *epd = lua_touserdata ( L, 1 );

    if ( epd->process_timeout == 1 ) {
        epd->keepalive = 0;
        network_be_end ( epd );
        lua_pushstring ( L, "Process Time Out!" );
        lua_error ( L ); /// stop lua script
    }

    return 0;
}

int lua_header ( lua_State *L )
{
    if ( !lua_isuserdata ( L, 1 ) ) {
        luaL_error ( L, "miss epd!" );
        return 0;
    }

    epdata_t *epd = lua_touserdata ( L, 1 );

    if ( lua_gettop ( L ) < 2 ) {
        return 0;
    }

    int t = lua_type ( L, 2 );
    size_t dlen = 0;
    const char *data = NULL;

    if ( t == LUA_TSTRING ) {
        data = lua_tolstring ( L, 2, &dlen );

        if ( stristr ( data, "content-length", dlen ) != data ) {
            network_send_header ( epd, data );
        }

    } else if ( t == LUA_TTABLE ) {
        int len = lua_objlen ( L, 2 ), i = 0;

        for ( i = 0; i < len; i++ ) {
            lua_pushinteger ( L, i + 1 );
            lua_gettable ( L, -2 );

            if ( lua_isstring ( L, -1 ) ) {
                data = lua_tolstring ( L, -1, &dlen );

                if ( stristr ( data, "content-length", dlen ) != data ) {
                    network_send_header ( epd, lua_tostring ( L, -1 ) );
                }
            }

            lua_pop ( L, 1 );
        }
    }

    return 0;
}

int lua_echo ( lua_State *L )
{
    if ( !lua_isuserdata ( L, 1 ) ) {
        luaL_error ( L, "miss epd!" );
        return 0;
    }

    int nargs = lua_gettop ( L );

    if ( nargs < 2 ) {
        luaL_error ( L, "miss content!" );
        return 0;
    }

    size_t len = 0;
    epdata_t *epd = lua_touserdata ( L, 1 );

    if ( lua_istable ( L, 2 ) ) {
        len = lua_calc_strlen_in_table ( L, 2, 2, 0 /* strict */ );

        if ( len < 1 ) {
            return 0;
        }

        char *buf = tbuf_4096;

        if ( len > 4096 ) {
            buf = large_malloc ( len );

            if ( !buf ) {
                return 0;
            }

            lua_copy_str_in_table ( L, 2, buf );
            network_send ( epd, buf, len );
            free ( buf );

        } else {
            lua_copy_str_in_table ( L, 2, buf );
            network_send ( epd, buf, len );
        }

    } else {
        const char *data = NULL;
        int i = 0;

        for ( i = 2; i <= nargs; i++ ) {
            if ( lua_isboolean ( L, i ) ) {
                if ( lua_toboolean ( L, i ) ) {
                    network_send ( epd, "true", 4 );

                } else {
                    network_send ( epd, "false", 5 );
                }

            } else {
                data = lua_tolstring ( L, i, &len );
                network_send ( epd, data, len );
            }
        }
    }

    return 0;
}

int lua_clear_header ( lua_State *L )
{
    if ( !lua_isuserdata ( L, 1 ) ) {
        luaL_error ( L, "miss epd!" );
        return 0;
    }

    epdata_t *epd = lua_touserdata ( L, 1 );
    epd->response_header_length = 0;
    return 0;
}

int lua_sendfile ( lua_State *L )
{
    if ( !lua_isuserdata ( L, 1 ) ) {
        luaL_error ( L, "miss epd!" );
        return 0;
    }

    epdata_t *epd = lua_touserdata ( L, 1 );

    if ( !lua_isstring ( L, 2 ) ) {
        lua_pushnil ( L );
        lua_pushstring ( L, "Need a file path!" );
        return 2;
    }

    network_sendfile ( epd, lua_tostring ( L, 2 ) );

    return 0;
}

int lua_die ( lua_State *L )
{
    if ( !lua_isuserdata ( L, 1 ) ) {
        luaL_error ( L, "miss epd!" );
        return 0;
    }

    epdata_t *epd = lua_touserdata ( L, 1 );
    network_be_end ( epd );
    lua_pushnil ( L );
    lua_error ( L ); /// stop lua script
    return 0;
}

int lua_get_post_body ( lua_State *L )
{
    if ( !lua_isuserdata ( L, 1 ) ) {
        luaL_error ( L, "miss epd!" );
        return 0;
    }

    epdata_t *epd = lua_touserdata ( L, 1 );

    if ( epd->content_length > 0 ) {
        lua_pushlstring ( L, epd->contents, epd->content_length );

    } else {
        lua_pushnil ( L );
    }

    free ( epd->headers );
    epd->headers = NULL;
    epd->header_len = 0;
    epd->contents = NULL;
    epd->content_length = 0;

    return 1;
}

int lua_f_random_string ( lua_State *L )
{
    int size = 32;

    if ( lua_gettop ( L ) == 1 && lua_isnumber ( L, 1 ) ) {
        size = lua_tonumber ( L, 1 );
    }

    if ( size < 1 ) {
        size = 32;
    }

    if ( size > 4096 ) {
        return 0;
    }

    random_string ( tbuf_4096, size, 0 );

    lua_pushlstring ( L, tbuf_4096, size );

    return 1;
}

int lua_f_file_exists ( lua_State *L )
{
    if ( !lua_isstring ( L, 1 ) ) {
        lua_pushnil ( L );
        lua_pushstring ( L, "Need a file path!" );
        return 2;
    }

    struct stat st;

    const char *fname = lua_tostring ( L, 1 );

    if ( stat ( fname, &st ) == -1 ) {
        lua_pushboolean ( L, 0 );
    }

    lua_pushboolean ( L, 1 );

    return 1;
}
