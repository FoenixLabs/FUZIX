BASE_DIR = ../platform-a2560-base

all:
	$(MAKE) -C $(BASE_DIR) \
		EXTRA_CFLAGS="-I$(CURDIR)"

clean:
	$(MAKE) -C $(BASE_DIR) clean

image:
	$(MAKE) -C $(BASE_DIR) image

diskimage:
	$(MAKE) -C $(BASE_DIR) diskimage