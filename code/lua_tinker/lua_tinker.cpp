// Copyright (C) 2015 Leslie Zhai <xiang.zhai@i-soft.com.cn>

#include "lua_tinker.h"
#include <iostream>
#include <string>

#if defined(_MSC_VER)
	#define I64_FMT "I64"
#elif defined(__APPLE__)
	#define I64_FMT "q"
#else
	#define I64_FMT "ll"
#endif

void lua_tinker::init(lua_State *L)
{
	init_s64(L);
	init_u64(L);
}

static int tostring_s64(lua_State* L)
{
	lua_pushstring(L,std::to_string(*(long long*)lua_topointer(L,1)).c_str());
	return 1;
}

static int eq_s64(lua_State *L)
{
	int64_t a = *(int64_t*)lua_touserdata(L,1);
	int64_t b = *(int64_t*)lua_touserdata(L,2);
	lua_pushboolean(L,(a==b));
	return 1;
}

static int lt_s64(lua_State *L)
{
	int64_t a = *(int64_t*)lua_touserdata(L,1);
	int64_t b = *(int64_t*)lua_touserdata(L,2);
	lua_pushboolean(L,(a < b));
	return 1;
}

static int le_s64(lua_State *L)
{
	int64_t a = *(int64_t*)lua_touserdata(L,1);
	int64_t b = *(int64_t*)lua_touserdata(L,2);
	lua_pushboolean(L,(a <= b));
	return 1;
}

void lua_tinker::init_s64(lua_State *L)
{
	const char* name = "__s64";
	lua_newtable(L);
	
	lua_pushstring(L,"__name");
	lua_pushstring(L,name);
	lua_rawset(L,-3);
	
	lua_pushstring(L,"__tostring");
	lua_pushcclosure(L,tostring_s64,0);
	lua_rawset(L,-3);
	
	lua_pushstring(L,"__eq");
	lua_pushcclosure(L,eq_s64,0);
	lua_rawset(L,-3);
	
	lua_pushstring(L,"__lt");
	lua_pushcclosure(L,lt_s64,0);
	lua_rawset(L,-3);
	
	lua_pushstring(L,"__le");
	lua_pushcclosure(L,le_s64,0);
	lua_rawset(L,-3);
	
	lua_setglobal(L,name);
}

static int tostring_u64(lua_State* L)
{
	lua_pushstring(L,std::to_string(*(unsigned long long*)lua_topointer(L,1)).c_str());
	return 1;
}

static int eq_u64(lua_State *L)
{
	uint64_t a = *(uint64_t*)lua_touserdata(L,1);
	uint64_t b = *(uint64_t*)lua_touserdata(L,2);
	lua_pushboolean(L,(a==b));
	return 1;
}

static int lt_u64(lua_State *L)
{
	uint64_t a = *(uint64_t*)lua_touserdata(L,1);
	uint64_t b = *(uint64_t*)lua_touserdata(L,2);
	lua_pushboolean(L,(a < b));
	return 1;
}

static int le_u64(lua_State *L)
{
	uint64_t a = *(uint64_t*)lua_touserdata(L,1);
	uint64_t b = *(uint64_t*)lua_touserdata(L,2);
	lua_pushboolean(L,(a <= b));
	return 1;
}

void lua_tinker::init_u64(lua_State *L)
{
	const char* name = "__u64";
	lua_newtable(L);
	
	lua_pushstring(L,"__name");
	lua_pushstring(L,name);
	lua_rawset(L,-3);
	
	lua_pushstring(L,"__tostring");
	lua_pushcclosure(L,tostring_u64,0);
	lua_rawset(L,-3);
	
	lua_pushstring(L,"__eq");
	lua_pushcclosure(L,eq_u64,0);
	lua_rawset(L,-3);
	
	lua_pushstring(L,"__lt");
	lua_pushcclosure(L,lt_u64,0);
	lua_rawset(L,-3);
	
	lua_pushstring(L,"__le");
	lua_pushcclosure(L,le_u64,0);
	lua_rawset(L,-3);
	
	lua_setglobal(L,name);
}

void lua_tinker::print_error(lua_State *L,const char* fmt,...)
{
	char text[4096];
	va_list args;
	va_start(args,fmt);
	vsnprintf(text,sizeof(text),fmt,args);
	va_end(args);
	lua_getglobal(L,"_ALERT");
	if(lua_isfunction(L,-1))
	{
		lua_pushstring(L,text);
		lua_call(L,1,0);
	}
	else
	{
		printf("%s\n",text);
		lua_pop(L,1);
	}
}

