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

insert into numberic values (12345, 10, 135, 102400, 10240000000000, 12345.235, 123456789.12356, 12345678996.2310);