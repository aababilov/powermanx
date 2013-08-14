# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI="5"

EGIT_REPO_URI="git://github.com/aababilov/powermanx.git"

inherit autotools systemd git-2

DESCRIPTION="Extensible power manager"
HOMEPAGE="http://github.com/aababilov/powermanx"

LICENSE="GPL-3+"
SLOT="0"
KEYWORDS="~amd64 ~x86"

RDEPEND=""
DEPEND="${RDEPEND}
	>=dev-libs/glib-2.24
	>=x11-libs/gtk+-3.1:3
	>=sys-apps/dbus-1.5
	>=dev-libs/dbus-glib-0.70
	virtual/udev[gudev]
	>=sys-power/upower-0.9
	x11-misc/lightdm
	>=x11-libs/libnotify-0.7
	x11-libs/libXScrnSaver
	dev-libs/jsoncpp
"

src_prepare() {
	if [[ ! -e configure ]]; then
		eautoreconf
	else
		elibtoolize
	fi
}

src_configure() {
	econf \
		--localstatedir=/var \
		"$(systemd_with_unitdir)"
}

src_install() {
	emake DESTDIR="${D}" install
}
