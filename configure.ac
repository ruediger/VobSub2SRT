AC_INIT([VobSub2SRT], 1.1)
AM_INIT_AUTOMAKE([subdir-objects foreign])
AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_CXX
PKG_CHECK_MODULES([Tesseract], [tesseract])

AC_ARG_WITH([bash-completion-dir],
  AS_HELP_STRING([--with-bash-completion-dir[=PATH]],
    [Install the bash auto-completion script in this directory. @<:@default=yes@:>@]),
  [],
  [with_bash_completion_dir=yes])

if test "x$with_bash_completion_dir" = "xyes"; then
  PKG_CHECK_VAR(bashcompdir, [bash-completion], [completionsdir], ,
    bashcompdir="${sysconfdir}/bash_completion.d")
else
  bashcompdir="$with_bash_completion_dir"
fi

AC_SUBST([bashcompdir])
AM_CONDITIONAL(ENABLE_BASH_COMPLETION, [test "$with_bashcompletiondir" != "no"])

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
