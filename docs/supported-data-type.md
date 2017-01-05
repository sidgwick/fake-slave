# Data Types Supported

## Numeric Types

|  Type      |  Binlog Type            |  Status          |
| ---------- | ----------------------- | ---------------- |
|  TINYINT   |  MYSQL_TYPE_TINY        |  fully supported |
|  SMALLINT  |  MYSQL_TYPE_SHORT       |  fully supported |
|  MEDIUMINT |  MYSQL_TYPE_INT24       |  fully supported |
|  INT       |  MYSQL_TYPE_LONG        |  fully supported |
|  BIGINT    |  MYSQL_TYPE_LONGLONG    |  fully supported |
|  FLOAT     |  MYSQL_TYPE_FLOAT       |  loss precision  |
|  DOUBLE    |  MYSQL_TYPE_DOUBLE      |  loss precision  |
|  DECIMAL   |  MYSQL_TYPE_NEWDECIMAL  |  supported       |


## Temporal Types

|  Type      |  Binlog Type            |  Status          |
| ---------- | ----------------------- | ---------------- |
|  TIMESTAMP |  MYSQL_TYPE_TIMESTAMP2  |  fully supported |
|  DATETIME  |  MYSQL_TYPE_DATETIME2   |  fully supported |
|  TIMESTAMP |  MYSQL_TYPE_TIMESTAMP   |  Untested |
|  DATETIME  |  MYSQL_TYPE_DATETIME    |  Untested |
|  DATE      |  MYSQL_TYPE_DATE        |  Untested |
|  TIME      |  MYSQL_TYPE_TIME        |  Untested |
|  TIME      |  MYSQL_TYPE_TIME2       |  fully supported |
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
