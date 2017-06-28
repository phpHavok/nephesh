# An evaluation of shells and their features

*Jacob Chappell*  
*chappellind 0x40 gmail 0x2E com*

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
abstract sense and syntactic considerations. For exsh, we will oftentimes assume
an arbitrary choice of syntax for the sake of example. This arbitrary choice of
syntax will be uncannily bash-like, for no other reason than to accommodate the
majority of readers who are coming from a bash background (myself included).

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

### Pipelines

The Unix philosophy includes a statement that can be roughly paraphrased as: a
program should do one job, do only one job, and do that job well. Consequently,
many Unix programs are designed with a specific task in mind. They expect their
inputs to include the necessary data to perform only their task, and they expect
their outputs to be used as inputs to other programs if further processing is
necessary. For example, one may wish to list the files in a directory in
lexicographical order which match a certain filter. In Unix, there are three
programs which can together accomplish this task. `ls` lists the files in a
directory, `grep` filters the list, and `sort` sorts the list (lexicographically
or otherwise). Currently, accomplishing this task in exsh is possible but
inconvenient.

```
exsh> ls > /tmp/ls.out
exsh> grep filter < /tmp/ls.out > /tmp/grep.out
exsh> sort < /tmp/grep.out
```

Notice how the `grep` command is really just a bridge between the `ls` and
`sort` commands. This pattern is so common and paramount in Unix, that it
deserves special consideration by shells. A shell which supports attaching the
standard output of one program directly to the standard input of a second
program is said to support a feature we'll call **pipelines**. As with file
redirection, we'll say that a shell supports **arbitrary pipelines** if it
permits establishing pipes between streams other than standard output and
standard input.

In most shells, the `|` metacharacter is used for creating pipelines. It is a
fitting symbol given that it looks like a vertical pipe. Given a dedicated
syntax for pipelines, the previous example would be much simpler and appear
cleaner in exsh.

```
exsh> ls | grep filter | sort
```

Astute observation may, at this point, provoke thought on the similarities and
differences between pipelines and file redirection. Both features revolve around
redirecting file streams identified by file descriptors. Furthermore, the two
features are often syntactically similar.

```
exsh> cat < file.in | sort > file.out
```

In the previous example, `file.in` is sorted and written to `file.out`. By shell
convention, the command is listed first rather than the file. However, if we are
to rearrange the example slightly, a striking resemblance emerges.

```
exsh> file.in > cat | sort > file.out
```

Now, it is perhaps more clear just how similar pipelines and file redirection
are. Syntactically, the file redirection symbols indicate to the parser that the
thing being redirected to or from should be treated as a file and *not* executed
as a program. In the previous example, we were able to deprecate the `<`
metacharacter in favor of moving the file to the left of the command. This
creates a parsing ambiguity which can be resolved by realizing that the thing on
the left of the first `>` symbol must be a file while the thing on the right of
the last `>` symbol must be a file. The reader is encouraged to pause and think
about why this must be the case.

By introducing a separate syntax indicating whether or not a file should be
executed as a command or not, a shell is able to provide a cleaner, uniform
pipeline syntax. As an example, consider prefixing files which should *not* be
executed with the `@` metacharacter.

```
exsh> @file.in | cat | sort | @file.out
```

Because a pipeline is a serial structure, one broken link effectively breaks the
whole chain. Thus, all programs in a pipeline should be treated somehow as a
unit. For example, if any of the programs in a pipeline are suspended (i.e.,
with SIGSTOP), then *all* of the programs in that pipeline should be suspended.
Fortunately, because Unix was designed with pipelines in mind, Unix provides a
mechanism for easily accomplishing this: process groups. By creating a process
group for each pipeline and assigning each program of the pipeline to that
process group, one can effectively signal all of the associated programs as a
unit.

### Command substitution

Occasionally, it is useful to use the output of a command or the contents of a
file as an argument to a command. For example, consider trying to create a
directory named after the current date in YYYY-MM-DD format. As it stands, this
is not possible in exsh as a single command. To accomplish this, one would have
to run the `date` command, copy the output, then run the `mkdir` command and
paste the output of the date command.

```
exsh> date +%F
2017-06-27
exsh> mkdir 2017-06-27
```

Many shells allow one to accomplish this same task more automatically with a
feature we'll call **command substitution**. In a programming language, one can
effectively replace a variable's identifier with the contents of the variable.
Similarly, command substitution effectively substitutes the output of a command
in place of an argument to another command. Shells vary in the syntax they use
to achieve command substitution. For example, bash uses `` `command` `` while
fish uses `(command)`. Assuming bash's syntax, the previous example would be
much simpler.

