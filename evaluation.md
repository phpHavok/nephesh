# An evaluation of shells and their features
*Author: Jacob Chappell*

With the advent of the operating system (OS), a mechanism was necessary by which
a human operator could interface with and direct the operating system.  Thus was
born the shell. The traditional shell (and the shell for the purposes of our
future discussions) is operated by means of textual commands through a command
line interface (CLI). However, the modern graphical user interface (GUI) present
in desktop environments can also be thought of as a shell. While the GUI is now
the primary means by which the typical user interfaces with the OS, the GUI has
in no way obsoleted the CLI. There exist many so-called power users that
continue to use CLI shells predominately or even exclusively.

With bash as the de facto standard shell and other well-established options
available for the daring user, by what justification do shells warrant further
discussion today? I choose to answer this question with an analogy. Consider
that nearly all modern programming languages are Turing-complete and thus
equivalent in computational potential. If existing languages have reached the
limits of expressing what is computable, why then are many new languages
developed every year? For that matter, why did language development not cease
at the development of FORTRAN? The truth is, some problems are more easily
expressible in some languages than others. That is, language developers are not
necessarily trying to develop languages that can solve new problems, rather
they are trying to develop languages that allow programmers to more easily
express existing problems. In this sense, modern shells are not necessarily
trying to do more with the underlying OS (although it is not clear to me that
existing shells have exhausted the capabilities of the underlying OS). Rather,
modern shells have the potential to propose new and effective workflows for
system administrators and developers. Even syntactic improvements can increase
productivity and decrease the learning curve of shells.

In this document, I will illustrate by example some of the design decisions a
shell developer faces. The purpose of this experiment is to entice the
interested reader and encourage further exploration and study. Then, I will
evaluate some of the more interesting and specialized features provided by a
couple of existing shells. Namely, I will discuss features of rc, fish, and
(briefly) dgsh.

## Designing a shell from the ground up

Designing a shell, like any sizable piece of software, is an involved process.
There are countless books detailing the design processes of programming
languages, compilers, and even operating systems. However, I haven't found many
good resources on shells in general. I suspect this is due in part to several
things. First, the GUI has superseded the CLI for most computer users. Second,
very few people still write or maintain shells today (possibly due to the first
point). Third, many serious shell power users are seemingly forever devoted to
Bourne shell derivatives.

In this section, we will design a shell from the ground up. We'll call this
shell exsh for "example shell." Through a cycle of discerning what features we
need and determining how and to what extent to implement those features, we will
slowly build up a simple but useable shell. We will discuss both features in the
abstract sense and syntactic considerations. For exsh, we will sometimes assume
an arbitrary choice of syntax for the sake of example.

### Command execution

We'll begin exsh with perhaps the most important and obvious shell feature: the
ability to execute a single command with optional arguments.  The command is
executed in a new foreground process group with its standard input, standard
output, and standard error attached to the controlling terminal of the shell.
We'll henceforth refer to this feature as **command execution**.

Let's begin developing our syntax. Given that we need to execute a command, it's
sensible that merely accepting a Unix path followed by a line feed is
sufficient. The path indicates at which location in the file system the command
to be executed is located. Ignoring the optional arguments for the moment, an
exsh session would be very limited.

```
exsh> /usr/bin/pwd
/home/griff
```

That being said, exsh can already launch vim. What more do you need?

One can imagine other ways of indicating the location of the command to be
executed. For instance, if we were to somehow execute a command from RAM, exsh's
syntax might accept the address in memory (perhaps represented as a hexadecimal
number) at which the command's instructions begin.

```
exsh> 0xdeadbeefcafed00d
/home/griff
```

However, we must remember that we are developing a shell for GNU/Linux, and the
capabilities and architecture of the operating system thus influence the design
of our shell to an extent. Looking ahead, we realize that implementing command
execution will ultimately require a call to `exec`, which accepts a Unix path as
its first parameter.

Unix is designed with the concept of environment variables in mind. A set of
environment variables (key-value pairs) essentially defines an execution context
that can be passed to a process. This is similar to how a C function inherits
global variables in addition to its local parameters. Some environment variables
have been assigned de facto meanings over time. One such environment variable is
`PATH`, which represents a delimited list of directories that can be searched
for binaries which are referenced without their fully-qualified paths. In our
case, we can search `PATH` for commands in order to allow users to specify
commands which are not fully-qualified (commands which do not begin with `/`).

```
exsh> pwd
/home/griff
```

Many commands are limited or useless without arguments. Supporting arguments in
exsh requires two syntactic considerations. First, there must be some way of
delimiting the arguments from the command and from each other. Second, because
`exec` accepts arbitrary strings as arguments, exsh arguments should also allow
arbitrary strings in order to fully take advantage of the operating system's
capabilities. Notably, the method we use for delimiting arguments must somehow
also be interpretable as the contents of an argument rather than as a delimiter.
Formally, the metacharacters of our syntax must be escapable.

