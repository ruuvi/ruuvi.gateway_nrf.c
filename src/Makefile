# Tag on this commit
TAG := $(shell git describe --tags --exact-match)
# Commit hash from git
COMMIT := $(shell git rev-parse --short HEAD)
VERSION := $(if $(TAG),\\\"$(TAG)\\\",\\\"$(COMMIT)\\\")
BOARDS = pca10040 pca10059 ruuvigw_nrf
VARIANTS = debug release

.PHONY: all sync ${BOARDS} analysis publish clean 

all: sync clean ${BOARDS} sha256_stub

sync:
	@echo Synchronizing GIT...
	# TODO: reject if repo is not clean
	git submodule update --init --recursive
	git submodule sync --recursive
	git submodule update --init --recursive

# nRF52-DK
pca10040:
	@echo build FW ${VERSION}
	$(MAKE) -j1 -C targets/pca10040 clean
	$(MAKE) -j1 -C targets/pca10040 DEBUG=-DDEBUG FW_VERSION=-DAPPLICATION_FW_VERSION=${VERSION}
	targets/pca10040/package.sh -n ruuvigw_debug
	$(MAKE) -j1 -C targets/pca10040 clean
	$(MAKE) -j1 -C targets/pca10040 DEBUG=-DNDEBUG FW_VERSION=-DAPPLICATION_FW_VERSION=${VERSION}
	targets/pca10040/package.sh -n ruuvigw_release

# nRF52840-DK
pca10059:
	@echo build FW ${VERSION}
	$(MAKE) -j1 -C targets/pca10059 clean
	$(MAKE) -j1 -C targets/pca10059 DEBUG=-DDEBUG FW_VERSION=-DAPPLICATION_FW_VERSION=${VERSION}
	targets/pca10059/package.sh -n ruuvigw_debug
	$(MAKE) -j1 -C targets/pca10059 clean
	$(MAKE) -j1 -C targets/pca10059 DEBUG=-DNDEBUG FW_VERSION=-DAPPLICATION_FW_VERSION=${VERSION}
	targets/pca10059/package.sh -n ruuvigw_release

ruuvigw_nrf:
	$(MAKE) -j1 -C targets/ruuvigw_nrf clean
	$(MAKE) -j1 -C targets/ruuvigw_nrf DEBUG=-DNDEBUG FW_VERSION=-DAPPLICATION_FW_VERSION=${VERSION}
	targets/ruuvigw_nrf/package.sh -n ruuvigw_release

sha256_stub:
	$(MAKE) -j1 -C targets/sha256_stub clean
	$(MAKE) -j1 -C targets/sha256_stub DEBUG=-DNDEBUG

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
	$(MAKE) -C targets/pca10059 clean
	$(MAKE) -C targets/ruuvigw_nrf clean
