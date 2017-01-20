#include <my_global.h>
#include <mysql.h>

int main(int argc, char **argv)
{
    printf("MySQL client version: %s\n", mysql_get_client_info());

    MYSQL *con = mysql_init(NULL);
    if (con == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con));
        exit(2);
    }

    if (mysql_real_connect(con, "127.0.0.1", "root", "1111", NULL, 0, NULL, 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }

    if (mysql_query(con, "CREATE DATABASE testdb")) {
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }

    int fd = con->net.fd;
    char buf[1024];

    write(fd, "SHOW DATABASES;", 15);
    read(fd, &buf, 1024);

    printf("%s\n", buf);


    mysql_close(con);
    exit(0);

    exit(0);
}
