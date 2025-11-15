################################################################################
#                                                                              #
# Project:      Filtering DNS Resolver                                         #
# University:   Faculty of Information Technology, BUT                         #
# Subject:      ISA: Network Applications and Network Administration           #
#                                                                              #
# File:         Makefile                                                       #
# Author:       Jan Kalina <xkalinj00>                                         #
#                                                                              #
# Created:      15.11.2025                                                     #
# Last edit:    15.11.2025                                                     #
#                                                                              #
# Description: Portable dispatcher Makefile that selects the proper platform-  #
#              specific build script. On FreeBSD (server 'Eva') it forwards    #
#              all targets to 'Makefile.bsd' via 'gmake'. On GNU/Linux it uses #
#              'Makefile.gnu'. Provides a minimal, buildable and runnable      #
#              setup for the Filtering DNS Resolver across both environments.  #
#                                                                              #
################################################################################

################################################################################
#                                                                              #
#                 BASIC SETTINGS AND DEFINITIONS FOR MAKEFILE                  #
#                                                                              #
################################################################################

###                                   ###
#  Basic configuration of the Makefile  #
###                                   ###

# Default shell set to 'shell'
SHELL := /bin/sh

# Default goal is build all
.DEFAULT_GOAL := all

# Flags to suppress 'Entering/Leaving directory' messages
MAKEFLAGS += --no-print-directory

# Disable implicit suffix rules for speed and predictability
.SUFFIXES:

# Define dispatch targets
DISPATCH_TARGETS := all build run test help clean doc pack \
                    clean-all clean-build clean-exec clean-test clean-doc clean-pack \
                    run-test test-venv activate-venv deactivate-venv clean-venv \
                    pack-prepare \
                    developer-mode submission-mode install-dev-dep install-help-dep install-build-dep install-doc-dep install-pack-dep update-dep

# Define phony targets
.PHONY: $(DISPATCH_TARGETS)

# Dispatcher: forward every target to the platform-specific Makefile
$(DISPATCH_TARGETS):
	@os=`uname -s`; \
	if [ "$$os" = "FreeBSD" ]; then \
		exec gmake -f Makefile.bsd --no-print-directory "$@"; \
	else \
		exec $(MAKE) -f Makefile.gnu --no-print-directory "$@"; \
	fi

### end of file Makefile ###
