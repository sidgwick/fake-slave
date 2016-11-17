# Numberic test

## Input

```sql
CREATE TABLE `numberic` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `tiny` tinyint(4) DEFAULT NULL,
  `short` smallint(6) DEFAULT NULL,
  `medium` mediumint(9) DEFAULT NULL,
  `big` bigint(20) DEFAULT NULL,
  `float_number` float(5,2) DEFAULT NULL,
  `double_number` double(9,4) DEFAULT NULL,
  `decimal_number` decimal(9,4) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

insert into numberic values ('', 10, 135, 102400, 102400000, 1.235, 12789.12356, 12396.2310);
insert into numberic values ('', -10, -135, -102400, -102400000, -1.235, -12789.12356, -12396.2310);
```


## Excpet output

```
Column def: 03, ID: 0 value: LONG 12354
Column def: 01, ID: 1 value: TINY INT: 10
Column def: 02, ID: 2 value: SHORT INT: 135
Column def: 09, ID: 3 value: MEDIUM 102400
Column def: 08, ID: 4 value: LONG LONG INT: 102400000
Column def: 04, ID: 5 value: FLOAT: 1.235000
Column def: 05, ID: 6 value: DOUBLE 12789.123600
Column def: f6, ID: 7 value: Decimal: 12396.2310
```

```
Column def: 03, ID: 0 value: LONG 12355
Column def: 01, ID: 1 value: TINY INT: -10
Column def: 02, ID: 2 value: SHORT INT: -135
Column def: 09, ID: 3 value: MEDIUM -102400
Column def: 08, ID: 4 value: LONG LONG INT: -102400000
Column def: 04, ID: 5 value: FLOAT: -1.235000
Column def: 05, ID: 6 value: DOUBLE -12789.123600
Column def: f6, ID: 7 value: Decimal: -12396.2310
```


## Temporal Types


## Input

```sql
CREATE TABLE `temporal` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `tm_timestamp` timestamp,
  `tm_datetime` datetime,
  `tm_date` date,
  `tm_time` time,
  `tm_year` year,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- insert into temporal values ('', unix_timestamp(), now(), curdate(), curtime(), year(now()));
insert into temporal values ('', '2016-11-17 21:08:05', '2016-11-17 21:08:05', '2016-11-17', '21:08:05', 2016);
```
