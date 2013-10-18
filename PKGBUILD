pkgname="gedit-textwrap-plugin"
pkgver=0.2.1
pkgrel=0
pkgdesc="Toggle Text Wrap Setting by Menu Entry or Toolbar Button"
url="http://hartmann-it-design.de/gedit/TextWrap/"
license=("GPL3")
arch=('any')
depends=('gedit>=3' 'pygtk')
source=("TextWrap.py"
        "TextWrap.plugin")

build() {
    cd $srcdir
    install -d $pkgdir/usr/lib/gedit/plugins
    install -m644 TextWrap.plugin TextWrap.py $pkgdir/usr/lib/gedit/plugins
}

md5sums=('c190755749c7397864ab86bc6f788a18'
         '767e2e8eed17a6d79f13813c5d419c66')
