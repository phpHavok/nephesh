# nephesh

A really cool shell for GNU/Linux.

## Notes / TODO

- Need to add timeout for matching partial key bindings.
- Scanner should support escapes.

## Scanner

- LT ('<')
- GT ('>')
- PIPE ('|')
- AT ('@')
- STR (TODO: description)

## Parser

`LAMBDA` is the empty string, and `EOF` is the end of the token stream.

```
<pipeline>       ::= STR <str-more> <pipeline-more>
<str-more>       ::= STR <str-more>
                 ::= LAMBDA
<pipeline-more>  ::= <pipe> <pipeline>
                 ::= LAMBDA
<pipe>           ::= <unary-pipe>
                 ::= <nary-pipe>
<unary-pipe>     ::= <maybe-str> PIPE <maybe-str>
<nary-pipe>      ::= LT <unary-pipe> <nary-pipe-more> GT
<nary-pipe-more> ::= <unary-pipe> <nary-pipe-more>
                 ::= LAMBDA
<maybe-str>      ::= STR
                 ::= LAMBDA
```

### Sets

| Non-terminal (N) | First(N)              | Follow(N)          |
| ---------------- | --------------------- | ------------------ |
| pipeline         | STR                   | EOF                |
| str-more         | STR, LAMBDA           | STR, PIPE, LT, EOF |
| pipeline-more    | STR, PIPE, LT, LAMBDA | EOF                |
| pipe             | STR, PIPE, LT         | STR                |
| unary-pipe       | STR, PIPE             | STR, PIPE, GT      |
| nary-pipe        | LT                    | STR                |
| nary-pipe-more   | STR, PIPE, LAMBDA     | GT                 |
| maybe-str        | STR, LAMBDA           | STR, PIPE, GT      |
