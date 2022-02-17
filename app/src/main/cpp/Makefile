export CC_USE_MATH  = 1
export VKK_USE_VKUI = 1

TARGET   = popcorn
CLASSES  = popcorn_renderer popcorn_cockpit
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
CFLAGS   = $(OPT) -I. -I$(VULKAN_SDK)/include `sdl2-config --cflags` -DA3D_GL2
LDFLAGS  = -Llibvkk -lvkk -L$(VULKAN_SDK)/lib -lvulkan `sdl2-config --libs` -Llibgltf -lgltf -Ljsmn/wrapper -ljsmn -Llibpak -lpak -Llibxmlstream -lxmlstream -Ltexgz -ltexgz -Llibcc -lcc -Llibexpat/expat/lib -lexpat -lm -lpthread -lz -ljpeg
CCC      = gcc

all: $(TARGET) libcc libgltf jsmn libpak libxmlstream texgz libvkk libexpat

$(TARGET): $(OBJECTS) libcc libgltf jsmn libpak libxmlstream texgz libvkk libexpat
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: libcc libgltf jsmn libpak libxmlstream texgz libvkk libexpat

libcc:
	$(MAKE) -C libcc

libgltf:
	$(MAKE) -C libgltf

jsmn:
	$(MAKE) -C jsmn/wrapper

libpak:
	$(MAKE) -C libpak

libxmlstream:
	$(MAKE) -C libxmlstream

texgz:
	$(MAKE) -C texgz

libvkk:
	$(MAKE) -C libvkk

libexpat:
	$(MAKE) -C libexpat/expat/lib

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C libcc clean
	$(MAKE) -C libgltf clean
	$(MAKE) -C jsmn/wrapper clean
	$(MAKE) -C libpak clean
	$(MAKE) -C libxmlstream clean
	$(MAKE) -C texgz clean
	$(MAKE) -C libvkk clean
	$(MAKE) -C libexpat/expat/lib clean

$(OBJECTS): $(HFILES)
