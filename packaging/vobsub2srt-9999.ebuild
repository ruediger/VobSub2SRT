# -*- mode:sh; -*-

# See https://github.com/ruediger/VobSub2SRT/issues/13

# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

EAPI="4"

EGIT_REPO_URI="git://github.com/ruediger/VobSub2SRT.git"

inherit cmake-utils git-2

IUSE=""

DESCRIPTION="Converts image subtitles created by VobSub (.sub/.idx) to .srt textual subtitles using tesseract OCR engine"
HOMEPAGE="https://github.com/ruediger/VobSub2SRT"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~x86"

RDEPEND=">=app-text/tesseract-2.04-r1
    >=virtual/ffmpeg-0.6.90"
DEPEND="${RDEPEND}"
