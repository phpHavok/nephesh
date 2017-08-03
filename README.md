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
<pipeline>       ::= STR <pipeline-more>
<pipeline-more>  ::= <pipe> <pipeline>
                 ::= LAMBDA
<pipe>           ::= <unary-pipe>
                 ::= <nary-pipe>
<unary-pipe>     ::= <maybe-fd> PIPE <maybe-fd>
<nary-pipe>      ::= LT <unary-pipe> <nary-pipe-more> GT
<nary-pipe-more> ::= <unary-pipe> <nary-pipe-more>
                 ::= LAMBDA
<maybe-fd>       ::= STR
                 ::= LAMBDA
```

### Sets

| Non-terminal (N) | First(N)              | Follow(N)     |
| ---------------- | --------------------- | ------------- |
| pipeline         | STR                   | EOF           |
| pipeline-more    | STR, PIPE, LT, LAMBDA | EOF           |
| pipe             | STR, PIPE, LT         | STR           |
| unary-pipe       | STR, PIPE             | STR, PIPE, GT |
| nary-pipe        | LT                    | STR           |
| nary-pipe-more   | STR, PIPE, LAMBDA     | GT            |
| maybe-fd         | STR, LAMBDA           | STR, PIPE, GT |