At this point, exsh can begin to diverge from other shells. Most shells choose
to delimit arguments with sequences of whitespace (e.g., spaces, tabs).
However, this is an arbitrary choice of syntax that has become a modern
convention. This convention is prevalent to the extent that experienced
GNU/Linux users avoid spaces with a passion. Because of this, we dare not
disturb the delicate hearts of programmers and system administrators. We will
adopt space-delimited arguments for exsh. Also by convention, `\␣` will
represent a literal space character, and, consequently, `\\` will represent a
literal backslash character.

```
exsh> mkdir hello world hello\\\ world
exsh> ls
 hello  'hello\ world'   world
```

### File redirection

Now that we are able to execute commands, we'd like to be able to redirect the
inputs and outputs of our commands. What if we'd like to save the outputs of a
command for later? What if we'd like to discard the outputs of a command? What
if we want a friend to provide the input to a command from a remote terminal and
not the terminal by which exsh is controlled? All of these examples can be
summarized by a feature we'll call **file redirection**.

File redirection is conventionally represented by the `<` and `>`
metacharacters. Precisely, `<` represents a change to standard input while `>`
represents a change to standard output. The following example reads from the
file `file.in` and writes to the file `file.out`. This is effectively the same
as running `cp file.in file.out`.

```
exsh> cat < file.in > file.out
```

#### Arbitrary file redirection

Shells differ somewhat in how they handle redirecting standard error. Some
shells introduce another metacharacter (such as `^`). Other shells support
overloading the `<` and `>` operators by specifying the file descriptor of the
stream the user wishes to redirect for reading or writing. If a shell implements
the latter, we will say that the shell supports **arbitrary file redirection**.

The following example will search the entire file system for the string `test`,
sending results to the terminal and any reported errors to the file `error.log`.

```
exsh> grep -r test / 2> error.log
```

### File duplication

Oftentimes, programs will distinguish between different types of outputs by
writing to different file descriptors. This is especially true of standard
output, which typically contains only "normal" outputs, versus standard error,
which typically contains errors, warnings, and debug messages. However, a user
may wish to aggregate all of a program's outputs into a single stream. This can
be accomplished in shells that support a feature we'll call **file
duplication**.

File duplication is very much a related feature to file redirection. However, I
think it warrants its own section as it does require syntactic considerations.
Shells handle file duplication in various ways, usually with a special operator
like `>&`. A clever shell may be able to overload file redirection semantics to
support file duplication inherently. The following example illustrates
aggregating standard input and standard error into a single stream which is then
saved into an `aggregate.log` file.

```
exsh> grep -r test / > aggregate.log 2>&1
```

## Interesting features of exotic shells

Following is a brief evaluation of a few shells that I found to be interesting
in one way or another. These shells have contributed to the inspiration and
design of nephesh. For the purposes of this document, "exotic" is defined
subjectively to mean "any shell that I found interesting enough to write about."
This document may grow and morph over time.

### rc

Rc, written by Tom Duff, is the flagship shell of the Plan 9 operating system.
While roughly backwards compatible with the Bourne shell in simplistic cases,
rc features a cleaner design, a formal grammar, and some nice features. The
effort to include a level of backwards compatibility with the Bourne shell did,
in some sense, inhibit the innovation of rc. As Duff writes [1], "Any successor
of the Bourne shell is bound to suffer in comparison."

Duff designed rc under the realization that, in reality, shells mostly deal
with arrays. For example, the most common shell action is to execute a command
with an *array* of arguments. As such, variables in rc are array-valued rather
than string-valued. More precisely, variables in rc are arrays of strings. A
scalar string is thus represented as an array with one element. Undefined
variables are equivalent to the empty array, which is different from the array
containing the empty string.

The decision to implement array-valued variables in rc was made as a
consequence of an important design decision: commands should be single-pass
interpreted. Duff did not want to deal with the counter-intuitive and
unpleasant behavior the Bourne shell introduces by parsing commands a second
time after variable substitution is performed. Bourne's need for this second
pass is twofold. In variable assignment, Bourne needs a second pass in order to
handle arrays. Furthermore, Bourne needs to be able to handle nested command
substitutions, which must be escaped. The problem of nested command
substitution is handled in rc by use of a unary command substitution argument.

Rc is somewhat agnostic about the interface you use for command line editing.
For example, you can link rc against GNU readline in order to use many of the
editing features bash provides (which also uses GNU readline). Other options
are also available.

Several rc features are listed below, many of which are present in other
shells. This is not an exhaustive list.

