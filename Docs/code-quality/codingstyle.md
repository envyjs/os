<h1>Mode line</h1>

Files should have Emacs and vim mode line comments as the first two
lines of the file, which should set ``indent-tabs-mode`` to ``nil``. For new
files, use the following, specifying two-space indentation:

<code>
   /* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
   /* vim: set ts=2 et sw=2 tw=80: */
</code>

Be sure to use the correct ``Mode`` in the first line, don't use ``C++`` in
JavaScript files.