static void enum_cur_stack(lua_State *L,lua_Debug* pAr)
{
	int index = 0;
	const char* indent;
	do
	{
		indent = lua_getlocal(L,pAr,++index);
		if(indent)
		{
			switch(lua_type(L,-1))
			{
				case LUA_TNIL:
					lua_tinker::print_error(L,"[LUA_DEBUG] |\t\t|-%s:%s",lua_typename(L,lua_type(L,-1)),indent);
					break;
				case LUA_TBOOLEAN:
					lua_tinker::print_error(L,"[LUA_DEBUG] |\t\t|-%s:%s    %s",lua_typename(L,lua_type(L,-1)),indent,lua_toboolean(L,-1)?"true":"false");
					break;
				case LUA_TLIGHTUSERDATA:
					lua_tinker::print_error(L,"[LUA_DEBUG] |\t\t|-%s:%s    %x%08p",lua_typename(L,lua_type(L,-1)),indent,lua_topointer(L,-1));
					break;
				case LUA_TNUMBER:
					lua_tinker::print_error(L,"[LUA_DEBUG] |\t\t|-%s:%s    %f",lua_typename(L,lua_type(L,-1)),indent,lua_tonumber(L,-1));
					break;
				case LUA_TSTRING:
					lua_tinker::print_error(L,"[LUA_DEBUG] |\t\t|-%s:%s    %s",lua_typename(L,lua_type(L,-1)),indent,lua_tostring(L,-1));
					break;
				case LUA_TTABLE:
					lua_tinker::print_error(L,"[LUA_DEBUG] |\t\t|-%s:%s    %x%08p",lua_typename(L,lua_type(L,-1)),indent,lua_topointer(L,-1));
					break;
				case LUA_TFUNCTION:
					lua_tinker::print_error(L,"[LUA_DEBUG] |\t\t|-%s:%s()    %x%08p",lua_typename(L,lua_type(L,-1)),indent,lua_topointer(L,-1));
					break;
				case LUA_TUSERDATA:
					lua_tinker::print_error(L,"[LUA_DEBUG] |\t\t|-%s:%s    %x%08p",lua_typename(L,lua_type(L,-1)),indent,lua_topointer(L,-1));
					break;
				case LUA_TTHREAD:
					lua_tinker::print_error(L,"[LUA_DEBUG] |\t\t|-%s:%s",lua_typename(L,lua_type(L,-1)),indent);
					break;
			}
			lua_settop(L,-2);
		}
	}
	while(indent);
}

static void call_stack(lua_State* L,int n)
{
	lua_Debug ar;
	if(lua_getstack(L,n,&ar) == 1)
	{
		lua_getinfo(L,"nSlu",&ar);
		const char* indent;
		if(n == 0)
		{
			indent = "->\t";
			lua_tinker::print_error(L,"\t<call stack>");
		}
		else
		{
			indent = "\t";
		}
		if(ar.name)
		{
			lua_tinker::print_error(L,"%s%s() :line %d [$s : line %d]",indent,ar.name,ar.currentline,ar.source,ar.linedefined);
		}
		else
		{
			lua_tinker::print_error(L,"%sunknown: line %d [%s : line %d]",indent,ar.currentline,ar.source,ar.linedefined);
		}
		enum_cur_stack(L,&ar);
		call_stack(L, n+1);
	}
}
int lua_tinker::on_error(lua_State *L)
{
	print_error(L,"%s",lua_tostring(L,-1));
	call_stack(L,0);
	return 0;
}


void lua_tinker::dofile(lua_State *L,const char* filename)
{
	lua_pushcclosure(L,on_error,0);
	int errfunc = lua_gettop(L);
	if(luaL_loadfile(L,filename)==0)
	{
		lua_pcall(L,0,1,errfunc);
	}
	else
	{
		print_error(L,"%s",lua_tostring(L,-1));
	}
	lua_remove(L,errfunc);
	lua_pop(L,1);
}
void lua_tinker::dostring(lua_State *L,const char* buff)
{
	lua_tinker::dobuffer(L,buff,strlen(buff));
}
void lua_tinker::dobuffer(lua_State* L,const char* buff,size_t len)
{
	lua_pushcclosure(L,on_error,0);
	int errfunc = lua_gettop(L);
	if(luaL_loadbuffer(L,buff,len,"lua_tinker::dobuffer()") == 0)
	{
		lua_pcall(L,0,1,errfunc);
	}
	else
	{
		print_error(L,"%s",lua_tostring(L,-1));
	}
	lua_remove(L,errfunc);
	lua_pop(L,1);
}




