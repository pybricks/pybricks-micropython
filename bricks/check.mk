
# NB: relative paths are making the assumption that this file get included
# from bricks/*/Makefile

# This is a test to ensure that the parent micropython git tree is checked
# out to the correct commit.
ifeq ($(shell git rev-parse --is-inside-work-tree 2>/dev/null),true)
HEAD = $(shell git --git-dir=../../../../.git rev-parse HEAD)
TAG = $(shell cat ../../micropython-tag)
HASH = $(shell git --git-dir=../../../../.git rev-parse $(TAG))
ifneq ($(HEAD),$(HASH))
$(error "Unexpected micropython git hash. Update micropython-tag or checkout correct commit.")
endif
endif
