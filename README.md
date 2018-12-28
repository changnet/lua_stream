lua_stream
-----------
Pack binary data in lua.unfinished!

some thoughts
----

* api
```lua
    -- 打包数据
    -- @cmd:协议号,根据此协议号读schema文件
    -- @optional:允许部分数据不打包,用nil占位即可
    -- 所有需要打包的数据平铺在堆栈上
    lua_stream:pack([optional,]cmd,d1,d2,d3,nil,d5...)

    -- 解包数据,缓冲区无数据则返回nil
    -- 不支持optional解包，解出的数据包平铺在堆栈
    local d1,d2,d3 = lua_stream:unpack(cmd)

    -- 在缓冲区分配一个数据包,并且手动打包数据
    -- 用于复杂的数据结构、动态协议
    lua_stream:new_pack(cmd)
    lua_stream:write_int8(d1)
    lua_stream:write_int16(d1)
    ...
    lua_stream:send()

    -- 读取数据包，缓冲区无数据则返回nil
    -- 用于解析带optional的数据包
    local d1 = lua_stream:read_int8()
    local d1 = lua_stream:read_int16()
    ...
```
* schema
```lua
    -- 允许自定义数据结构
    -- 允许结构嵌套
    -- 允许数组，可用json的[]表示，也可以用lua的{}表示
    -- 允许重定义数据类型，例如;player_id = int64
    equip = { id:int,attr:{int},exp:float }
```

* editor
```lua
    -- 能够自识别自定义数据结构
    -- 考虑用lua语法，给一个巨大的输入栏由程序输入
    equip = 
    {
        id:int, -- 装备id
        attr:{int}, -- 属性数组
        exp:float -- 强化经验
    }
    -- 能校验语法
    -- 最后有语法高亮，自动缩进
    -- 让程序按字段输入的方式太慢，而且对嵌套结构非常不友好，输入界面也很难设置
```

* error
```lua
    -- 把错误的结构按json或者lua结构打印出来,打印到出错的地方
    -- 例如 attr中的第三个元素错误，则打印
    "expect int,get string at
    {
        id:int,
        attr:{int,int,int<
    }
```

other
-----

* 不要设置太复杂，不要尝试在编辑器、schema中去处理optional字段，也不要尝试自动解析带optional的包
* 如果你想加更多的功能，为啥不直接换protobuf