//lua通过__index类似a[1]的方式访问 a跟1都会压入堆栈，之后lua会负责出栈
static void invoke_parent(lua_State* L)
{
	lua_pushstring(L,"__parent");
	lua_rawget(L,-2);
	if(lua_istable(L,-1))
	{
		lua_pushvalue(L,2);
		lua_rawget(L,-2);
		if(!lua_isnil(L,-1))
		{
			lua_remove(L,-2); //删除meta表
		}
		else
		{
			lua_remove(L,-1);
			invoke_parent(L);
			lua_remove(L,-2); //删除meta表
		}
	}
}

int lua_tinker::meta_get(lua_State* L)
{
	lua_getmetatable(L,1);
	lua_pushvalue(L,2);
	lua_rawget(L,-2);
	if(lua_isuserdata(L,-1) != 0)
	{
		//返回一个userdata转换为var_base
		//然后堆栈1的位置还是table，然后读出来转换为T*指针
		//_var是指向那个数值指针位置
		//read<T*>(L,1)->*(_var)
		//相当于T* t，在t中指向某个成员变量的指针为_var
		//怎样确定_var指向的类型
		user2type<var_base*>(L,-1)->get(L);  
		lua_remove(L,-2);
	}
	else if(lua_isnil(L,-1))
	{
		lua_remove(L,-1);
		invoke_parent(L);
		if(lua_isuserdata(L,-1))
		{
			user2type<var_base*>(L,-1)->get(L);
			lua_remove(L,-2);
		}
		else if(lua_isnil(L,-1))
		{
			lua_pushfstring(L,"can't find %s function or variable",lua_tostring(L,2));
			lua_error(L);
		}
	}
	lua_remove(L,-2);
	return 1;
}

int lua_tinker::meta_set(lua_State *L)
{
	lua_getmetatable(L,1);
	lua_pushvalue(L,2);
	lua_rawget(L,-2);
	if(lua_isuserdata(L,-1))
	{
		user2type<var_base*>(L,-1)->set(L);
	}
	else if(lua_isnil(L,-1))
	{
		lua_remove(L,-1);
		lua_pushvalue(L,2);
		lua_pushvalue(L,4);
		invoke_parent(L);
		if(lua_isuserdata(L,-1))
		{
			user2type<var_base*>(L,-1)->set(L);
		}
		else if(lua_isnil(L,-1))
		{
			lua_pushfstring(L,"can't find %s function or variable",lua_tostring(L,2));
			lua_error(L);
		}
	}
	lua_settop(L,3);
	return 0;
}

void lua_tinker::push_meta(lua_State* L,const char* name)
{
	lua_getglobal(L,name);
}

lua_tinker::table_obj::table_obj(lua_State* L,int index)
:m_L(L),m_index(index),m_ref(0)
{
	m_pointer = lua_topointer(m_L,m_index);
	m_size = lua_rawlen(m_L,m_index);
}

lua_tinker::table_obj::~table_obj()
{
	if(validate())
	{
		lua_remove(m_L,m_index);
	}
}

void lua_tinker::table_obj::inc_ref()
{
	++m_ref;
}

void lua_tinker::table_obj::dec_ref()
{
	if(--m_ref == 0)
	{
		delete this;
	}
}

bool lua_tinker::table_obj::validate()
{
	if(m_pointer != NULL)
	{
		if(m_pointer == lua_topointer(m_L,m_index))
		{
			return true;
		}
		else
		{
			int top = lua_gettop(m_L);
			for(int i=1; i<=top; ++i)
			{
				if(m_pointer == lua_topointer(m_L,i))
				{
					m_index = i;
					return true;
				}
				m_pointer = NULL;
				return false;
			}
		}
	}
	else
	{
		return false;
	}
}

lua_tinker::table::table(lua_State* L)
{
	lua_newtable(L);
	m_obj = new table_obj(L,lua_gettop(L));
	m_obj->inc_ref();
}

lua_tinker::table::table(lua_State* L,const char* name)
{
	lua_getglobal(L,name);
	if(lua_istable(L,-1) == 0)
	{
		lua_pop(L,1);
		lua_newtable(L);
		lua_pushvalue(L,-1);
		lua_setglobal(L,name);
	}
	m_obj = new table_obj(L,lua_gettop(L));
	m_obj->inc_ref();
}

lua_tinker::table::table(lua_State* L,int index)
{
	if(index < 0)
	{
		index = lua_gettop(L) + index +1;
	}
	m_obj = new table_obj(L,index);
	m_obj->inc_ref();
}

lua_tinker::table::~table()
{
	m_obj->dec_ref();
}

template<>
char* lua_tinker::_read(lua_State *L, int index)
{
	return (char*)lua_tostring(L,index);
}

