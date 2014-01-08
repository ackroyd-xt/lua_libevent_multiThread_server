
--local clib = require "x_api"

function server(info)
	while 1 do
		--wait4Read(info)	--如果确定客户端连接后马上会write则无需等待
		--info = coroutine.yield()
		str = luaRead(info)
		if luaCheckConnection(info) == 1 then
			luaWrite(info, "[filter1] "..str)	--直接阻塞写
		else
			luaClose(info)	--客户端退出则直接等待下一个连接，避免重复删除和创建lua_State影响速度
			info = coroutine.yield()
		end
	end
end



--[[co = coroutine.create(server)
coroutine.resume(co, 1, 0, 3, "aaa")
coroutine.resume(co, 1, 0, 3, "bbb")
coroutine.resume(co, 1, 0, 3, "ccc")
coroutine.resume(co, 1, 0, 3, "aaa")]]



