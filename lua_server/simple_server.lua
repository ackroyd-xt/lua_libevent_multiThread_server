
require "x_api"

--示例脚本：等待超时5秒，若5秒内客户端未发送文本则发送文本 "connection timeout" 给客户端
function server(info)
	while 1 do
		str = x_read(info, 5000000)
		if str == "" then
			--[[todo: 这里使用非阻塞写会导致一个问题，就是注册事件前对方已关闭连接，当下一个连接来临时才唤醒事件，导致下一个连接获取了上一个连接的应答 ]]
			if luaWrite(info, "connection timeout\n") < 0 then
				luaClose(info)
				info = coroutine.yield()
			end
		elseif luaCheckConnection(info) == 0 then
			luaClose(info)
			info = coroutine.yield()
		else
			if luaWrite(info, "[filter1] "..str) < 0 then
				luaClose(info)
				info = coroutine.yield()
			end
		end
	end
end



--[[coroutine.resume(co, 1, 0, 3, "aaa")
coroutine.resume(co, 1, 0, 3, "bbb")
coroutine.resume(co, 1, 0, 3, "ccc")
coroutine.resume(co, 1, 0, 3, "aaa")]]



