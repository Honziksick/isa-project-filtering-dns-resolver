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
SHELL = /bin/sh

# Makefile self-reference
SELF := Makefile

# Flags to suppress 'Entering/Leaving directory' messages
MAKEFLAGS += --no-print-directory

# Wrapper to choose the appropriate Makefile based on the OS
.DEFAULT:
	@os=`uname -s`; \
	if [ "$$os" = "FreeBSD" ]; then \
		exec gmake -f Makefile.bsd --no-print-directory "$@"; \
	else \
		exec $(MAKE) -f Makefile.gnu --no-print-directory "$@"; \
	fi

### end of file Makefile ###
