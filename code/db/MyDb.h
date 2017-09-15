#ifndef _MYDB_
#define _MYDB_

#include <mysql/mysql.h>

#define EXTERN_C_DEFINE
//因为这几个文件会相互包含所以特意组成一个文件
#include "MyDatabase.h"
#include "MyField.h"
#include "MyRecord.h"
#include "MyRecordset.h"

static const char DEFALT_SQL_STMT[] = "SELECT * FROM";
static const char SQL_STMT_SHOWDATABASES[] = "WHOW DATABASES";
static const char SQL_STMT_SHOWTABLES[] = "SHOW TABLES";

static const char SQL_STMT_DELETE[] = "DELETE FROM %s %s WHERE %s";
static const char SQL_STMT_UPDATE[] = "UPDATE %s SET %s WHERE %s LIMIT 1";
static const char SQL_STMT_INSERT[] = "INSERT %s SET %s";

#endif