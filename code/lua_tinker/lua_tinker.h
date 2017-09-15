#if !defined(_LUA_TINKER_H_)
#define _LUA_TINKER_H_

#include <new>
#include <stdint.h>
#include <stdio.h>
#include <typeinfo>
#include <type_traits>
#include <map>
#include <vector>
#include <set>
#include <string.h>
#include "lua.hpp"

#ifdef LUA_CALL_CFUNC_NEED_ALL_PARAM
#define LUA_CHECK_HAVE_THIS_PARAM(L,index) if(lua_isnone(L,index)){lua_pushfstring(L,"need argument %d to call cfunc",index);lua_error(L);}
#define LUA_CHECK_HAVE_THIS_PARAM_AND_NOT_NIL(L,index) if(lua_isnoneornil(L,index)){lua_pushfstring(L,"need argument %d to call cfunc",index);lua_error(L);}
#else
#define LUA_CHECK_HAVE_THIS_PARAM(L,index)
#define LUA_CHECK_HAVE_THIS_PARAM_AND_NOT_NIL(L,index)
#endif

#define CHECK_CLASS_PTR(T) {if(lua_isnoneornil(L,1)){lua_pushfstring(L,"class_ptr %s is nil or none",lua_tinker::class_name<typename class_type<T>::type>::name());lua_error(L);}}
#define TRY_LUA_TINKER_INVOKE() try
#define CATCH_LUA_TINKER_INVOKE() catch(...)
//push -- luatotype--is_enum就pushnum，不是的话调用object2lua--建立一个user
//read -- lua2type--is_enum就tonum 不是的话先type2user在调用user的m_ptr再void2type
namespace lua_tinker
{
	void init(lua_State *L);
	void init_s64(lua_State *L);
	void init_u64(lua_State *L);
	
	void dofile(lua_State *L,const char* filename);
	void dostring(lua_State *L,const char* buff);
	void dobuffer(lua_State* L,const char* buff,size_t len);
	
	void enum_stack(lua_State *L);
	int on_error(lua_State *L);
	void print_error(lua_State *L,const char* fmt,...);
	
	int meta_get(lua_State *L);
	int meta_set(lua_State *L);
	void push_meta(lua_State* L,const char* name);
	
	//typedef void(*(VFuncPtr))(void) ;
	
	struct lua_value
	{
		virtual void to_lua(lua_State *L) = 0;
	};
	
	struct table;
	template<typename T>
	struct class_name
	{
		static const char* name(const char* tempname = NULL)
		{
			static char temp[256]="";
			if(tempname != NULL)
			{
				strncpy(temp,tempname,sizeof(temp)-1);
			}
			return temp;
		}
	};
	
	template<bool C,typename A,typename B> struct if_{};
	template<typename A,typename B> struct if_ <true,A,B>{typedef A type;};
	template<typename A,typename B> struct if_ <false,A,B> {typedef B type;};
	
	template<typename A> struct is_ptr {static const bool value = false;};
	template<typename A> struct is_ptr<A*> {static const bool value = true;};
	
	template<typename A> struct is_ref {static const bool value = false;};
	template<typename A> struct is_ref<A&> {static const bool value = true;};
	
	template<typename A> struct remove_const {typedef A type;};
	template<typename A> struct remove_const<const A> {typedef A type;};
	
	/*
	template<typename A> struct base_type{typedef A type;};
	template<typename A> struct base_type <A*>{typedef A type;};
	template<typename A> struct base_type <A&>{typedef A type;};
	*/
	template<typename T> 
	using base_type = typename std::remove_cv<typename std::remove_reference<typename std::remove_pointer<T>::type>::type>::type;
	
	template<typename A> 
	struct class_type {typedef typename remove_const< base_type<A> >::type type;};
	
	/*
	template<typename T>
	struct void2val{ static T invoke(void* input){return *(T*)input;}};
	template<typename T>
	struct void2ptr{ static T* invoke(void* input){return (T*)input;}};
	template<typename T>
	struct void2ref{ static T& invoke(void* input){return *(T*)input;}};
	
	template<typename T>
	struct void2type
	{
		static T invoke(void* ptr)
		{
			return if_< is_ptr<T>::value,
			void2ptr<typename base_type<T>::type >,
			typename if_< is_ref<T>::value,
			void2ref<typename base_type<T>::type>,
			void2val<typename base_type<T>::type> 
			>::type
			>::type::invoke(ptr);
		}
	}
	*/
	
