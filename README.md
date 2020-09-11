# ruuvi.gateway_nrf.c
Ruuvi Gateway nRF52 code

[![Build Status](https://travis-ci.org/ruuvi/ruuvi.gateway_nrf.c.svg?branch=master)](https://travis-ci.org/ruuvi/ruuvi.gateway_nrf.c)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=ruuvi.gateway_nrf.c.c&metric=alert_status)](https://sonarcloud.io/dashboard?id=ruuvi.gateway_nrf.c.c)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=ruuvi.gateway_nrf.c.c&metric=bugs)](https://sonarcloud.io/dashboard?id=ruuvi.gateway_nrf.c.c)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=ruuvi.gateway_nrf.c.c&metric=code_smells)](https://sonarcloud.io/dashboard?id=ruuvi.gateway_nrf.c.c)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=ruuvi.gateway_nrf.c.c&metric=coverage)](https://sonarcloud.io/dashboard?id=ruuvi.gateway_nrf.c.c)
[![Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=ruuvi.gateway_nrf.c.c&metric=duplicated_lines_density)](https://sonarcloud.io/dashboard?id=ruuvi.gateway_nrf.c.c)
[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=ruuvi.gateway_nrf.c.c&metric=ncloc)](https://sonarcloud.io/dashboard?id=ruuvi.gateway_nrf.c.c)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=ruuvi.gateway_nrf.c.c&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=ruuvi.gateway_nrf.c.c)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=ruuvi.gateway_nrf.c.c&metric=reliability_rating)](https://sonarcloud.io/dashboard?id=ruuvi.gateway_nrf.c.c)
[![Technical Debt](https://sonarcloud.io/api/project_badges/measure?project=ruuvi.gateway_nrf.c.c&metric=sqale_index)](https://sonarcloud.io/dashboard?id=ruuvi.gateway_nrf.c.c)

# Setting up
## SDK 15.3
Download [Nordic SDK15.3](https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/) and install it at the project root.
If you're working on multiple nRF projects, you can (and should) use softlinking instead.

## Submodules
Run `git submodule update --init --recursive`. This will search for and install the other git repositories referenced by this project. If any of the submodules has a changed remote, you'll need to run `git submodule sync --recursive` and again `git submodule update --init --recursive` to update the modules from new remotes. 

## Toolchain
ARMGCC is used for [Jenkins builds](http://jenkins.ruuvi.com/job/ruuvi.gateway_nrf.c/), it's recommended to use SES for developing.
 
Segger Embedded Studio can be set up by installing [nRF Connect for Desktop](https://www.nordicsemi.com/?sc_itemid=%7BB935528E-8BFA-42D9-8BB5-83E2A5E1FF5C%7D) 
and following Getting Started plugin instructions.

Start SES and open `ruuvi.gateway_nrf.c.emProject` in `src` directory, each of the target boards is in their own project.

## Code style
Code is formatted with [Artistic Style](http://astyle.sourceforge.net). 
Run `astyle --project=.astylerc ./target_file`.

## Static analysis
The code can be checked with PVS Studio and Sonarcloud for some common errors, style issues and potential problems. [Here](https://ruuvi.github.io/ruuvi.gateway_nrf.c/fullhtml/index.html) is a link to generated report which gets pushed to GitHub.


### PVS
Obtain license and software from [Viva64](https://www.viva64.com/en/pvs-studio/).

Make runs PVS Studio scan and outputs results under doxygen/html/fullhtml. 

This results into hundreds of warnings, it is up to you to filter the data you're interested in. For example you probably want to filter out warnings related to 64-bit systems. 

### Sonar scan
Travis pushes the results to [SonarCloud.IO](https://sonarcloud.io/dashboard?id=ruuvi.gateway_nrf.c.c).
SonarCloud uses access token which is private to Ruuvi, you'll need to fork the project and setup
the SonarCloud under your own account if you wish to run Sonar Scan on your own code.

# Running unit tests
## Ceedling
Unit tests are implemented with Ceedling. Run the tests with
`ceedling test:all`

# Builds
Builds are in the Github [project releases](https://github.com/ruuvi/ruuvi.gateway_nrf.c/releases).

# Developing Ruuvi Firmware
## Compiling with SES
Double click to select project: `pca10040` or `ruuvigw_nrf`. 
Select the confuguaration used to build the project. For `ruuvigw_nrf` only `Release` version buildable.
Press `F7` to build and `F5` for debug, or select `Build` and `Debug` in menu.

## Compiling with ARMGCC
Run `make` in `src` directory to build all the sources. 
Run `make clean` in `src` directory to clean current build.

# Flashing
## With nrfjprog
How to program nRF5x SoCs with nrfjprog you can find [Nordic Infocenter](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fug_nrf5x_cltools%2FUG%2Fcltools%2Fnrf5x_nrfjprogexe.html).To get started:
```
nrfjprog --eraseall
nrfjprog --program ruuvigw_nrf_armgcc_ruuvigw_release_vX.X.X_full.hex
nrfjprog --reset
```
