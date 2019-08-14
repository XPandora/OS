## Lab 6: Network Driver

- Student ID: 516030910141
- Name: Tianyi Xie

#### Question:

1. How did you structure your transmit implementation? In particular, what do you do if the transmit ring is full?

- First, read the TDT descriptor and check whether the DD bit status is set. If not, it means that transmit ring is full and return -E_AGAIN to notify the user environment to retry sending request. Second, copy the data to the transmit buffer. Third, set the length and cmd as well as unmasking the DD  bit status. Finally, increase the TDT and return 0 as success.

2. How did you structure your receive implementation? In particular, what do you do if the receive queue is empty and a user environment requests the next incoming packet?

- First, read the RDT to get the correct descriptor and check  whether the DD bit status is set. If not, it means the receive queue is empty and return -E_AGAIN to notify the user environment to retry receiving request. Second, copy the data to the requesting buffer from receiving buffer. Third, unmask the DD and EOP bit status. Finally, increase the RDT and return the length of received packet.

3. What does the web page served by JOS's web server say?

- Title: This file came from JOS. Sliding sentence: Cheesy web page!

4. How long approximately did it take you to do this lab?

- About 9 hours.

#### Challenge:

- **Challenge!** Read about the EEPROM in the developer's manual and write the code to load the E1000's MAC address out of the EEPROM. Currently, QEMU's default MAC address is hard-coded into both your receive initialization and lwIP. Fix your initialization to use the MAC address you read from the EEPROM, add a system call to pass the MAC address to lwIP, and modify lwIP to the MAC address read from the card. Test your change by configuring QEMU to use a different MAC address.

1. Modify the E1000 struct: add EECD and EERD attribute.
2. Add a e1000_eeprom_read_register function to help read mac address. After setting the address and start bits, we can get the data from 16-31 bits once the done bit is set.
3. In e1000_rx_init, get and save the 6 bytes mac address by calling e1000_eeprom_read_register.
4. Add a sys_read_mac system call to tell the user environment the mac address.
5. Modify net/lwip/jos/jif/jif.c to load and set the mac address by sys_read_mac system call. 