#include <stdio.h>
#include "lua_tinker/lua_tinker.h"
#include<iostream>
void test1()
{
	printf("this is test1 \n");
}

void test2(int n)
{
	printf("this is test2 num %d \n",n);
}

int test3()
{
	printf("this is test3 \n");
	return 1;
}

class f
{
public:
		f():m_num(999)
		{
			
		}
		void testf()
		{
			printf("this is ff testf \n");
		}
	int m_num;
};

class ff: public f
{
public:
	ff(int a =0):m_val(a)
	{
		printf("construct ff \n");
	}
	void test1()
	{
		printf("this is ff test1 \n");
	}

	void test2(int n)
	{
		printf("this is test2 num %d \n",n);
	}

	int test3()
	{
		printf("this is test3 %d \n",m_val);
		return 1;
	}
private:
	int m_val;
};

void RegisterLua(lua_State* L)
{
	lua_tinker::def(L,"test1",test1);
	lua_tinker::def(L,"test2",test2);
	lua_tinker::def(L,"test3",test3);
	lua_tinker::class_add<f>(L,"f");
	lua_tinker::class_add<ff>(L,"ff");
	lua_tinker::class_inh<ff,f>(L);
	
	lua_tinker::class_con<f>(L,&lua_tinker::constructor<f>);
	lua_tinker::class_def<f>(L,"testf",&f::testf);
	lua_tinker::class_mem<f>(L,"m_num",&f::m_num);
	
	
	lua_tinker::class_con<ff>(L,&lua_tinker::constructor<ff,int>);
	lua_tinker::class_def<ff>(L,"test1",&ff::test1);
	lua_tinker::class_def<ff>(L,"test2",&ff::test2);
	lua_tinker::class_def<ff>(L,"test3",&ff::test3);
}
int main()
{
    lua_State* L;
    L = luaL_newstate();
    luaL_openlibs(L);
    lua_tinker::init(L);
	RegisterLua(L);
	lua_tinker::dofile(L,"1.lua");
	int a = lua_tinker::get<int>(L,"num");
	lua_tinker::call<void>(L,"lua_test");
	f * fa = new f();
	int b = lua_tinker::call<int,f*>(L,"lua_test2",fa);
	printf("a=%d,b=%d\n",a,b);
	std::cout<< &ff::test1 << std::endl;
    lua_close(L);
    return 1;
}