	//根据值，指针，引用不同来转换为T类型
	template<typename T>
	typename std::enable_if<std::is_pointer<T>::value,T>::type void2type(void* ptr)
	{
		return (T)ptr;
	}
	
	template<typename T>
	typename std::enable_if<std::is_reference<T>::value,T>::type void2type(void* ptr)
	{
		return *(T*)ptr;
	}
	
	template<typename T>
	typename std::enable_if<!std::is_reference<T>::value && !std::is_pointer<T>::value,T>::type void2type(void* ptr)
	{
		return *(T*)ptr;
	}
	
	struct user
	{
		user(void* p):m_p(p){}
		virtual ~user(){}
		void* m_p;
	};
	
	//userdata to void2type
	template<typename T> 
	T user2type(lua_State* L,int index)
	{
		return void2type<T>(lua_touserdata(L,index));
	}
	template<typename T>
	T lua2enum (lua_State* L,int index)
	{
		return (T)(int)(lua_tonumber(L,index));
	}
	
	//先lua userdata 先转到user，再转换user的m_p类型（m_p相当于upvalue？）
	template<typename T> 
	typename std::enable_if<!std::is_enum<T>::value && !std::is_pointer<T>::value,T>::type lua2type (lua_State* L,int index)
	{
		if(!lua_isuserdata(L,index))
		{
			lua_pushfstring(L,"can't convert argument %d to class %s",index,class_name<typename class_type<T>::type>::name());
			lua_error(L);
		}
		return void2type<T>(user2type<user*>(L,index)->m_p);
	}
	
	template<typename T> 
	typename std::enable_if<std::is_pointer<T>::value,T>::type lua2type (lua_State* L,int index)
	{
		if(lua_isnoneornil(L,index))
		{
			return void2type<T>(nullptr);
		}
		else if(lua_isnumber(L,index) && lua_tonumber(L,index) == 0)
		{
			return void2type<T>(nullptr);
		}
		return void2type<T>(user2type<user*>(L,index)->m_p);
	}
	
	template<typename T> 
	typename std::enable_if<std::is_enum<T>::value,T>::type lua2type (lua_State* L,int index)
	{
		return (T)(int)(lua_tonumber(L,index));
	}
	
	
	//这里开始先转换到user，再转换到lua的userdata,因为这样就可以用userdata的形式来保存所有的数据
	//这里特别注意是val2user是new一个T出来，所以这里要负责释放内存，而ptr2user，ref2user则不负责释放内存
	//newuserdata 是由lua负责释放的。。所以不要在c里面手动释放
	template<typename T>
	struct val2user:user
	{
		val2user(): user(new T){}
		
		template<typename T1>
		val2user(T1 t1): user(new T(t1)) {}
		
		template<typename T1,typename T2>
		val2user(T1 t1,T2 t2): user(new T(t1,t2)) {}
		
		template<typename T1,typename T2,typename T3>
		val2user(T1 t1,T2 t2,T3 t3): user(new T(t1,t2,t3)) {}
		
		template<typename T1,typename T2,typename T3,typename T4>
		val2user(T1 t1,T2 t2,T3 t3,T4 t4): user(new T(t1,t2,t3,t4)) {}
		
		template<typename T1,typename T2,typename T3,typename T4,typename T5>
		val2user(T1 t1,T2 t2,T3 t3,T4 t4,T5 t5): user(new T(t1,t2,t3,t4,t5)) {}
		
		template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
		val2user(T1 t1,T2 t2,T3 t3,T4 t4,T5 t5,T6 t6): user(new T(t1,t2,t3,t4,t5,t6)) {}
		
		template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
		val2user(T1 t1,T2 t2,T3 t3,T4 t4,T5 t5,T6 t6,T7 t7): user(new T(t1,t2,t3,t4,t5,t6,t7)) {}
		
		template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
		val2user(T1 t1,T2 t2,T3 t3,T4 t4,T5 t5,T6 t6,T7 t7,T8 t8): user(new T(t1,t2,t3,t4,t5,t6,t7,t8)) {}
		
		template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
		val2user(T1 t1,T2 t2,T3 t3,T4 t4,T5 t5,T6 t6,T7 t7,T8 t8,T9 t9): user(new T(t1,t2,t3,t4,t5,t6,t7,t8,t9)) {}
	
		template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
		val2user(T1 t1,T2 t2,T3 t3,T4 t4,T5 t5,T6 t6,T7 t7,T8 t8,T9 t9,T10 t10): user(new T(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10)) {}
		
		~val2user(){ delete ((T*)m_p);}
	};
	
