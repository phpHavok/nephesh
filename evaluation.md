# An evaluation of exotic shells

Following is a brief evaluation of a few shells that I found to be interesting
in one way or another. These shells have contributed to the inspiration and
design of nephesh. For the purposes of this document, "exotic" is defined
subjectively to mean "any shell that I found interesting enough to write about."
This document may grow and morph over time.

## rc

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

* Arbitrary file redirection
* Command substitution
* Pipelines (with branching)
* Foreground/background processes with basic job control
* Command grouping
* Control structures (e.g., if, for, while, etc.)
* Definable functions
* Basic globbing
* Support for various editing interfaces
* Array-based with Cartesian products
* Variable expansion

For more information, see [2].

## dgsh

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

* Features of bash
* Multipipes

For more information, see [3].

## fish

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

* Arbitrary file redirection (with non-clobbering option)
* Pipelines
* Foreground/background processes with basic job control
* Definable functions (auto-loadable)
* Extensive autosuggestion engine based on parsed man pages (among other things)
* Tab completions (user-definable)
* Basic globbing
* Command substitution
* Powerful arrays with Cartesian products
* Variable expansion and other expansions
* Universal variables
* Extensive editing capabilities (multiline) with user-definable key bindings
* Syntax highlighting
* Event handlers
* Built-in debugger

For more information, see [6].

# References

* [1] (http://static.tobold.org/rc/rc-duff.html)
* [2] (http://tobold.org/article/rc)
* [3] (https://www.spinellis.gr/sw/dgsh/)
* [4] (https://fishshell.com/docs/current/design.html)
* [5] (https://en.wikipedia.org/wiki/Doge_(meme))
* [6] (http://fishshell.com/)
