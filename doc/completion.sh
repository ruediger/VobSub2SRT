#!/bin/bash
# -*- mode:sh; coding:utf-8; -*-
#
#  Bash completions for vobsub2srt(1).
#
#  Copyright (C) 2010 Rüdiger Sonderfeld <ruediger@c-plusplus.de>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

have vobsub2srt &&
_vobsub2srt() {
    local cmd cur prev tmp arg
    cmd=${COMP_WORDS[0]}
    _get_comp_words_by_ref cur prev

    case $prev in
        --ifo)
            _filedir '(ifo|IFO)'
            return 0
            ;;
        --lang|-l)
            _get_first_arg 
            tmp=$( "$cmd" --langlist -- "$arg" 2>/dev/null | sed -E -e '/Languages:/d; s/^[[:digit:]]+: //;' )
            COMPREPLY=( $( compgen -W "$tmp" -- "$cur" ) )
            return 0
            ;;
        --tesseract-data)
            _filedir -d
            return 0
            ;;
    esac

    case $cur in
        -*)
            COMPREPLY=( $( compgen -W '--dump-images --dump-error-images --verbose --ifo --lang --langlist --tesseract-lang --tesseract-data --blacklist' -- "$cur" ) )
            ;;
        *)
            _filedir '(idx|IDX|sub|SUB)'
            COMPREPLY=$( echo "$COMPREPLY" | sed -E -e 's/.(idx|IDX|sub|SUB)$//' ) # remove suffix
            ;;
    esac

    return 0
} &&
complete -F _vobsub2srt vobsub2srt