- Arbitrary file redirection
- Command substitution
- Pipelines (with branching)
- Foreground/background processes with basic job control
- Command grouping
- Control structures (e.g., if, for, while, etc.)
- Definable functions
- Basic globbing
- Support for various editing interfaces
- Array-based with Cartesian products
- Variable expansion

For more information, see [2].

### dgsh

Put simply, dgsh is bash with parallelism. Vanilla bash permits parallelism by
running several processes in the background. However, dgsh permits building
parallel pipelines. Dgsh accomplishes this by slightly extended bash's syntax
and by providing parallel aware versions of common Unix utilities (such as tee,
for example). Dgsh models processes as a DAG, executes processes which have no
interdependencies in parallel, and performs aggregation as necessary.

My prime interest with dgsh is its idea of multipipes: redirecting the
input/output of a process from/to *multiple* processes. This naturally requires
some sort of aggregation in order to print a final result to a normal or to run
a command which expects only a single input.

While I'm not doing dgsh justice, I define the non-exhaustive feature set of
interest as follows.

- Features of bash
- Multipipes

For more information, see [3].

### fish

Fish, like rc, is based upon a design philosophy. This, by definition, makes
fish far superior to the Bourne shell. (The Bourne shell's syntax seems to be
inspired by a cat running across Bourne's keyboard.)  The fish design philosophy
presents an important point [4]: "Configurability is the root of all evil." That
is, by providing too many configuration options, a piece of software is more
difficult to write, more difficult to maintain, and more difficult for a user to
learn and fully comprehend. The highly popular and successful CMS WordPress has
a similar design philosophy. Admittedly, I find the philosophy intriguing and on
point. While fish has an enormous feature set in comparison to rc, it is still
manageable. This is largely due to its extensive documentation and consistent
design. 

To begin, fish has typical arbitrary file redirection but with a dedicated
syntax for stderr. Additionally, fish supports non-clobbering file redirection
which will not overwrite an existing file. Pipelines and file redirection can be
combined in interesting ways, and fish's design philosophy ensures that these
features work together.

Where possible, fish tries to avoid introducing new syntax in favor of using
built-in commands. For instance, a function's body is defined to start with the
<code>function</code> command and end with the <code>end</code> command. This is
contrary to e.g. bash which introduces brace syntax to delimit a function's
body. This consistency keeps fish's syntax small, and thus fish is easier to
learn. Speaking of functions, fish functions can be auto-loaded. That is, if
fish encounters the use of an undefined function, it will search a predefined
list of directories to see if that function is defined somewhere. Also, fish
supports event handlers which are functions that get called when a specified
event is triggered (such as the receipt of a signal). Wow! [5]

Fish implements an extensive command-line editor. For starters, fish supports
Emacs- and Vi-like editing interfaces with programmable keybindings. There is
tab completion, as expected, and multi-line editing capabilities. However,
perhaps the most intriguing feature of fish's editing capabilities is its
support for suggestions. Fish is the only shell which will parse all of the man
pages on the running computer and use discovered information to provide detailed
suggestions. Fish will suggest not only commands but also arguments to known
programs. All of this is programmable, of course.

One extremely interesting feature of fish is its variable scoping. Fish supports
three levels of variable scopes: local, global, and universal. The semantics of
local and global are perhaps obvious: local variables are attributed to a
specific block such as an if statement while global variables are attributed to
an entire fish session. Universal variables are the most interesting; these
variables are shared among *all* running fish instances on the computer. Such
neat! [5] A variable's desired scope can be specified upon declaration.

The last feature to be discussed is fish's array support. Fish's arrays are
quite powerful and similar in spirit (perhaps inspired by?) rc's arrays. Fish
supports multi-dimensional arrays, generating sub-arrays based on a range of
indices, and Cartesian products, whereby combinations of two arrays are
generated. Many dimensions, much Sagan! [5]

As with the other shells discussed, here is a non-exhaustive list of interesting
fish features.

- Arbitrary file redirection (with non-clobbering option)
- Pipelines
- Foreground/background processes with basic job control
- Definable functions (auto-loadable)
- Extensive autosuggestion engine based on parsed man pages (among other things)
- Tab completions (user-definable)
- Basic globbing
- Command substitution
- Powerful arrays with Cartesian products
- Variable expansion and other expansions
- Universal variables
- Extensive editing capabilities (multiline) with user-definable key bindings
- Syntax highlighting
- Event handlers
- Built-in debugger

For more information, see [6].

## References

- [1] <http://static.tobold.org/rc/rc-duff.html>
- [2] <http://tobold.org/article/rc>
- [3] <https://www.spinellis.gr/sw/dgsh/>
- [4] <https://fishshell.com/docs/current/design.html>
- [5] <https://en.wikipedia.org/wiki/Doge_(meme)>
- [6] <http://fishshell.com/>
