# ESP32 NANO &amp; BANANO ATM

A small prototype for a Nano &amp; Banano ATM. Payouts in physical euro coins, made with ESP32 and Lego bricks. Link to reddit post: https://www.reddit.com/r/nanocurrency/comments/pssu65/i_built_a_prototype_for_a_nano_banano_atm_payouts

YouTube video: https://www.youtube.com/watch?v=7HnLuUlSCQs

The ESP32 microcontroller has built-in WiFi and allows you to program and use libraries in the Arduino IDE. It can be powered from a powerbank, needs 5V on the power supply, runs on 3.3V logic. The cost of such a microcontroller is around $ 8 or less. It also has PWM pins that allow you to control the servo and allows communication via SPI, which was used to display messages on the LCD screen from Nokia 5110.

The Nokia 5110 display works on 3.3V logic and has a resolution of 84x48 pixels, the display is single-color, has LED backlight. The display matrix consumes a current of approx. 6-7mA, while the backlight diodes, when supplied with 3.3V voltage without a limiting resistor (at maximum backlight), consume approx. 100mA of current.

Micro Servo SG90 is a popular and cheap servo. The rotation angle is 180 °. Direct powering of the servo from the microcontroller could result in its damage - that's why an additional 9V battery with the MB102 power module was used, which is easy to attach to the prototype board and provides 5V voltage (which is within the required limit of the servo).

The housing is made of Lego bricks, and the servo is placed in it in such a way that the arm connected to the SG90 allows for pushing out 1 EURO coins individually. The coins are stacked on top of each other and the housing allows only the coin at the bottom to be ejected. Additionally, one button was used in the project to interact with the user.

After connecting the power supply, the device connects to the WiFi network and, via API, checks the status of the wallet to which the user will deposit the cryptocurrency. This state is saved at the beginning, so that later when it is refreshed, it can be compared with the previous state and how much was transferred by the user. After saving the account balance, the device switches to the waiting mode for pressing the button and starting the transaction.

After pressing the button, the current price of the cryptocurrency is downloaded via the API, converted into the value of 1 EURO rounded up to hundredths (in the case of Nano) and to the whole (in the case of Banano) and displays the rates on the screen. The program then updates the information on account balances and compares them with the previous ones. It does this at intervals of seconds and for 50 attempts. If during these 50 attempts (about 3 minutes) no larger wallet balance is detected, the machine will go back to the state of waiting for pressing the button and restarting the whole process.

If, however, a larger balance is detected on any of the wallets, the program checks whether the difference between these amounts corresponds to the value of at least 1 EURO in relation to the displayed exchange rate and pays the appropriate amount of EURO coins by pushing the coins with the microserve arm. After the coins have been dispensed, the machine returns to its initial state.
