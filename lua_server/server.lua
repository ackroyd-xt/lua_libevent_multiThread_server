
--local clib = require "x_api"

function server(info)
	while 1 do
		--wait4Read(info)	--���ȷ���ͻ������Ӻ����ϻ�write������ȴ�
		--info = coroutine.yield()
		str = luaRead(info)
		if luaCheckConnection(info) == 1 then
			luaWrite(info, "[filter1] "..str)	--ֱ������д
		else
			luaClose(info)	--�ͻ����˳���ֱ�ӵȴ���һ�����ӣ������ظ�ɾ���ʹ���lua_StateӰ���ٶ�
			info = coroutine.yield()
		end
	end
end



--[[co = coroutine.create(server)
coroutine.resume(co, 1, 0, 3, "aaa")
coroutine.resume(co, 1, 0, 3, "bbb")
coroutine.resume(co, 1, 0, 3, "ccc")
coroutine.resume(co, 1, 0, 3, "aaa")]]



