# (Experimental) Ruby Rope extension written in C

These codes are written as a part of [Google Summer of Code 2016](https://summerofcode.withgoogle.com/projects/).

[Automatic-selection mechanism for data structures in MRI](https://summerofcode.withgoogle.com/projects/#4576418910437376)

Prototypes of [Rope data structure](https://en.wikipedia.org/wiki/Rope_(data_structure)) for String class of Ruby.

## Short Explanation
* I implemented concatenation, substring, indexing operations and iterator over string as a data structure in C. (in src/)
 * Memory management are done by reference count, and hence good performance is not obtained without memory leak by stopping reference count.
* After that, I wrapped it as an extension of Ruby String object which have methods such as eql, +, concat, length, size, [], delete\_at, slice, at, to\_s, to\_str, inspect, dump. (in ext/rope, especially rb_rope.c is implementation of Rope class)
 * Wrapped as Data class in Ruby (Details about Ruby extension: http://docs.ruby-lang.org/en/2.3.0/extension_rdoc.html)
* Finally, I wrote Ruby class ERope(Elastic Rope) for an evaluation of prototyping and dynamic-selection of data structure. ERope class is just a thin wrapper of Ruby string class and Rope class written above, and it can concatenate string as a rope. (in ext/erope, codes here are only for an experiment and not mature at all)

## Build
(It may be needed to fix Rakefile, e.g. CC=clang)

``` sh
rake
```
