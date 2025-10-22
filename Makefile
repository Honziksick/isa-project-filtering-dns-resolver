################################################################################
#                                                                              #
# Project:      Filtering DNS Resolver                                         #
# University:   Faculty of Information Technology, BUT                         #
# Subject:      ISA: Network Applications and Network Administration           #
#                                                                              #
# File:         Makefile                                                       #
# Author:       Jan Kalina <xkalinj00>                                         #
#                                                                              #
# Created:      23.09.2025                                                     #
# Last edit:    24.09.2025                                                     #
#                                                                              #
# Description:  This Makefile is used for compiling the project Filtering DNS  #
#               Resolver for the ISA course. Besides building, the Makefile    #
#               also serves to automate other tasks such as generating         #
#               documentation, cleaning project directories, packaging the     #
#               project for submission, etc. This Makefile is inspired by      #
#               Makefiles I created for previous projects at BUT FIT (e.g.,    #
#               for the IVS, IFJ, ICP and IPK courses).                        #
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

# Project name
PROJECT = dns

# Binaries
EXECUTABLE = $(PROJECT)

# Name of the ZIP archive for project submission
PACK_NAME = xkalinj00

# ANSI sequences for colors
COLOR_RESET = \033[0m
COLOR_RED = \033[0;31m
COLOR_GREEN = \033[0;32m
COLOR_BLUE = \033[0;36m
COLOR_YELLOW = \033[0;33m
COLOR_MAGENTA = \033[0;35m


###                                        ###
#  Switches for running the $(MAKE) command  #
###                                        ###

# Run 'make' in silent mode (without event output)
$(VERBOSE)SILENTOPT = -s

# Definition of a constant to disable selected targets (for submission)
#SUBMISSION_MODE ?= true


###                   ###
#  Definition of paths  #
###                   ###

# Path to the directory with source files for the compiler
SRC_DIR = src

# Directories for placing built files
BUILD_DIR = build
RELEASE_BUILD_DIR = $(BUILD_DIR)/release
DEBUG_BUILD_DIR = $(BUILD_DIR)/debug
TEST_BUILD_DIR = $(BUILD_DIR)/test

# Path to the directory with tests
TEST_DIR = test
TEST_BIN_DIR = $(TEST_DIR)/bin

# Directory for generating documentation
DOC_DIR = doc

# Directory with the prepared project for packaging
PACK_DIR = pack
ARCHIVE_DIR = $(PACK_DIR)/$(PACK_NAME)


################################################################################
#                                                                              #
#                                BUILD SETTINGS                                #
#                                                                              #
################################################################################

###           ###
#  Compilation  #
###           ###

# Compiler and flags
CXX = g++
CXX_STD =-std=c++20
WARNING_FLAGS = -Wall -Wextra -Werror -pedantic -Wshadow -Wconversion -Wsign-conversion -Wnull-dereference
SANITIZE_FLAGS = -fsanitize=address -fsanitize=undefined
THREAD_FLAGS = -pthread
RELEASE_FLAGS = -O2 -DNDEBUG
TEST_FLAGS = -O1 -g3 -DTESTING
DEBUG_FLAGS = -O0 -g3 -DDEBUG

# Build flags for different build types
CXXFLAGS_RELEASE = $(CXX_STD) $(RELEASE_FLAGS) $(THREAD_FLAGS)
CXXFLAGS_TEST = $(CXX_STD) $(TEST_FLAGS) $(WARNING_FLAGS) $(SANITIZE_FLAGS) $(THREAD_FLAGS)
CXXFLAGS_DEBUG = $(CXX_STD) $(DEBUG_FLAGS) $(WARNING_FLAGS) $(SANITIZE_FLAGS) $(THREAD_FLAGS)


###                  ###
#  Source & Libraries  #
###                  ###

INCLUDES = -I$(SRC_DIR)
ISA_LIB = lib$(EXECUTABLE).a
ISA_LIB_TEST = lib$(EXECUTABLE)-test.a
ISA_LIB_DEBUG = lib$(EXECUTABLE)-debug.a


###                     ###
#  Wildcards & Variables  #
###                     ###

# For 'libdns.a' library
LIB_SRCS := $(shell find $(SRC_DIR) -type f -name "*.cpp" | grep -v "$(SRC_DIR)/App/main.cpp")
LIB_OBJS_RELEASE := $(patsubst $(SRC_DIR)/%, $(RELEASE_BUILD_DIR)/%, $(LIB_SRCS:.cpp=.o))
LIB_OBJS_TEST := $(patsubst $(SRC_DIR)/%, $(TEST_BUILD_DIR)/%, $(LIB_SRCS:.cpp=.o))
LIB_OBJS_DEBUG := $(patsubst $(SRC_DIR)/%, $(DEBUG_BUILD_DIR)/%, $(LIB_SRCS:.cpp=.o))

# For 'dns' executable
MAIN_SRC = $(SRC_DIR)/App/main.cpp
MAIN_OBJ_RELEASE = $(patsubst $(SRC_DIR)/%, $(RELEASE_BUILD_DIR)/%, $(MAIN_SRC:.cpp=.o))
MAIN_OBJ_TEST = $(patsubst $(SRC_DIR)/%, $(TEST_BUILD_DIR)/%, $(MAIN_SRC:.cpp=.o))
MAIN_OBJ_DEBUG = $(patsubst $(SRC_DIR)/%, $(DEBUG_BUILD_DIR)/%, $(MAIN_SRC:.cpp=.o))


################################################################################
#                                                                              #
#                                MAIN COMMANDS                                 #
#                                                                              #
################################################################################

# The '.PHONY' command indicates that the following commands are never considered as files
.PHONY: all build run test help clean doc pack \
	clean-all clean-build clean-exec clean-test clean-doc clean-pack \
	run-test, test-venv, activate-venv, deactivate-venv, clean-venv \
	pack-prepare \
	developer-mode submission-mode install-dev-dep install-help-dep install-build-dep install-doc-dep install-pack-dep install-test-dep update-dep

### MC # all: # Builds the 'dns' app
all: build

### MC # build: # Builds the 'dns' via CMake in developer version and Make in submission version (different versions)
ifndef SUBMISSION_MODE
build:
	@cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	@cmake --build build --config Release --target dns
else
build: $(EXECUTABLE)
endif

### MC # run: # Runs the executable with print help argument
run:
	@if [ ! -f "$(EXECUTABLE)" ]; then \
		$(MAKE) build; \
	fi
	./$(EXECUTABLE) -h

### MC # test: # Builds and runs the test executable 'dns' and runs the test script
test:
	@if [ ! -f "$(EXECUTABLE)" ]; then \
		$(MAKE) build; \
	fi
	@if [ ! -d "$(TEST_DIR)/myenv" ]; then \
		$(MAKE) test-venv; \
	fi
	$(MAKE) run-test

# Definition of shortcuts for command categories
CATEGORIES := MC C T P DEV

### MC # help: # Prints help for using the Makefile
help:
ifndef SUBMISSION_MODE
	@$(MAKE) $(SILENTOPT) install-help-dep
endif
	@{ \
	for CATEGORY in $(CATEGORIES); do \
		case $$CATEGORY in \
		"MC") FULL_CAT="Main Commands";; \
		"T") FULL_CAT="Test";; \
		"C") FULL_CAT="Clean (specialized)";; \
		"P") FULL_CAT="Pack (specialized)";; \
		"DEV") FULL_CAT="Install Dependencies";; \
		esac; \
		echo "$(COLOR_YELLOW)$$FULL_CAT:$(COLOR_RESET)"; \
		grep -E "^### $$CATEGORY # [a-zA-Z0-9_\-]+:.*?# .*$$" "$(lastword $(MAKEFILE_LIST))" | \
		sort -f | \
		awk 'BEGIN {FS = ":.*?# "}; \
		{ \
			gsub(/^### [A-Z]+ # /, "", $$1); \
			split($$2, lines, "\\\\n"); \
			printf "$(COLOR_BLUE)%-30s$(COLOR_RESET) %s\n", $$1, lines[1]; \
			for (i = 2; i <= length(lines); i++) { \
				printf "$(COLOR_BLUE)%-30s$(COLOR_RESET) %s\n", "", lines[i]; \
			} \
		}'; \
		echo ""; \
	done; \
	} | less -R

### MC # clean: # Runs 'clean-all' in developer and some specialized clean commands in submission mode (different versions)
ifndef SUBMISSION_MODE
clean: clean-all
else
clean: clean-build clean-test clean-doc clean-exec
endif

### MC # doc: # Generates project documentation into the `doc` directory (different versions)
ifndef SUBMISSION_MODE
doc:
	@$(MAKE) $(SILENTOPT) install-doc-dep
	$(MAKE) $(SILENTOPT) clean-doc
	doxygen Doxyfile
	cd $(DOC_DIR)/html && grep -v 'target="_self">resources\|target="_self">doc' files.html > temp.html && mv temp.html files.html
	@sed -i 's/\&lt;tt\&gt;/<tt>/g; s/\&lt;\/tt\&gt;/<\/tt>/g' $(DOC_DIR)/html/index.html
	@sed -i '/\&lt;style\&gt; .smallcaps { font-variant: small-caps; } \&lt;\/style\&gt;/d' $(DOC_DIR)/html/index.html
	@sed -i '/README.md/d' $(DOC_DIR)/doxygen_warnings.log
	@echo '<html><head><meta http-equiv="refresh" content="0; url=html/index.html"></head></html>' > $(DOC_DIR)/documentation.html
	@echo -e "$(COLOR_YELLOW)Do you want to open the HTML documentation in the main system browser? (y/n): $(COLOR_RESET)"
	@bash -c 'read -t 5 -p "" choice; \
	if [ "$$choice" = "y" ]; then \
		if grep -qEi "(Microsoft|WSL)" /proc/version &> /dev/null; then \
			cmd.exe /C start $(DOC_DIR)/documentation.html; \
		else \
			xdg-open $(DOC_DIR)/documentation.html; \
		fi \
	fi'
else
doc:
	$(MAKE) $(SILENTOPT) clean-doc
	doxygen Doxyfile
	@sed -i 's/\&lt;tt\&gt;/<tt>/g; s/\&lt;\/tt\&gt;/<\/tt>/g' $(DOC_DIR)/html/index.html
	@sed -i '/\&lt;style\&gt; .smallcaps { font-variant: small-caps; } \&lt;\/style\&gt;/d' $(DOC_DIR)/html/index.html
	@sed -i '/README.md/d' $(DOC_DIR)/doxygen_warnings.log
	@echo '<html><head><meta http-equiv="refresh" content="0; url=./html/index.html"></head></html>' > $(DOC_DIR)/documentation.html
endif

### MC # pack: # Creates a TAR archive with files intended for submission (not allowed for submission)
ifndef SUBMISSION_MODE
pack:
	@$(MAKE) $(SILENTOPT) install-pack-dep
	$(MAKE) $(SILENTOPT) clean
	mkdir -p $(PACK_DIR)
	$(MAKE) $(SILENTOPT) pack-prepare
	@echo ""
	@tar -C $(PACK_DIR) -cf $(PACK_DIR)/$(PACK_NAME).tar $(PACK_NAME)
else
pack:
	@echo "$(COLOR_RED)The 'pack' target is disabled for project submission.$(COLOR_RESET)"
endif


################################################################################
#                                                                              #
#                                BUILD TARGETS                                 #
#                                                                              #
################################################################################

###                                                                          ###
#                      COMPILATION OF RELEASE APP VERSION                      #
###                                                                          ###

# Build static library 'libdns.a'
$(RELEASE_BUILD_DIR)/$(ISA_LIB): $(LIB_OBJS_RELEASE)
	@mkdir -p $(RELEASE_BUILD_DIR)
	@echo "$(COLOR_MAGENTA)Creating static library '$(RELEASE_BUILD_DIR)/$(ISA_LIB)'...$(COLOR_RESET)"
	ar rcs $(RELEASE_BUILD_DIR)/$(ISA_LIB) $(LIB_OBJS_RELEASE)

# Build the excecutable 'dns'
$(EXECUTABLE): $(MAIN_OBJ_RELEASE) $(RELEASE_BUILD_DIR)/$(ISA_LIB)
	@echo "$(COLOR_MAGENTA)Linking executable '$(EXECUTABLE)'...$(COLOR_RESET)"
	$(CXX) $(CXXFLAGS_RELEASE) -o $(EXECUTABLE) $(MAIN_OBJ_RELEASE) -L$(RELEASE_BUILD_DIR) -l$(EXECUTABLE)

# Compile all object files into the 'build' directory
$(RELEASE_BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo "$(COLOR_MAGENTA)Compiling $<...$(COLOR_RESET)"
	$(CXX) $(CXXFLAGS_RELEASE) $(INCLUDES) -c $< -o $@


###                                                                          ###
#                       COMPILATION OF DEBUG APP VERSION                       #
###                                                                          ###

# Build static library 'libdns-debug.a'
$(DEBUG_BUILD_DIR)/$(ISA_LIB_DEBUG): $(LIB_OBJS_DEBUG)
	@mkdir -p $(DEBUG_BUILD_DIR)
	@echo "$(COLOR_MAGENTA)Creating static library '$(DEBUG_BUILD_DIR)/$(ISA_LIB_DEBUG)' for debug...$(COLOR_RESET)"
	ar rcs $(DEBUG_BUILD_DIR)/$(ISA_LIB_DEBUG) $(LIB_OBJS_DEBUG)

# Build the executable 'dns-debug'
$(EXECUTABLE)-debug: $(MAIN_OBJ_DEBUG) $(DEBUG_BUILD_DIR)/$(ISA_LIB_DEBUG)
	@echo "$(COLOR_MAGENTA)Linking executable '$(EXECUTABLE)-debug' for debug...$(COLOR_RESET)"
	$(CXX) $(CXXFLAGS_DEBUG) -o $(EXECUTABLE)-debug $(MAIN_OBJ_DEBUG) -L$(DEBUG_BUILD_DIR) -l$(EXECUTABLE)-debug

# Compile all object files into the 'build' directory
$(DEBUG_BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo "$(COLOR_MAGENTA)Compiling $< for debug...$(COLOR_RESET)"
	$(CXX) $(CXXFLAGS_DEBUG) $(INCLUDES) -c $< -o $@


################################################################################
#                                                                              #
#                        SPECIALIZED 'CLEAN' COMMANDS                          #
#                                                                              #
################################################################################

### C # clean-all: # Removes all created files (build, doc, executable, archive, ...)
clean-all: clean-build clean-exec clean-test clean-doc clean-pack

### C # clean-build: # Removes the 'build' directory
clean-build:
	rm -rf $(BUILD_DIR)

### C # clean-exec: # Removes the executable
clean-exec:
	rm -f $(EXECUTABLE)
	rm -f $(EXECUTABLE)-test
	rm -f $(EXECUTABLE)-debug

### C # clean-test: # Removes 'test/bin' folder with test executables
clean-test:
	rm -rf $(TEST_BIN_DIR)
	rm -rf $(TEST_DIR)/test_filter_copies
	rm -rf $(TEST_DIR)/.pytest_cache
	$(MAKE) $(SILENTOPT) clean-venv

### C # clean-doc: # Removes generated content of the 'doc' directory
clean-doc:
	find $(DOC_DIR) -mindepth 1 ! -path '$(DOC_DIR)/resources*' ! -path '$(DOC_DIR)/raw*' -delete || true

### C # clean-pack: # Removes the 'pack' directory including the archive (not allowed for submission)
ifndef SUBMISSION_MODE
clean-pack:
	rm -rf $(PACK_DIR)
else
clean-pack:
	@echo "$(COLOR_RED)The 'clean-pack' target is disabled for project submission.$(COLOR_RESET)"
endif


################################################################################
#                                                                              #
#                               'TEST' COMMANDS                                #
#                                                                              #
################################################################################

### T # run-test: # Sets executable permissions for the test script and runs it
run-test: $(EXECUTABLE)
	@chmod +x $(TEST_DIR)/run.sh
	cd $(TEST_DIR) && ./run.sh

### T # test-venv: # Creates a virtual environment for running integration tests
test-venv:
	@if [ ! -d "$(TEST_DIR)/myenv" ]; then \
		python3 -m venv $(TEST_DIR)/myenv; \
	fi && \
    $(TEST_DIR)/myenv/bin/python -m pip install --upgrade pip setuptools wheel && \
    $(TEST_DIR)/myenv/bin/python -m pip install -r $(TEST_DIR)/IntegrationTests/requirements.txt

### T # clean-venv: # Removes the virtual environment for running integration tests
clean-venv:
	rm -rf $(TEST_DIR)/myenv

################################################################################
#                                                                              #
#                    PACKAGING THE PROJECT FOR SUBMISSION INTO '.ZIP'          #
#                                                                              #
################################################################################

### P # pack-prepare: # Copies all necessary files to the 'pack/xkalinj00' directory (not allowed for submission)
ifndef SUBMISSION_MODE
pack-prepare:
	@{ \
		missing_files=0; \
		if [ -d "$(SRC_DIR)" ]; then \
			rsync -a --include '*/' --include '*.cpp' --include '*.hpp' --include '*.py' --exclude '*' --exclude '*/' \
			--prune-empty-dirs ./ $(ARCHIVE_DIR)/; \
		else \
			echo "$(COLOR_RED)\nError: The directory "$(SRC_DIR)" does not exist.$(COLOR_RESET)"; \
		fi; \
		if [ -d "$(TEST_DIR)" ]; then \
			rsync -a --include '*.cpp' --include '*.hpp' --exclude '*/' $(TEST_DIR)/ $(ARCHIVE_DIR)/$(TEST_DIR)/; \
		else \
			echo "$(COLOR_RED)\nError: The directory "$(TEST_DIR)" does not exist.$(COLOR_RESET)"; \
		fi; \
		if [ -d "$(DOC_DIR)/resources" ]; then \
			mkdir -p $(ARCHIVE_DIR)/$(DOC_DIR)/resources; \
			rsync -a $(DOC_DIR)/resources/ $(ARCHIVE_DIR)/$(DOC_DIR)/resources/; \
		else \
			echo "$(COLOR_RED)\nError: The directory "$(DOC_DIR)/resources" does not exist.$(COLOR_RESET)"; \
		fi; \
		if [ -f "Makefile" ]; then \
			rsync -a Makefile $(ARCHIVE_DIR)/; \
			sed -i '0,/SUBMISSION_MODE/ {/SUBMISSION_MODE/ s|#||g}' $(ARCHIVE_DIR)/Makefile; \
            sed -i '0,/SUBMISSION_MODE/ s|^\s*\(.*SUBMISSION_MODE.*\)$$|\1|' $(ARCHIVE_DIR)/Makefile; \
		else \
			missing_files=1; \
		fi; \
		if [ -f "CMakeLists.txt" ]; then \
			rsync -a CMakeLists.txt $(ARCHIVE_DIR)/; \
		else \
			missing_files=1; \
		fi; \
		if [ -f "Doxyfile" ]; then \
			rsync -a Doxyfile $(ARCHIVE_DIR)/; \
		else \
			missing_files=1; \
		fi; \
		if [ -f "README.md" ]; then \
			rsync -a README.md $(ARCHIVE_DIR)/; \
		else \
			missing_files=1; \
		fi; \
		if [ -f "manual.pdf" ]; then \
			rsync -a manual.pdf $(ARCHIVE_DIR)/; \
		else \
			missing_files=1; \
		fi; \
		if [ -f "LICENSE" ]; then \
			rsync -a LICENSE $(ARCHIVE_DIR)/; \
		else \
			missing_files=1; \
		fi; \
		echo "$(COLOR_GREEN)\nList of copied files:$(COLOR_RESET)"; \
		find $(PACK_DIR) -type f -printf "$(COLOR_GREEN)%p$(COLOR_RESET)\n"; \
		if [ "$$missing_files" -eq 1 ]; then \
			echo "$(COLOR_RED)\nList of missing files:$(COLOR_RESET)"; \
		fi; \
		if [ ! -f "$(ARCHIVE_DIR)/Makefile" ]; then \
			echo "$(COLOR_RED)Error: The file 'Makefile' was not copied.$(COLOR_RESET)"; \
		fi; \
		if [ ! -f "$(ARCHIVE_DIR)/CMakeLists.txt" ]; then \
        	echo "$(COLOR_RED)Error: The file 'CMakeLists.txt' was not copied.$(COLOR_RESET)"; \
        fi; \
		if [ ! -f "$(ARCHIVE_DIR)/Doxyfile" ]; then \
			echo "$(COLOR_RED)Error: The file 'Doxyfile' was not copied.$(COLOR_RESET)"; \
		fi; \
		if [ ! -f "$(ARCHIVE_DIR)/README.md" ]; then \
			echo "$(COLOR_RED)Error: The file 'README.md' was not copied.$(COLOR_RESET)"; \
		fi; \
		if [ ! -f "$(ARCHIVE_DIR)/manual.pdf" ]; then \
			echo "$(COLOR_RED)Error: The file 'manual.pdf' was not copied.$(COLOR_RESET)"; \
		fi; \
		if [ ! -f "$(ARCHIVE_DIR)/LICENSE" ]; then \
			echo "$(COLOR_RED)Error: The file 'LICENSE' was not copied.$(COLOR_RESET)"; \
		fi; \
	}
else
pack-prepare:
	@echo "$(COLOR_RED)The 'pack-prepare' target is disabled for project submission.$(COLOR_RESET)"
endif


################################################################################
#                                                                              #
#                    TARGETS FOR INSTALLING NECESSARY TOOLS                    #
#                                                                              #
################################################################################

### DEV # developer-mode: # Switches the Makefile to DEVELOPER mode
developer-mode:
	@sed -i '0,/SUBMISSION_MODE/ {/^[^#]*SUBMISSION_MODE/ s/^/#/}' Makefile
	@echo "$(COLOR_MAGENTA)The Makefile has been switched to:$(COLOR_RESET) $(COLOR_YELLOW)DEVELOPER MODE$(COLOR_RESET)"

### DEV # submission-mode: # Switches the Makefile to SUBMISSION mode
submission-mode:
	@sed -i '0,/SUBMISSION_MODE/ {/SUBMISSION_MODE/ s|#||g}' Makefile
	@sed -i '0,/SUBMISSION_MODE/ s|^\s*\(.*SUBMISSION_MODE.*\)$$|\1|' Makefile
	@echo "$(COLOR_MAGENTA)The Makefile has been switched to:$(COLOR_RESET) $(COLOR_YELLOW)SUBMISSION MODE$(COLOR_RESET)"

### DEV # install-dev-dep: # Installs dependencies needed for using all 'Makefile' functions (not allowed for submission)
ifndef SUBMISSION_MODE
install-dev-dep: update-dep install-help-dep install-build-dep install-doc-dep install-pack-dep install-test-dep
else
install-dev-dep:
	@echo "$(COLOR_RED)The 'install-dev-dep' target is disabled for project submission.$(COLOR_RESET)"
endif

### DEV # install-help-dep: # Installs dependencies needed for printing 'Makefile' help - 'less' (not allowed for submission)
ifndef SUBMISSION_MODE
install-help-dep:
	@dpkg -s less >/dev/null 2>&1 || (echo "Installing less" && sudo apt-get install less)
else
install-help-dep:
	@echo "$(COLOR_RED)The 'install-help-dep' target is disabled for project submission.$(COLOR_RESET)"
endif

### DEV # install-build-dep: # Installs dependencies needed for developer mode app compilation - 'cmake' (not allowed for submission)
ifndef SUBMISSION_MODE
install-build-dep:
	@dpkg -s cmake >/dev/null 2>&1 || (echo "Installing cmake" && sudo apt-get install cmake)
else
install-build-dep:
	@echo "$(COLOR_RED)The 'install-build-dep' target is disabled for project submission.$(COLOR_RESET)"
endif

### DEV # install-doc-dep: # Installs dependencies needed for generating documentation - 'doxygen' (not allowed for submission)
ifndef SUBMISSION_MODE
install-doc-dep:
	@dpkg -s doxygen >/dev/null 2>&1 || (echo "Installing doxygen" && sudo apt-get install doxygen)
else
install-doc-dep:
	@echo "$(COLOR_RED)The 'install-doc-dep' target is disabled for project submission.$(COLOR_RESET)"
endif

### DEV # install-pack-dep: # Installs dependencies needed for project packaging - 'rsync', 'tar' (not allowed for submission)
ifndef SUBMISSION_MODE
install-pack-dep:
	@dpkg -s rsync >/dev/null 2>&1 || (echo "Installing rsync" && sudo apt-get install rsync)
	@dpkg -s tar >/dev/null 2>&1 || (echo "Installing tar" && sudo apt-get install tar)
else
install-pack-dep:
	@echo "$(COLOR_RED)The 'install-pack-dep' target is disabled for project submission.$(COLOR_RESET)"
endif

### DEV # update-dep: # Updates the list of available packages (not allowed for submission)
ifndef SUBMISSION_MODE
update-dep:
	sudo apt-get update -y
else
update-dep:
	@echo "$(COLOR_RED)The 'update-dep' target is disabled for project submission.$(COLOR_RESET)"
endif

### end of file Makefile ###