```
exsh> mkdir `date +%F`
```

It is worth noting that supporting nested command substitutions requires
syntactic care (or significant parsing overhead). Later in this document, we
will discuss how rc manages to cleverly avoid this problem.

### Job control

At this point in our development, exsh is said to execute all commands and
pipelines in a foreground process group. A foreground process group in Unix is
connected to a controlling terminal, which is usually the controlling terminal
of the shell which launched the commands running in the foreground process
group. Consequently, we can only run one pipeline at a time in a single shell
session, because each pipeline will lock up the terminal until it completes.
This is sometimes undesirable. For example, we may wish to search the entire
file system for some string, which may take a long time. We'd like to be able
to continue using our shell for other tasks while the search is ongoing. This
can be accomplished in shells which support a feature we'll call **job
control**.

With job control, a shell allows a pipeline (or job) to be pushed into the
background.  While in the background, a process group can continue executing,
but it will no longer be attached to the controlling terminal. If the process
group attempts to read from the terminal, it will block until it is restored to
the foreground. The shell can provide a set of commands which allow moving
processes to and from the background. In fact, multiple processes can be
executing in the background simultaneously, but only one process group can be
executing in the foreground at any given time. Shells with job control
typically associate some kind of unique identifiers with jobs so that they can
be more easily referenced by the user.

### Globbing

Oftentimes, users will need to run a command on a set of related files. For
example, consider the task of archiving a bunch of PNG images.

```
exsh> tar -cf images.tar img1.png img2.png img3.png img4.png img5.png
```

For a small number of files, this isn't much of a problem. However, imaging
trying to archive hundreds or thousands of files. Put simply, manually typing
each filename doesn't scale. Many shells support methods of expanding a
filename *pattern* into separate arguments. This feature is typically known as
**globbing** or parameter expansion.

Shells vary in the degree to which they implement globbing. In the simplest
case, the most vital globbing pattern is the "put anything here" pattern, which
is typically represented with a `*` metacharacter. As with any metacharacter,
passing a literal `*` as an argument will require escaping it. With just this
pattern, the previous example becomes feasible.

```
exsh> tar -cf *.png
```

Essentially, globbing is a form of (typically a very small subset of) regular
expressions. A shell designer is free to implement globbing to any extent, but
the previous example illustrates the most important pattern which is arguably
vital to any decent shell.

### Input modes

Sometimes, the metacharacters of a shell's language can get in the way. As the
language grows in size, it can become difficult for users to remember all of the
metacharacters. This is motivation to keep the language as small as possible,
but most shells also support some concept of **input modes** to ease the burden
on users. For example, the space character is a metacharacter in exsh, but a
user may wish to provide a single argument to a command which contains a lot of
spaces. Presently, that would be annoying.

```
exsh> grep -r a\ \*really\*\ 'unique'\ string /
```

Not only is this very difficult to parse for a human reader, but it is
error-prone. Input modes allow a user to toggle on or off different
metacharacters in the language. Most shells typically provide at least a second
input mode, different from the default mode, which disables *all* metacharacters
except the character which toggles the mode. This toggling metacharacter is
typically the single quote `'`. Using this mode, the previous example is much
cleaner.

```
exsh> grep -r 'a *really* \'unique\' string' /
```

Notice that the *only* character that needs to be escaped to obtain its literal
value is the `'` character used for toggling the mode. This is one (albeit a
very important) example of input modes, but several others can exist. For
example, command substitution can be thought of us an input mode. Thus,
appending a string to the result of a command substitution might be allowed.

```
exsh> ping `hostname -s`.local
```

### Command line editing

Perhaps the most important feature of interactive shells is a doozy: **command
line editing**. This feature really encompasses a whole class of ideas. Command
line editing is all about providing efficient ways for users to input, edit, and
recall commands. For example, a user types out a long command and then realizes
that he or she needs to edit something at the *beginning* of the command. A
shell might provide a quick keyboard shortcut for jumping to the beginning of
the command.

One paramount feature of command line editing, without which a shell is doomed
to fail, is tab completion. Tab completion allows a user to type part of a
command or filename, press tab, and have the remainder of the command or
filename automatically completed for them. Shell users are so dependant on this
feature, that a modern shell simply cannot dismiss this and expect to succeed.
Shells vary in the extent to which they implement tab completion. For example,
some shells have programmable tab completion which can be tuned to a power
user's delicate needs.

