
How to write a useful commit message
------------------------------------

1. Add a prefix to the subject line that gives the area of code that is changed.
   Usually this will be the relative file path of the file being changed. (If
   the change is to the pbio library, omit the `lib/pbio/` part of the path.)

2. The rest of the subject line describes *what* changed.

3. The body describes *why* the change was made.

4. In included other relevant information in the commit message if applicable.
   For example, if the commit fixes a compiler error, include a copy of the
   error message.

5. Include a link to the relevant GitHub issue, if applicable.

6. Include the change in firmware size, if applicable.


Coding style
------------

We follow the MicroPython coding style.

https://github.com/micropython/micropython/blob/master/CODECONVENTIONS.md
