
Flashing RuuviGateway

1) From  https://github.com/ruuvi/ruuvi.gateway_nrf.c/releases
    download full.hex and 
    tail -n 1 *.hex   #  to see End-of-file record: :00000001FF to be sure it's a complete file.

2) Connect Nordic Dev Board to laptop/desktop USB

3) Connect Ruuvi gateway to laptop/desktop USB

4) Connect T-connect cable to Nordic Dev Board mini 10 pin p19 at the corner of the Nordic Dev Board

5) Attach 6pin + 3 locator nails end of T-connect to RuuviGateway with either retainer clips or hold it on firmly

6a) Run         ./rF52flash.sh      xxxx.hex  # from step 1, This invokes nrfjprog to display part number, MAC and does the flashing

6B) Observe 

    Parsing image file.

    Erasing page at address 0x0.

    Erasing page at address 0x1000.

    Erasing page at address 0x2000.
    
    ...
    
    Erasing page at address 0x2C000.
    
    Applying system reset.
    
    Checking that the area to write is not protected.
    
    Programming device.
    
    Verifying programming.
    
    Verified OK.



