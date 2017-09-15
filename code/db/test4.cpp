#include <stdio.h>
#include <mysql/mysql.h>
#include "../BaseCode.h"

int main()
{
	MYSQL *conn_ptr;
	conn_ptr = mysql_init(NULL);
	if(!conn_ptr)
	{
		printf("mysql init error\n");
		return 1;
	}
	if(!mysql_real_connect(conn_ptr,"localhost","root","123456","test",MYSQL_PORT,NULL,0))
	{
		printf("mysql real connect error\n");
	}
	printf("myql real connect success \n");
	const char* szSQL = "select * from user;";
	if(mysql_query(conn_ptr,szSQL) != 0)
	{
		printf("log/db DB error:[%u]%s when execute:%s.\n",mysql_errno(conn_ptr),mysql_error(conn_ptr),szSQL);
		mysql_close(conn_ptr);
		return 0;
	}
	MYSQL_RES* pRes = mysql_store_result(conn_ptr);
	if(!pRes)
	{
		printf("log/db pRes is NULL");
	}
	printf("row count %d",(int)(pRes->row_count));
	
	
	mysql_close(conn_ptr);
	return 0;
}