	template<typename T>
	struct ptr2user:user
	{
		ptr2user(T* t):user((void*)t){}
	};
	
	template<typename T>
	struct ref2user:user
	{
		ref2user(T&t):user(&t){}
	};
	
	template<typename T>
	typename std::enable_if<std::is_pointer<T>::value,void>::type object2lua(lua_State* L,T input)
	{
		if(input)
		{
			new ( lua_newuserdata(L,sizeof(ptr2user<base_type<T> >)))ptr2user<base_type<T> >(input);
		}
		else
		{
			lua_pushnil(L);
		}
	}
	
	template<typename T>
	typename std::enable_if<std::is_reference<T>::value,void>::type object2lua(lua_State* L,T input)
	{
		new ( lua_newuserdata(L,sizeof(ref2user<T>)))ref2user<T>(input);
	}
	
	template<typename T>
	typename std::enable_if<!std::is_reference<T>::value && !std::is_pointer<T>::value,void>::type object2lua(lua_State* L,T input)
	{
		new ( lua_newuserdata(L,sizeof(val2user<T>)))val2user<T>(input);
	}
	
	template<typename T>
	typename std::enable_if<std::is_enum<T>::value,void>::type type2lua(lua_State* L,T val)
	{
		lua_pushnumber(L,(int)val);
	}
	
	//都是先生成user这个userdata在设置metatable
	template<typename T>
	typename std::enable_if<!std::is_enum<T>::value,void>::type type2lua(lua_State* L,T val)
	{
		object2lua(L,val);
		push_meta(L,class_name<base_type<T>>::name());
		lua_setmetatable(L,-2);
	}
	
	template<typename T>
	T upvalue_(lua_State *L)
	{
		return user2type<T>(L,lua_upvalueindex(1));
	}
	
	template<typename T>
	T _read(lua_State* L,int index){return lua2type<T>(L,index);}
	
	template<> char* _read(lua_State* L,int index);
	template<> const char* _read(lua_State* L,int index);
	template<> char _read(lua_State* L,int index);
	template<> unsigned char _read(lua_State* L,int index);
	template<> short _read(lua_State* L,int index);
	template<> unsigned short _read(lua_State* L,int index);
	template<> long _read(lua_State* L,int index);
	template<> unsigned long _read(lua_State* L,int index);
	template<> int _read(lua_State* L,int index);
	template<> unsigned int _read(lua_State* L,int index);
	template<> float _read(lua_State* L,int index);
	template<> double _read(lua_State* L,int index);
	template<> bool _read(lua_State* L,int index);
	template<> long long _read(lua_State* L,int index);
	template<> unsigned long long _read(lua_State* L,int index);
	template<> table _read(lua_State* L,int index);
	template<> std::string _read(lua_State* L,int index);
	
	template<> void _read(lua_State* L,int index);
	
	template<typename T>
	T read(lua_State *L,int index)
	{
	#ifdef LUA_CALL_CFUNC_NEED_ALL_PARAM
		if(std::is_pointer<T>)
		{
			LUA_CHECK_HAVE_THIS_PARAM(L,index);
		}
		else
		{
			LUA_CHECK_HAVE_THIS_PARAM_AND_NOT_NIL(L,index);
		}
	#endif
		return _read<T>(L,index);
	}
	
	template<> void read(lua_State *L,int index);
	
	template<typename T>
	T read_nocheck(lua_State *L,int index)
	{
		return _read<T>(L,index);
	}
	
	
	template<typename T>
	void push(lua_State *L,T ret)
	{
		type2lua<T>(L,ret);
	}
	template<> void push(lua_State *L,char ret);
	template<> void push(lua_State *L,unsigned char ret);
	template<> void push(lua_State *L,short ret);
	template<> void push(lua_State *L,unsigned short ret);
	template<> void push(lua_State *L,long ret);
	template<> void push(lua_State *L,unsigned long ret);
	template<> void push(lua_State *L,int ret);
	template<> void push(lua_State *L,unsigned int ret);
	template<> void push(lua_State *L,float ret);
	template<> void push(lua_State *L,double ret);
	template<> void push(lua_State *L,char* ret);
	template<> void push(lua_State *L,const char* ret);
	template<> void push(lua_State *L,bool ret);
	template<> void push(lua_State *L,lua_value* ret);
	template<> void push(lua_State *L,long long ret);
	template<> void push(lua_State *L,unsigned long long ret);
	template<> void push(lua_State *L,table ret);
	template<> void push(lua_State *L,std::string ret);
	template<> void push(lua_State *L,const std::string& ret);
	
