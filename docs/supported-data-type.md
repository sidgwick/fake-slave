# Data Types Supported

## Numeric Types

|  Type      |  Binlog Type            |  Status          |
| ---------- | ----------------------- | ---------------- |
|  TINYINT   |  MYSQL_TYPE_TINY        |  Fully supported |
|  SMALLINT  |  MYSQL_TYPE_SHORT       |  Fully supported |
|  MEDIUMINT |  MYSQL_TYPE_INT24       |  Fully supported |
|  INT       |  MYSQL_TYPE_LONG        |  Fully supported |
|  BIGINT    |  MYSQL_TYPE_LONGLONG    |  Bug |
|  FLOAT     |  MYSQL_TYPE_FLOAT       |  Untested |
|  DOUBLE    |  MYSQL_TYPE_DOUBLE      |  Untested |
|  DECIMAL   |  MYSQL_TYPE_NEWDECIMAL  |  Untested |


## Temporal Types

|  Type      |  Binlog Type            |  Status          |
| ---------- | ----------------------- | ---------------- |
|  TIMESTAMP |  MYSQL_TYPE_TIMESTAMP   |  Untested |
|  DATETIME  |  MYSQL_TYPE_DATETIME    |  Untested |
|  DATE      |  MYSQL_TYPE_DATE        |  Untested |
|  TIME      |  MYSQL_TYPE_TIME        |  Untested |
|  YEAR      |  MYSQL_TYPE_YEAR        |  Untested |


## String Types

|  Type                               |  Binlog Type            |  Status          |
| ----------------------------------- | ----------------------- | ---------------- |
|  CHAR VARCHAR                       |  MYSQL_TYPE_STRING      |  Untested |
|  TINYBLOB BLOB MEDIUMBLOB LONGBLOB  |  MYSQL_TYPE_BLOB        |  Untested |


## Other Types

|  Type      |  Binlog Type            |  Status          |
| ---------- | ----------------------- | ---------------- |
|  ENUM      |  MYSQL_TYPE_STRING      |  Untested |
|  SET       |  MYSQL_TYPE_STRING      |  Untested |
|  BIT       |  MYSQL_TYPE_BIT         |  Untested |
|  GEOMETRY  |  MYSQL_TYPE_GEOMETRY    |  Untested |

# Test SQL
