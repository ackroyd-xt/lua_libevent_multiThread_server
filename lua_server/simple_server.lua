
require "x_api"

--ʾ���ű����ȴ���ʱ5�룬��5���ڿͻ���δ�����ı������ı� "connection timeout" ���ͻ���
function server(info)
	while 1 do
		str = x_read(info, 5000000)
		if str == "" then
			--[[todo: ����ʹ�÷�����д�ᵼ��һ�����⣬����ע���¼�ǰ�Է��ѹر����ӣ�����һ����������ʱ�Ż����¼���������һ�����ӻ�ȡ����һ�����ӵ�Ӧ�� ]]
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



