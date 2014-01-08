
--local clib = require "libcapi"

--示例脚本：等待超时5秒，若5秒内客户端未发送文本则发送文本 "connection timeout" 给客户端
function server(info)
	while 1 do
		wait4Read(info, 5000000)
		info = coroutine.yield()
		if luaCheckTimeout(info) == 1 then
			luaWrite(info, "connection timeout\n")
		else
			str = luaRead(info)
			if luaCheckConnection(info) == 1 then
				luaWrite(info, "[filter1] "..str)	--直接阻塞写
			else
				luaClose(info)	--客户端退出则直接等待下一个连接，避免重复删除和创建lua_State影响速度
				info = coroutine.yield()
			end
		end
	end
end



--[[co = coroutine.create(server)
coroutine.resume(co, 1, 0, 3, "aaa")
coroutine.resume(co, 1, 0, 3, "bbb")
coroutine.resume(co, 1, 0, 3, "ccc")
coroutine.resume(co, 1, 0, 3, "aaa")]]



