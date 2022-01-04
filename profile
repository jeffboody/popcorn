# Setup Vulkan for Linux SDL
if [[ "$OSTYPE" != "darwin"* ]]; then
	source ${HOME}/vulkan/1.2.170.0/setup-env.sh
fi

#-- DON'T CHANGE BELOW LINE --

export TOP=`pwd`
alias croot='cd $TOP'