	template<typename K,typename V>
	void push(lua_State *L,const std::map<K,V>& ret)
	{
		lua_newtable(L);
		for(typename std::map<K,V>::const_iterator it = ret.begin();
		it != ret.end(); it++)
		{
			push(L,it->first);
			push(L,it->second);
			lua_settable(L,-3);
		}
	}
	template<typename T>
	void push(lua_State *L, const std::set<T>& ret)
	{
		lua_newtable(L);
		int i =1;
		for(typename std::set<T>::const_iterator it = ret.begin();
		it != ret.end(); it++,i++)
		{
			push(L,i);
			push(L,*it);
			lua_settable(L,-3);
		}
	}
	
	template<typename T>
	void push(lua_State *L, const std::vector<T>& ret)
	{
		lua_newtable(L);
		int i =1;
		for(typename std::vector<T>::const_iterator it = ret.begin();
		it != ret.end(); it++,i++)
		{
			push(L,i);
			push(L,*it);
			lua_settable(L,-3);
		}
	}
	
	template<typename T,typename...Args>
	void push_args(lua_State *L,T ret,Args...args)
	{
		push<T>(L,ret);
		push_args<Args...>(args...);
	}
	
	template<typename T,typename...Args>
	void push_args(lua_State *L,T ret)
	{
		push<T>(L,ret);
	}
	
	template<typename T>
	T pop(lua_State *L)
	{
		T t = read_nocheck<T>(L,-1);
		lua_pop(L,1);
		return t;
	}
	template<> void pop(lua_State *L);
	template<> table pop(lua_State *L);
	//堆栈调用函数，参数从1开始
	
