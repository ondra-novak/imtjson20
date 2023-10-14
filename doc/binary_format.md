# Binary format

The binary format is stored as variant similar to TLV (Tag Length Value)

## Type

length of type is 1 byte, This byte is futher split to two parts

* **tag**
* **arg** = which acts as argument of tag, or size of length in bytes

```
+---+---+---+---+---+---+---+---+
|     tag           |   arg     |
+---+---+---+---+---+---+---+---+
```

### Tags
```
+-----------+------------------------------------+-------+
| 00000 (0) |    special types                   | 00-07 |
|           |    arg = 0 - null                  |       |
|           |    arg = 1 - true                  |       |
|           |    arg = 2 - false                 |       |
|           |    arg = 7 - undefined             |       |
+-----------+------------------------------------+-------+
| 0001X(2-3)|  integer number (X = sign), arg=len| 10-1F |
+-----------+------------------------------------+-------+
| 00100 (4) |  string, arg=size of length        | 20-27 |
+-----------+------------------------------------+-------+
| 00101 (5) |  number string arg=size of length  | 28-2F |
+-----------+------------------------------------+-------+
| 00110 (6) |  array arg=size of length          | 30-37 |
+-----------+------------------------------------+-------+
| 00111 (7) |  object arg=size of length         | 37-3F |
+-----------+------------------------------------+-------+
```
### Storing integer number 

when `arg` contains length, it specifies count of bytes used to store integer
 number. This argument can be between 0 and 7. You need to add +1 to the
 number, to retrieve count of bytes
 
 So 0x10 is integer which as 1 byte, 0x11 is integer with two bytes, 0x17 has 8 bytes
 
```
10 0A - number 10
11 AB CD - number 0xABCD
12 12 AB EF - number 0x12ABEF
1A 12 AB EF - number -0x12ABEF (negative number)
```
 
### Storing length

Length is stored as unsigned integer number

```
20 0A - string has 10 characters 
31 12 34 - array has 0x1234 items [...]
37 00 - empty object {}
```

### Example

```
38 0B 20 03 61 61 61 30 03 10 01 10 02 10 03 ...
```
* **38** - Object, size of length is 1 byte
* **0B** - Object has 11 itmes
* **20** - String(key), size of length is 1 byte 
* **03** - String(key) has 3 bytes
* **61** - 3x 61 = "aaa"
* **30** - Array, size of length is 1 byte
* **03** - Array has 3 items
* **10** - integer, 1 byte
* **01** - value 1
* **10** - integer, 1 byte
* **02** - value 2
* **10** - integer, 1 byte
* **03** - value 3

```
{"aaa":[1,2,3],....}
```

