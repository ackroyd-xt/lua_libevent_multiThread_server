

function x_read(info, timeout)

	wait4Read(info, timeout)
	info = coroutine.yield()
	if luaCheckTimeout(info) == 1 then
		return ""
	end
	return luaRead(info)
end

function x_readn(info, n)
	local str = ""
	while 1 do
		local t = x_read(info, -1)
		if luaCheckConnection(info) == 0 then
			return str
		end
		str = str..t
		if string.len(str) >= n then
			return str
		end
	end
end

function x_writen(info, str)
	local t = 0
	while 1 do
		wait4Write(info, -1)
		info = coroutine.yield()
		local ret = luaUnblockWrite(info, string.sub(str, t+1, -1))
		if ret < 0 then
			return ret
		end
		t = t + ret
		if t == string.len(str) then
			return t
		end
	end
end




	