Another feature of command line editing that users depend heavily upon is
history. Users like to be able to quickly recall previous commands in order to
execute them again, possibly with minor edits. Shells typically allow users to
press the up arrow key in order to cycle through previously executed commands.
Furthermore, some shells provide special syntax for performing convenient
command substitution on previously executed commands. The extent to which
history is implemented is up to the shell designer, of course.

We have only discussed a couple of editing features, but the sky's the limit
here. Modern shells can benefit heavily from innovation in the area of command
line editing. While shells can choose to manually implement all of these
features, libraries like GNU Readline exist for bootstrapping a lot of what we
have discussed here. More daring shell developers can investigate ncurses or
direct ANSI escape sequences.

### Considerations for scripting

Thus far, we have been primarily focused on designing a shell for interactive
use. However, shells have long been used for batch processing as well. For batch
processing, it is necessary to be able to execute the shell with a file
containing a series of commands to execute. We refer to writing a file
containing shell commands as scripting.

Much of our existing work should trivially apply to scripting, but some
features, such as job control, have no relevance in scripting. Furthermore, a
shell which supports scripting typically needs some additional features. While
not an exhaustive list, we will briefly discuss a couple of shell features that
are important for scripting.

First, we discuss variables. Variables provide a way for the user to store a
string, array, the output of a command, or something else for later use. If
variables are implemented, there must be a way for variables to be substituted
for their contained values. This is similar to command substitution. Whether
variables are string-valued, array-valued, or otherwise is a shell design
decision that should be carefully considered. Shells which support arrays have
other considerations, such as how to support array indexing. While variables can
be useful for interactive use, they are typically vital for scripting.

Next on the list, functions. Functions allow grouping a set of commands which
tend to be repeated multiple times throughout a script. Typically, functions
allow altering their run based upon parameters (just as commands have
parameters). There is not a huge difference between commands and functions
except that functions are much more convenient for users to write.

Last, we mention flow control. Flow control provides a mechanism to repeat
commands and conditionally execute or skip commands. Typical flow control
constructs are `if`, `while`, `for`, and `switch`, which perform similar tasks
as they do in typical programming languages. Most flow control is not very
important for interactive use but vitally important for scripting.

### Conclusion

At this point, the reader should have a solid idea about how shells are designed
and the decisions that shell designers are faced with. I'd like to formally note
that we have only begun to scratch the surface here. A shell which implements
the discussed features is useable and useful; however, a modern shell should
implement interesting and useful features well beyond the accepted minimum
discussed in this document. That being said, don't be afraid to jump in and
start playing with shell design. Designing a shell is incredibly fun, exciting,
challenging, and rewarding. So go out, conquer, and stop putting Bourne shell
derivatives on a pedestal.

## Interesting features of exotic shells

In the previous section, we established significant context as to the design of
shells and their features. Now, we will investigate a few of the more
interesting features implemented by some exotic shells. For the purposes of this
document, "exotic" is defined subjectively based upon my personal interests.

### rc

Rc, written by Tom Duff, is the flagship shell of the Plan 9 operating system.
While roughly backwards compatible with the Bourne shell in simplistic cases, rc
features a cleaner design, a formal grammar, and some nice features. The effort
to include a level of backwards compatibility with the Bourne shell did, in some
sense, inhibit the innovation of rc. As Duff writes [1], "Any successor of the
Bourne shell is bound to suffer in comparison."

Duff designed rc under the realization that, in reality, shells mostly deal with
arrays. For example, the most common shell action is to execute a command with
an *array* of arguments. As such, variables in rc are array-valued rather than
string-valued. More precisely, variables in rc are arrays of strings. A scalar
string is thus represented as an array with one element. Undefined variables are
equivalent to the empty array, which is different from the array containing the
empty string. Arrays in rc have some nice indexing properties.

```
rc> arr=(a b c 1 2 3)
rc> echo $arr(1 1 1 4 3 2)
a a a 1 c b
```

Another cool feature of arrays in rc is Cartesian products, whereby two arrays
are pair-wise combined.

```
rc> echo (a b c)^(1 2 3)
a1 b2 c3
```

If one array is a singleton, the result is what you might expect, which is very
useful for generating a list of source files for example.

```
rc> echo (file1 file2 file3)^(.c)
file1.c file2.c file3.c
```

Duff was motivated to implement rc's parser in one pass. This is something that
bash failed to achieve due to its odd treatment of arrays and support for nested
command substitution. Duff circumvented the first problem by designing variables
as array-valued from the beginning, rather than as something secondary. For the
second problem, duff cleverly tweaked bash's `` ` `` pair into a unary prefix
operator. Thus, balancing backticks was a problem no more.

