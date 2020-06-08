# Tag on this commit
TAG := $(shell git describe --tags --exact-match)
# Commit hash from git
COMMIT := $(shell git rev-parse --short HEAD)
VERSION := $(if $(TAG),\\\"$(TAG)\\\",\\\"$(COMMIT)\\\")
BOARDS = pca10040 ruuvigw_nrf
VARIANTS = debug release

.PHONY: all sync ${BOARDS} analysis publish clean 

all: sync clean ${BOARDS}

sync:
	@echo Synchronizing GIT...
	# TODO: reject if repo is not clean
	git submodule update --init --recursive
	git submodule sync --recursive
	git submodule update --init --recursive


pca10040:
	@echo build FW ${VERSION}
	$(MAKE) -C targets/pca10040 clean
	$(MAKE) -C targets/pca10040 DEBUG=-DDEBUG FW_VERSION=-DAPPLICATION_FW_VERSION=${VERSION}
	targets/pca10040/package.sh -n ruuvigw_debug
	$(MAKE) -C targets/pca10040 clean
	$(MAKE) -C targets/pca10040 DEBUG=-DNDEBUG FW_VERSION=-DAPPLICATION_FW_VERSION=${VERSION}
	targets/pca10040/package.sh -n ruuvigw_release

ruuvigw_nrf:
	$(MAKE) -C targets/ruuvigw_nrf clean
	$(MAKE) -C targets/ruuvigw_nrf DEBUG=-DNDEBUG FW_VERSION=-DAPPLICATION_FW_VERSION=${VERSION}
	targets/ruuvigw_nrf/package.sh -n ruuvigw_release

analysis:
	@echo build FW ${VERSION}
	$(MAKE) -C targets/ruuvigw_nrf clean
	$(MAKE) -C targets/ruuvigw_nrf DEBUG=-DNDEBUG FW_VERSION=-DAPPLICATION_FW_VERSION=${VERSION} OPT="-Og -g3" VERBOSE=1 ABSOLUTE_PATHS=1

# https://medium.com/@systemglitch/continuous-integration-with-jenkins-and-github-release-814904e20776
publish:
	@echo Publishing $(TAG)
	./release.sh

clean:
	@echo cleaning build files…
	$(MAKE) -C targets/pca10040 clean
	$(MAKE) -C targets/ruuvigw_nrf clean
