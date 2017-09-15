test1()
test2()
test3()
f2 = ff(10)
f2:test1()
f2:test2(5)
f2:test3()
f1 = f()
f1:testf()
print(f1.m_num)
--print(f2.m_num)
f2:testf()

function lua_test()
	print("this is lua_test")
end

num = 10
function lua_test2(fa)
	num = num + fa.m_num
 return num
end