template<>
const char* lua_tinker::_read(lua_State *L, int index)
{
	return (const char*)lua_tostring(L,index);
}

template<>
char lua_tinker::_read(lua_State *L, int index)
{
	return (char)lua_tointeger(L,index);
}

template<>
unsigned char lua_tinker::_read(lua_State *L, int index)
{
	return (unsigned char)lua_tointeger(L,index);
}

template<>
short lua_tinker::_read(lua_State *L, int index)
{
	return (short)lua_tointeger(L,index);
}

template<>
unsigned short lua_tinker::_read(lua_State *L, int index)
{
	return (unsigned short)lua_tointeger(L,index);
}

template<>
long lua_tinker::_read(lua_State *L, int index)
{
	return (long)lua_tointeger(L,index);
}

template<>
unsigned long lua_tinker::_read(lua_State *L, int index)
{
	return (unsigned long)lua_tointeger(L,index);
}

template<>
int lua_tinker::_read(lua_State *L, int index)
{
	return (int)lua_tointeger(L,index);
}

template<>
unsigned int lua_tinker::_read(lua_State *L, int index)
{
	return (unsigned int)lua_tointeger(L,index);
}

template<>
float lua_tinker::_read(lua_State *L, int index)
{
	return (float)lua_tonumber(L,index);
}

template<>
double lua_tinker::_read(lua_State *L, int index)
{
	return (double)lua_tonumber(L,index);
}

template<>
bool lua_tinker::_read(lua_State *L,int index)
{
	if(lua_isboolean(L,index))
		return lua_toboolean(L,index) != 0;
	else
		return lua_tonumber(L,index) != 0;
}

template<>
void lua_tinker::_read(lua_State *L,int index)
{
	return;
}

template<>
long long lua_tinker::_read(lua_State *L,int index)
{
	if(lua_isnumber(L,index))
		return (long long)lua_tonumber(L,index);
	else
		return *(long long*)lua_touserdata(L,index);
}

template<>
unsigned long long lua_tinker::_read(lua_State *L,int index)
{
	if(lua_isnumber(L,index))
		return (unsigned long long)lua_tonumber(L,index);
	else
		return *(unsigned long long*)lua_touserdata(L,index);
}

template<>
lua_tinker::table lua_tinker::_read(lua_State *L,int index)
{
	return table(L,index);
}

template<>
std::string lua_tinker::_read(lua_State *L,int index)
{
	return std::string((const char*)lua_tostring(L,index));
}

template<>
void lua_tinker::read(lua_State *L,int index)
{
}

template<>
void lua_tinker::push(lua_State *L,char ret)
{
	lua_pushnumber(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,unsigned char ret)
{
	lua_pushnumber(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,short ret)
{
	lua_pushnumber(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,unsigned short ret)
{
	lua_pushnumber(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,long ret)
{
	lua_pushnumber(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,unsigned long ret)
{
	lua_pushnumber(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,int ret)
{
	lua_pushnumber(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,unsigned int ret)
{
	lua_pushnumber(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,float ret)
{
	lua_pushnumber(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,double ret)
{
	lua_pushnumber(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,char* ret)
{
	lua_pushstring(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,const char* ret)
{
	lua_pushstring(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,bool ret)
{
	lua_pushboolean(L,ret);
}

template<>
void lua_tinker::push(lua_State *L,lua_value* ret)
{
	if(ret)
	{
		ret->to_lua(L);
	}
	else
	{
		lua_pushnil(L);
	}
}

template<>
void lua_tinker::push(lua_State *L,long long ret)
{
	*(long long*)lua_newuserdata(L,sizeof(long long)) = ret;
	lua_getglobal(L,"__s64");
	lua_setmetatable(L,-2);
}

template<>
void lua_tinker::push(lua_State *L,unsigned long long ret)
{
	*(long long*)lua_newuserdata(L,sizeof(unsigned long long)) = ret;
	lua_getglobal(L,"__u64");
	lua_setmetatable(L,-2);
}

template<>
void lua_tinker::push(lua_State *L,lua_tinker::table ret)
{
	lua_pushvalue(L,ret.m_obj->m_index);
}

template<>
void lua_tinker::push(lua_State *L,std::string ret)
{
	lua_pushlstring(L,ret.data(),ret.size());
}

template<>
void lua_tinker::push(lua_State *L,const std::string& ret)
{
	lua_pushlstring(L,ret.data(),ret.size());
}

template<>
void lua_tinker::pop(lua_State *L)
{
	lua_pop(L,1);
}

template<>
lua_tinker::table lua_tinker::pop(lua_State *L)
{
	return table(L,lua_gettop(L));
}
