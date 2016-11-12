
int32 lstream_socket::s2c_send()
{
    class lstream **stream = static_cast<class lstream **>(
        luaL_checkudata( L, 1, "Stream" ) );
    uint16 mod  = static_cast<uint16>( luaL_checkinteger( L,2 ) );
    uint16 func = static_cast<uint16>( luaL_checkinteger( L,3 ) );
    uint16 eno  = static_cast<uint16>( luaL_checkinteger( L,4 ) );

    const struct stream_protocol::node *nd = (*stream)->find( mod,func );
    if ( (struct stream_protocol::node *)-1 == nd )
    {
        return luaL_error( L,
            "lstream_socket::s2c_send no such protocol %d-%d",mod,func );
    }
    else if ( nd ) /* 为0表示空协议，可以不传table */
    {
        luaL_checktype( L,5,LUA_TTABLE );
    }

    struct s2c_header header;
    header._mod   = mod;
    header._func  = func;
    header._errno = eno;

     /* 这个函数出错可能不会返回，缓冲区需要能够自动回溯 */

    int32 result = 0;
    const char *str_err = NULL;
    {
        class stream_packet packet( &_send,L );
        if ( (result = packet.pack( header,nd,5 ) ) < 0 )
        {
            str_err = packet.last_error();
            ERROR( str_err );
        }
    }

    /* stack-unwind和longjump冲突
     * 调用luaL_error,需要用code-block保证packet析构函数能执行
     * header这个不需要在析构函数里运行逻辑，无所谓
     */
    if ( result < 0 ) return luaL_error( L,str_err );

    pending_send();

    return 0;
}

/* 网络数据不可信，这些数据可能来自非法客户端，有很大机率出错
 * 这个接口少调用luaL_error，尽量保证能返回到lua层处理异常
 * 否则可能导致协议分发接口while循环中止，无法断开非法链接
 */
int32 lstream_socket::c2s_recv()
{
    class lstream **stream = static_cast<class lstream **>(
            luaL_checkudata( L, 1, "Stream" ) );

    if ( !stream_packet::is_complete( &_recv ) )
    {
        lua_pushinteger( L,0 );
        return 1;
    }

    c2s_header *header = NULL;
    if ( _recv.data_size() < sizeof(c2s_header) )
    {
        lua_pushinteger( L,0 );
        return 1;
    }

    header = reinterpret_cast<c2s_header*>( _recv.data() );

    const struct stream_protocol::node *nd = (*stream)->find( header->_mod,header->_func );
    if ( (struct stream_protocol::node *)-1 == nd )
    {
        ERROR( "c2s_recv no such protocol:%d-%d",header->_mod,header->_func );
        lua_pushinteger( L,-1 );
        return 1;
    }

    lua_pushinteger( L,header->_length );
    lua_pushinteger( L,header->_mod );
    lua_pushinteger( L,header->_func );

    class stream_packet packet( &_recv,L );
    if ( packet.unpack( *header,nd ) < 0 )
    {
        ERROR( packet.last_error() );

        lua_pushinteger( L,-1 );
        return 1;
    }

    /* 如果这时缓冲区刚好是空的，尽快处理悬空区，这时代价最小，不用拷贝内存 */
    if ( _recv.data_size() <= 0 ) _recv.clear();

    return 4;
}

int32 lstream_socket::c2s_send()
{
    class lstream **stream = static_cast<class lstream **>(
        luaL_checkudata( L, 1, "Stream" ) );
    uint16 mod  = static_cast<uint16>( luaL_checkinteger( L,2 ) );
    uint16 func = static_cast<uint16>( luaL_checkinteger( L,3 ) );

    const struct stream_protocol::node *nd = (*stream)->find( mod,func );
    if ( (struct stream_protocol::node *)-1 == nd )
    {
        return luaL_error( L,
            "lstream_socket::s2c_send no such protocol %d-%d",mod,func );
    }
    else if ( nd )
    {
        luaL_checktype( L,4,LUA_TTABLE );
    }

    struct c2s_header header;
    header._mod   = mod;
    header._func  = func;

    /* 这个函数出错可能不会返回，缓冲区需要能够自动回溯 */
    int result = 0;
    const char *str_err = NULL;
    {
        class stream_packet packet( &_send,L );
        if ( (result = packet.pack( header,nd,4 ) ) < 0 )
        {
            str_err = packet.last_error();
            ERROR( str_err );
        }
    }

    /* stack-unwind和longjump冲突
     * 调用luaL_error,需要用code-block保证packet析构函数能执行
     * header这个不需要在析构函数里运行逻辑，无所谓
     */
    if ( result < 0 ) return luaL_error( L,str_err );

    pending_send();

    return 0;
}

/* 网络数据不可信，这些数据可能来自非法客户端，有很大机率出错
 * 这个接口少调用luaL_error，尽量保证能返回到lua层处理异常
 * 否则可能导致协议分发接口while循环中止，无法断开非法链接
 */
int32 lstream_socket::s2c_recv()
{
    class lstream **stream = static_cast<class lstream **>(
            luaL_checkudata( L, 1, "Stream" ) );

    if ( !stream_packet::is_complete( &_recv ) )
    {
        lua_pushinteger( L,0 );
        return 1;
    }

    s2c_header *header = NULL;
    if ( _recv.data_size() < sizeof(s2c_header) )
    {
        lua_pushinteger( L,0 );
        return 1;
    }

    header = reinterpret_cast<s2c_header*>( _recv.data() );

    const struct stream_protocol::node *nd = (*stream)->find( header->_mod,header->_func );
    if ( (struct stream_protocol::node *)-1 == nd )
    {
        ERROR( "s2c_recv:no such protocol:%d-%d",header->_mod,header->_func );
        lua_pushinteger( L,-1 );
        return 1;
    }

    lua_pushinteger( L,header->_length );
    lua_pushinteger( L,header->_mod );
    lua_pushinteger( L,header->_func );

    class stream_packet packet( &_recv,L );
    if ( packet.unpack( *header,nd ) < 0 )
    {
        ERROR( packet.last_error() );

        lua_pushinteger( L,-1 );
        return 1;
    }

    /* 如果这时缓冲区刚好是空的，尽快处理悬空区，这时代价最小，不用拷贝内存 */
    if ( _recv.data_size() <= 0 ) _recv.clear();

    return 4;
}
