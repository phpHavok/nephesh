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
<pipeline-more>  ::= <nary-pipe> <pipeline>
                 ::= LAMBDA
<unary-pipe>     ::= <maybe-fd> PIPE <maybe-fd>
<nary-pipe>      ::= LT <unary-pipe> <nary-pipe-more> GT
<nary-pipe-more> ::= <unary-pipe> <nary-pipe-more>
                 ::= LAMBDA
<maybe-fd>       ::= STR
                 ::= AT
                 ::= LAMBDA
```

### Sets

| Non-terminal (N) | First(N)              | Follow(N)         |
| ---------------- | --------------------- | ----------------- |
| pipeline         | STR                   | EOF               |
| str-more         | STR, LAMBDA           | LT, EOF           |
| pipeline-more    | LT, LAMBDA            | EOF               |
| unary-pipe       | STR, AT, PIPE         | STR, AT, PIPE, GT |
| nary-pipe        | LT                    | STR               |
| nary-pipe-more   | STR, AT, PIPE, LAMBDA | GT                |
| maybe-fd         | STR, AT, LAMBDA       | STR, AT, PIPE, GT |
