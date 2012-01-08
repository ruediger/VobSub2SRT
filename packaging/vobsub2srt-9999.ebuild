# -*- mode:sh; -*-
# Copyright 1999-2012 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /media-video/vobsub2srt/ChangeLog,v 0.1 2012/01/06 19:15:04 thawn Exp $

EAPI="4"

EGIT_REPO_URI="git://github.com/ruediger/VobSub2SRT.git"

inherit git-2

IUSE=""

DESCRIPTION="Converts image subtitles created by VobSub (.sub/.idx) to .srt textual subtitles using tesseract OCR engine"
HOMEPAGE="https://github.com/ruediger/VobSub2SRT"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~x86"

RDEPEND=">=app-text/tesseract-2.04-r1
    >=virtual/ffmpeg-0.6.90"
DEPEND="${RDEPEND}"
src_configure() {
    econf
}
src_compile() {
    emake || die
}

src_install() {
    emake DESTDIR="${D}" install || die
}
