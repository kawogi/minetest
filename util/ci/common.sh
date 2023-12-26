#!/bin/bash -e

# Linux build only
install_linux_deps() {
	local pkgs=(
		cmake gettext
		libpng-dev libjpeg-dev libxi-dev libgl1-mesa-dev
		libsqlite3-dev libogg-dev libgmp-dev libvorbis-dev
		libopenal-dev libpq-dev libcurl4-openssl-dev libzstd-dev
	)

	if [[ "$1" == "--no-irr" ]]; then
		shift
	else
		local ver=$(cat misc/irrlichtmt_tag.txt)
		wget "https://github.com/minetest/irrlicht/releases/download/$ver/ubuntu-bionic.tar.gz"
		sudo tar -xaf ubuntu-bionic.tar.gz -C /usr/local
	fi

	sudo apt-get update
	sudo apt-get install -y --no-install-recommends "${pkgs[@]}" "$@"
}