```
rc> p1 `{p2 arg arg `{p3 arg} arg} arg
```

Another interesting feature of rc is that it's somewhat agnostic about the
interface you use for command line editing.  For example, you can link rc
against GNU readline in order to use many of the editing features bash provides
(which also uses GNU readline). Other options are also available. It is clever
that Duff was able to abstract the editing interface like this, although it does
limit the innovation somewhat.

Rc supports many of the features discussed in the first section plus more. For
more information, see [2].

### dgsh

Put simply, dgsh is bash with parallelism. Vanilla bash permits parallelism by
running several processes in the background. However, dgsh permits building
parallel pipelines. Dgsh accomplishes this by slightly extended bash's syntax
and by providing parallel aware versions of common Unix utilities (such as tee,
for example). Dgsh models processes as a DAG, executes processes which have no
interdependencies in parallel, and performs aggregation as necessary.

The primary unique feature of dgsh is **multipipes**. The pipelines we've
discussed thus far are always linear: one program feeds output into another
program which feeds output into another program. However, multipipes extends the
structure of pipelines to a tree. That is, a program can redirect its output to
one *or more* programs, and so on. Because the ultimate output of this pipeline
tree is likely the controlling terminal, some method must exist for aggregating
several outputs into a single stream. Dgsh provides facilities for accomplishing
this.

Dgsh is an interesting shell with an official research paper. I haven't done it
justice here, but see [3] for more information.

### fish

Fish, like rc, is based upon a design philosophy. This, by definition, makes
fish far superior to the Bourne shell. (The Bourne shell's syntax seems to be
inspired by a cat running across Bourne's keyboard.) Fish's design philosophy
(see [4]) can be roughly summarized by the following points.

- Keep the language as small as possible, and avoid implement redundant features
that overlap with existing features.
- Don't crash and disappoint the user.
- Configurability is the root of all evil.
- Design the user interface first.
- Make it easy to discover shell features.

One point of particular interest is "Configurability is the root of all evil."
That is, by providing too many configuration options, a piece of software is
more difficult to write, more difficult to maintain, and more difficult for a
user to learn and fully comprehend. The highly popular and successful CMS
WordPress has a similar design philosophy. Admittedly, I find the philosophy
intriguing and on point. While fish has an enormous feature set in comparison to
rc, it is still manageable. This is largely due to its extensive documentation
and consistent design. 

Where possible, fish tries to avoid introducing new syntax in favor of using
built-in commands. For instance, a function's body is defined to start with the
`function` command and end with the `end` command. This is contrary to e.g. bash
which introduces brace syntax to delimit a function's body. This consistency
keeps fish's syntax small, and thus fish is easier to learn. Speaking of
functions, fish functions can be auto-loaded. That is, if fish encounters the
use of an undefined function, it will search a predefined list of directories to
see if that function is defined somewhere. Note that this does not violate the
third design point, as functions are no more configuring the shell than running
a new and arbitrary command from outside the shell.

Fish also supports event handlers, which are functions that get called when a
specified event is triggered. In this sense, user functions can be callback
functions. The user can specify upon function definition if the function should
be automatically called as the result of an event. As indicated in fish's
documentation, the supported events which can be caught follow.

- Receipt of a Unix signal (like SIGTERM).
- Termination of a process or job.
- Update of a variable's value.
- Before the prompt is shown.
- When command lookup fails.

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
variables are shared among *all* running fish instances on the computer. A
variable's desired scope can be specified upon declaration.

Fish's arrays are similar in spirit (perhaps inspired by?) rc's arrays. I say
this, because fish also supports Cartesian products, which is one of rc's
interesting features. Furthermore, fish's arrays can be multidimensional, just
like a full-fledged programming language. This can be useful for nested for
loops, for example.

Finally, fish supports a most interesting feature: built-in debugging. When
executing a script (non-interactively), a user can stop the script at an
arbitrary point and drop into an interactive debugging session. In the debugging
session, the user is able to inspect the contents of variables and do other
actions typical of a debugger. This is accomplished by adding the `breakpoint`
command to the script at the point which the user wishes to break. It is also
possible to stop a script at an arbitrary point by sending it the TRAP signal.
Truly, this is a delightful feature for a shell.

Fish is perhaps the most interesting, user friendly, and modern shell discussed
here. The developer has clearly invested a lot of time and effort (and humor) in
the design and maintenance of the shell. For more information, see [5].

## References

- [1] <http://static.tobold.org/rc/rc-duff.html>
- [2] <http://tobold.org/article/rc>
- [3] <https://www.spinellis.gr/sw/dgsh/>
- [4] <https://fishshell.com/docs/current/design.html>
- [5] <http://fishshell.com/>
