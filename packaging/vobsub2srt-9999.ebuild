# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

EAPI=6

DESCRIPTION="Converts image subtitles created by VobSub (.sub/.idx) to .srt textual subtitles using tesseract OCR engine"
HOMEPAGE="https://github.com/ruediger/VobSub2SRT"
SRC_URI="https://github.com/ruediger/VobSub2SRT/archive/master.tar.gz ->
${P}.tar.gz"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

RDEPEND="
	>=app-text/tesseract-2.04-r1
	>=virtual/ffmpeg-0.6.90
"

DEPEND="${RDEPEND}"

src_unpack() {
	unpack ${A}
	mv VobSub2SRT-master "${S}" || die "unpack failed"
}