	template<typename Rval,typename T1=void,typename T2=void,typename T3=void,typename T4=void,typename T5=void,typename T6=void,typename T7=void,typename T8=void,typename T9=void,typename T10=void>
	struct functor
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5),read<T6>(L,6),read<T7>(L,7),read<T8>(L,8),read<T9>(L,9),read<T10>(L,10)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
	struct functor<Rval,T1,T2,T3,T4,T5,T6,T7,T8,T9>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)(T1,T2,T3,T4,T5,T6,T7,T8,T9)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5),read<T6>(L,6),read<T7>(L,7),read<T8>(L,8),read<T9>(L,9)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
	struct functor<Rval,T1,T2,T3,T4,T5,T6,T7,T8>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)(T1,T2,T3,T4,T5,T6,T7,T8)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5),read<T6>(L,6),read<T7>(L,7),read<T8>(L,8)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
	struct functor<Rval,T1,T2,T3,T4,T5,T6,T7>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)(T1,T2,T3,T4,T5,T6,T7)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5),read<T6>(L,6),read<T7>(L,7)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
	struct functor<Rval,T1,T2,T3,T4,T5,T6>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)(T1,T2,T3,T4,T5,T6)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5),read<T6>(L,6)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5>
	struct functor<Rval,T1,T2,T3,T4,T5>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)(T1,T2,T3,T4,T5)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4>
	struct functor<Rval,T1,T2,T3,T4>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)(T1,T2,T3,T4)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T1,typename T2,typename T3>
	struct functor<Rval,T1,T2,T3>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)(T1,T2,T3)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T1,typename T2>
	struct functor<Rval,T1,T2>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)(T1,T2)>(L)(read<T1>(L,1),read<T2>(L,2)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T1>
	struct functor<Rval,T1>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)(T1)>(L)(read<T1>(L,1)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval>
	struct functor<Rval>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,upvalue_<Rval(*)()>(L)());
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
	struct functor<void,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5),read<T6>(L,6),read<T7>(L,7),read<T8>(L,8),read<T9>(L,9),read<T10>(L,10));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	
	template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
	struct functor<void,T1,T2,T3,T4,T5,T6,T7,T8,T9>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)(T1,T2,T3,T4,T5,T6,T7,T8,T9)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5),read<T6>(L,6),read<T7>(L,7),read<T8>(L,8),read<T9>(L,9));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
	struct functor<void,T1,T2,T3,T4,T5,T6,T7,T8>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)(T1,T2,T3,T4,T5,T6,T7,T8)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5),read<T6>(L,6),read<T7>(L,7),read<T8>(L,8));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
	struct functor<void,T1,T2,T3,T4,T5,T6,T7>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)(T1,T2,T3,T4,T5,T6,T7)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5),read<T6>(L,6),read<T7>(L,7));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
	struct functor<void,T1,T2,T3,T4,T5,T6>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)(T1,T2,T3,T4,T5,T6)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5),read<T6>(L,6));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T1,typename T2,typename T3,typename T4,typename T5>
	struct functor<void,T1,T2,T3,T4,T5>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)(T1,T2,T3,T4,T5)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),
				read<T5>(L,5));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T1,typename T2,typename T3,typename T4>
	struct functor<void,T1,T2,T3,T4>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)(T1,T2,T3,T4)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T1,typename T2,typename T3>
	struct functor<void,T1,T2,T3>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)(T1,T2,T3)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T1,typename T2>
	struct functor<void,T1,T2>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)(T1,T2)>(L)(read<T1>(L,1),read<T2>(L,2));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T1>
	struct functor<void,T1>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)(T1)>(L)(read<T1>(L,1));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<>
	struct functor<void>
	{
		static int invoke(lua_State* L)
		{
			TRY_LUA_TINKER_INVOKE()
			{
				upvalue_<void(*)()>(L)();
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	
	//这里是设置闭包，前面还是需要push upvalue的操作，这个一般用来设置全局函数用的
	template<typename Rval>
	void push_functor(lua_State *L,Rval(*func)())
	{
		lua_pushcclosure(L,functor<Rval>::invoke,1);
	}
	
	template<typename Rval,typename T1>
	void push_functor(lua_State *L,Rval(*func)(T1))
	{
		lua_pushcclosure(L,functor<Rval,T1>::invoke,1);
	}
	
	template<typename Rval,typename T1,typename T2>
	void push_functor(lua_State *L,Rval(*func)(T1,T2))
	{
		lua_pushcclosure(L,functor<Rval,T1,T2>::invoke,1);
	}
	
	template<typename Rval,typename T1,typename T2,typename T3>
	void push_functor(lua_State *L,Rval(*func)(T1,T2,T3))
	{
		lua_pushcclosure(L,functor<Rval,T1,T2,T3>::invoke,1);
	}
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4>
	void push_functor(lua_State *L,Rval(*func)(T1,T2,T3,T4))
	{
		lua_pushcclosure(L,functor<Rval,T1,T2,T3,T4>::invoke,1);
	}
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5>
	void push_functor(lua_State *L,Rval(*func)(T1,T2,T3,T4,T5))
	{
		lua_pushcclosure(L,functor<Rval,T1,T2,T3,T4,T5>::invoke,1);
	}
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
	void push_functor(lua_State *L,Rval(*func)(T1,T2,T3,T4,T5,T6))
	{
		lua_pushcclosure(L,functor<Rval,T1,T2,T3,T4,T5,T6>::invoke,1);
	}
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
	void push_functor(lua_State *L,Rval(*func)(T1,T2,T3,T4,T5,T6,T7))
	{
		lua_pushcclosure(L,functor<Rval,T1,T2,T3,T4,T5,T6,T7>::invoke,1);
	}
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
	void push_functor(lua_State *L,Rval(*func)(T1,T2,T3,T4,T5,T6,T7,T8))
	{
		lua_pushcclosure(L,functor<Rval,T1,T2,T3,T4,T5,T6,T7,T8>::invoke,1);
	}
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
	void push_functor(lua_State *L,Rval(*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9))
	{
		lua_pushcclosure(L,functor<Rval,T1,T2,T3,T4,T5,T6,T7,T8,T9>::invoke,1);
	}
	
	template<typename Rval,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
	void push_functor(lua_State *L,Rval(*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10))
	{
		lua_pushcclosure(L,functor<Rval,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>::invoke,1);
	}
	
	struct var_base
	{
		virtual ~var_base(){}
		virtual void get(lua_State *L) = 0;
		virtual void set(lua_State *L) = 0;
	};
	
	//在类T的基础上找到偏移的指针，class_add和class_men加入的指针是偏移用？
	template<typename T,typename V>
	struct mem_var: var_base
	{
		V T::*_var;
		mem_var(V T::*val): _var(val){}
		void get(lua_State *L)
		{
			CHECK_CLASS_PTR(T);
			push(L,read<T*>(L,1)->*(_var));
		}
		void set(lua_State *L)
		{
			CHECK_CLASS_PTR(T);
			read<T*>(L,1)->*(_var) = read<V>(L,3);
		}
	};
	
	template<typename Rval,typename T,typename T1=void,typename T2=void,typename T3=void,typename T4=void,typename T5=void,typename T6=void,typename T7=void,typename T8=void,typename T9=void,typename T10=void>
	struct mem_functor
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6),read<T6>(L,7),read<T7>(L,8),read<T8>(L,9),read<T9>(L,10),read<T10>(L,11)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
	struct mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7,T8,T9>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)(T1,T2,T3,T4,T5,T6,T7,T8,T9)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6),read<T6>(L,7),read<T7>(L,8),read<T8>(L,9),read<T9>(L,10)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
	struct mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7,T8>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)(T1,T2,T3,T4,T5,T6,T7,T8)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6),read<T6>(L,7),read<T7>(L,8),read<T8>(L,9)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
	struct mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)(T1,T2,T3,T4,T5,T6,T7)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6),read<T6>(L,7),read<T7>(L,8)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
	struct mem_functor<Rval,T,T1,T2,T3,T4,T5,T6>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)(T1,T2,T3,T4,T5,T6)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6),read<T6>(L,7)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5>
	struct mem_functor<Rval,T,T1,T2,T3,T4,T5>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)(T1,T2,T3,T4,T5)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4>
	struct mem_functor<Rval,T,T1,T2,T3,T4>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)(T1,T2,T3,T4)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3>
	struct mem_functor<Rval,T,T1,T2,T3>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)(T1,T2,T3)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T,typename T1,typename T2>
	struct mem_functor<Rval,T,T1,T2>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)(T1,T2)>(L))(read<T1>(L,2),read<T2>(L,3)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T,typename T1>
	struct mem_functor<Rval,T,T1>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)(T1)>(L))(read<T1>(L,2)));
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T>
	struct mem_functor<Rval,T>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				push(L,(read<T*>(L,1)->*upvalue_<Rval(T::*)()>(L))());
				return 1;
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
	struct mem_functor<void,T,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6),read<T6>(L,7),read<T7>(L,8),read<T8>(L,9),read<T9>(L,10),read<T10>(L,11));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
	struct mem_functor<void,T,T1,T2,T3,T4,T5,T6,T7,T8,T9>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4,T5,T6,T7,T8,T9)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6),read<T6>(L,7),read<T7>(L,8),read<T8>(L,9),read<T9>(L,10));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
	struct mem_functor<void,T,T1,T2,T3,T4,T5,T6,T7,T8>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4,T5,T6,T7,T8)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6),read<T6>(L,7),read<T7>(L,8),read<T8>(L,9));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
	struct mem_functor<void,T,T1,T2,T3,T4,T5,T6,T7>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4,T5,T6,T7)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6),read<T6>(L,7),read<T7>(L,8));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
	struct mem_functor<void,T,T1,T2,T3,T4,T5,T6>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4,T5,T6)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6),read<T6>(L,7));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T,typename T1,typename T2,typename T3,typename T4,typename T5>
	struct mem_functor<void,T,T1,T2,T3,T4,T5>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4,T5)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),
				read<T5>(L,6));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T,typename T1,typename T2,typename T3,typename T4>
	struct mem_functor<void,T,T1,T2,T3,T4>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T,typename T1,typename T2,typename T3>
	struct mem_functor<void,T,T1,T2,T3>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T,typename T1,typename T2>
	struct mem_functor<void,T,T1,T2>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2)>(L))(read<T1>(L,2),read<T2>(L,3));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T,typename T1>
	struct mem_functor<void,T,T1>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)(T1)>(L))(read<T1>(L,2));
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename T>
	struct mem_functor<void,T>
	{
		static int invoke(lua_State* L)
		{
			CHECK_CLASS_PTR(T);
			TRY_LUA_TINKER_INVOKE()
			{
				(read<T*>(L,1)->*upvalue_<void(T::*)()>(L))();
			}
			CATCH_LUA_TINKER_INVOKE()
			{
				lua_pushfstring(L,"lua fail to invoke functor");
				lua_error(L);
			}
			return 0;
		}
	};
	
	template<typename Rval,typename T>
	void push_functor(lua_State *L,Rval(T::*func)())
	{
		lua_pushcclosure(L,mem_functor<Rval,T>::invoke,1);
	}
	
	template<typename Rval,typename T>
	void push_functor(lua_State *L,Rval(T::*func)() const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1>
	void push_functor(lua_State *L,Rval(T::*func)(T1))
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1>
	void push_functor(lua_State *L,Rval(T::*func)(T1) const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2))
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2)const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3))
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3) const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4))
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4) const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4,T5))
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4,T5) const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4,T5,T6))
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5,T6>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4,T5,T6)const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5,T6>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4,T5,T6,T7))
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4,T5,T6,T7)const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4,T5,T6,T7,T8))
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7,T8>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4,T5,T6,T7,T8)const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7,T8>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
	void push_functor(lua_State *L,Rval(T::*func)(T,T1,T2,T3,T4,T5,T6,T7,T8,T9))
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7,T8,T9>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
	void push_functor(lua_State *L,Rval(T::*func)(T,T1,T2,T3,T4,T5,T6,T7,T8,T9)const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7,T8,T9>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10))
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>::invoke,1);
	}
	
	template<typename Rval,typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
	void push_functor(lua_State *L,Rval(T::*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10)const)
	{
		lua_pushcclosure(L,mem_functor<Rval,T,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>::invoke,1);
	}
	
	//这里的construct函数是直接生成一个user对象然后再new 一个T对象，用指针指向它
	//然后设置metatable，最后返回它
	template<typename T,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
	int constructor(lua_State *L)
	{
		new(lua_newuserdata(L,sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6),read<T6>(L,7));
		push_meta(L,class_name<typename class_type<T>::type>::name());
		lua_setmetatable(L,-2);
		return 1;
	}
	
	template<typename T,typename T1,typename T2,typename T3,typename T4,typename T5>
	int constructor(lua_State *L)
	{
		new(lua_newuserdata(L,sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6));
		push_meta(L,class_name<typename class_type<T>::type>::name());
		lua_setmetatable(L,-2);
		return 1;
	}
	
	template<typename T,typename T1,typename T2,typename T3,typename T4>
	int constructor(lua_State *L)
	{
		new(lua_newuserdata(L,sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5));
		push_meta(L,class_name<typename class_type<T>::type>::name());
		lua_setmetatable(L,-2);
		return 1;
	}
	
	template<typename T,typename T1,typename T2,typename T3>
	int constructor(lua_State *L)
	{
		new(lua_newuserdata(L,sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4));
		push_meta(L,class_name<typename class_type<T>::type>::name());
		lua_setmetatable(L,-2);
		return 1;
	}
	
	template<typename T,typename T1,typename T2>
	int constructor(lua_State *L)
	{
		new(lua_newuserdata(L,sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3));
		push_meta(L,class_name<typename class_type<T>::type>::name());
		lua_setmetatable(L,-2);
		return 1;
	}
	
	template<typename T,typename T1>
	int constructor(lua_State *L)
	{
		new(lua_newuserdata(L,sizeof(val2user<T>))) val2user<T>(read<T1>(L,2));
		push_meta(L,class_name<typename class_type<T>::type>::name());
		lua_setmetatable(L,-2);
		return 1;
	}
	
	template<typename T>
	int constructor(lua_State *L)
	{
		new(lua_newuserdata(L,sizeof(val2user<T>))) val2user<T>();
		push_meta(L,class_name<typename class_type<T>::type>::name());
		lua_setmetatable(L,-2);
		return 1;
	}
	
	template<typename T>
	int destroyer(lua_State *L)
	{
		((user*)lua_touserdata(L,1))->~user();
		return 0;
	}
	
	template<typename F>
	void def(lua_State* L,const char* name,F func)
	{
		//lua_pushstring(L,name);
		lua_pushlightuserdata(L,(void*)func);
		push_functor(L,func);
		lua_setglobal(L,name);
	}
	
	template<typename T>
	void set(lua_State* L,const char* name,T object)
	{
		//lua_pushstring(L,name);
		push(L,object);
		lua_setglobal(L,name);
	}
	
	//话说最后的userdata如何释放
	template<typename T>
	T get(lua_State* L,const char* name)
	{
		//lua_pushstring(L,name);
		lua_getglobal(L,name);
		return pop<T>(L);
	}
	
	template<typename T>
	void decl(lua_State* L,const char* name,T object)
	{
		set(L,name,object);
	}
	
	template<typename RVal>
	RVal call(lua_State* L,const char* name)
	{
		lua_pushcclosure(L,on_error,0);
		int errfunc = lua_gettop(L);
		lua_getglobal(L,name);
		if(lua_isfunction(L,-1))
		{
			lua_pcall(L,0,1,errfunc);
		}
		else
		{
			print_error(L,"lua_tinker::call() attempt to call global %s (not a function)",name);
		}
		lua_remove(L,errfunc);
		return pop<RVal>(L);
	}
	
	template<typename RVal,typename...Args>
	RVal call(lua_State* L,const char* name,Args...arg)
	{
		lua_pushcclosure(L,on_error,0);
		int errfunc = lua_gettop(L);
		lua_getglobal(L,name);
		if(lua_isfunction(L,-1))
		{
			push_args(L,arg...);
			if(lua_pcall(L,sizeof...(Args),1,errfunc) != 0)
			{
				lua_pop(L,1);
			}
		}
		else
		{
			print_error(L,"lua_tinker::call() attempt to call global '%s' (not a function)",name);
		}
		lua_remove(L,-2);
		return pop<RVal>(L);
	}
	
	template<typename T>
	void class_add(lua_State* L,const char* name)
	{
		class_name<T>::name(name);
		lua_newtable(L);
		
		lua_pushstring(L,"__name");
		lua_pushstring(L,name);
		lua_rawset(L,-3);
		
		lua_pushstring(L,"__index");
		lua_pushcclosure(L,meta_get,0);
		lua_rawset(L,-3);
		
		lua_pushstring(L,"__newindex");
		lua_pushcclosure(L,meta_set,0);
		lua_rawset(L,-3);
		
		lua_pushstring(L,"__gc");
		lua_pushcclosure(L,destroyer<T>,0);
		lua_rawset(L,-3);
		
		lua_setglobal(L,name);
	}
	template<typename T,typename P>
	void class_inh(lua_State* L)
	{
		push_meta(L,class_name<T>::name());
		if(lua_istable(L,-1))
		{
			lua_pushstring(L,"__parent");
			push_meta(L,class_name<P>::name());
			lua_rawset(L,-3);
		}
		lua_pop(L,1);
	}
	
	//调用方式是用类名（）这种方式，所以用__call来声明
	//func就是constructor函数
	template<typename T,typename F>
	void class_con(lua_State* L,F func)
	{
		push_meta(L,class_name<T>::name());
		if(lua_istable(L,-1))
		{
			lua_newtable(L);
			lua_pushstring(L,"__call");
			lua_pushcclosure(L,func,0);
			lua_rawset(L,-3);
			lua_setmetatable(L,-2);
		}
		lua_pop(L,1);
	}
	
	template<typename T,typename F>
	void class_def(lua_State* L,const char* name,F func)
	{
		push_meta(L,class_name<T>::name());
		if(lua_istable(L,-1))
		{
			lua_pushstring(L,name);
			new(lua_newuserdata(L,sizeof(F)))F(func);
			push_functor(L,func);
			lua_rawset(L,-3);
		}
		lua_pop(L,1);
	}
	
	template<typename T,typename BASE,typename VAR>
	void class_mem(lua_State* L,const char* name,VAR BASE::*val)
	{
		push_meta(L,class_name<T>::name());
		if(lua_istable(L,-1))
		{
			lua_pushstring(L,name);
			new(lua_newuserdata(L,sizeof(mem_var<BASE,VAR>)))mem_var<BASE,VAR>(val);
			lua_rawset(L,-3);
		}
		lua_pop(L,1);
	}
	
	
	struct table_obj
	{
		table_obj(lua_State* L,int index);
		~table_obj();
		
		void inc_ref();
		void dec_ref();
		
		bool validate();
		
		template<typename T>
		void set(const char* name,T object)
		{
			if(validate())
			{
				lua_pushstring(m_L,name);
				push(m_L,object);
				lua_settable(m_L,m_index);
			}
		}
		template<typename T>
		T get(const char* name)
		{
			if(validate())
			{
				lua_pushstring(m_L,name);
				lua_gettable(m_L,m_index);
			}
			else
			{
				lua_pushnil(m_L);
			}
			return pop<T>(m_L);
		}
		
		template<typename T>
		T get(int num)
		{
			if(validate())
			{
				lua_pushinteger(m_L,num);
				lua_gettable(m_L,m_index);
			}
			else
			{
				lua_pushnil(m_L);
			}
			return pop<T>(m_L);
		}
		int size(){return m_size;}
		lua_State* m_L;
		int m_index;  //在堆栈的那个位置
		const void* m_pointer;
		int m_ref;
		int m_size;
	};
	
	struct table
	{
		table(lua_State* L);
		table(lua_State* L,int index);
		table(lua_State* L,const char* name);
		table(const table& input);
		~table();
		
		template<typename T>
		void set(const char* name, T object)
		{
			m_obj->set(name,object);
		}
		template<typename T>
		void set(int nIndex,T object)
		{
			m_obj->set(nIndex,object);
		}
		template<typename T>
		T get(const char* name)
		{
			return m_obj->get<T>(name);
		}
		template<typename T>
		T get(int nIndex)
		{
			return m_obj->get<T>(nIndex);
		}
		int size()
		{
			return m_obj->size();
		}
		table_obj* m_obj;
	};
}

typedef lua_tinker::table LuaTable;

#endif