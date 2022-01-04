About
=====

Popcorn is a mini flight simulator demo.

The cockpit model was created by Dale Boody.

Send questions or comments to Jeff Boody at jeffboody@gmail.com

Controls
========

Popcorn requires an XBox style controller.

	Aileron/Roll:   left stick X axis
	Elevator/Pitch: left stick Y axis
	Rudder/Yaw:     left/right triggers
	Head:           right stick
	Thrust:         B button
	Brake:          A button
	Reset:          X button

Screenshots
===========

Here is a screenshot of the Popcorn Flight Simulator.

![alt text](screenshot.jpg?raw=true "Popcorn Flight Simulator")

Setup
=====

Clone Project
-------------

Clone the popcorn project https://github.com/jeffboody/popcorn.

	git clone git@github.com:jeffboody/popcorn.git
	git checkout -b main origin/main
	git submodule update

Linux
-----

Install library dependencies

	sudo apt-get install libsdl2-2.0 libjpeg-dev

Install Vulkan libraries

	sudo apt-get install libvulkan1 vulkan-utils

Install the LunarG Vulkan SDK

	https://www.lunarg.com/vulkan-sdk/

Building
========

Edit profile as needed for your development environment
before building.

Linux
-----

Command line

	source profile.sdl
	cd app/src/main/cpp
	make
	./popcorn
