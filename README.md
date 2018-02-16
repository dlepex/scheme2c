### scheme2c

The code was written zillions years ago (in 2009).  Basically I always wanted to check how hard is it to write your own interpreter for a functional programming language (Scheme in this case).

It wasn't especially challenging and probably the only tricky part was debugging GC memory errors

#### Features

- The interpreter is based on transforming (linearizing) AST into the A-normal form: https://en.wikipedia.org/wiki/A-normal_form
- Tail recursion support. Without tail recursion you can't write optimal loops in Scheme. Note that mutually recursive functions are not optimized.
- GC is precise, it uses Cheney algorithm and have 2 generations (young & old)
- REPL
- Translating Scheme programes into C. The ouput looks nothing like normal C code, but it's 2x times faster than interpreter. Translated code can call interpreted code and vice versa.
- [] can be used as an alternative to () for clarity

#### What is missing

The implementation is very limited and many things are missing. Notably macros and continuations (call/cc). But it's not hard to add call/cc support at that point and macro-system can be implemented in Scheme itself. It's also quite easy to add missing BIFs, see the end of "libr.c" file.

See "test" folder for the samples of Scheme code, that can be run by interpreter.

#### Modules

Brief modules description:

- *parse*: hand written recursive descent parser
- *transform*: transformation to ANF
- *interpret*: interpreter loop
- *memmgr*: GC implementation https://en.wikipedia.org/wiki/Cheney%27s_algorithm
- *datum*: Scheme data-structures: vectors and hashtables etc
- *libr*: BIFs (built-in functions)
- *ioport*: Scheme IO
- *main*: entry point, console options

Header files started with double underscore are needed for the translation mode only.





