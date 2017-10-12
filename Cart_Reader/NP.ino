//******************************************
// NINTENDO POWER Cartridges for SFC and GB
//  (GB Memory starts at around line 1460)
//******************************************

/******************************************
   SF Memory Cassette
******************************************/

/******************************************
   SF Memory Clock Source
******************************************/
// The clock signal for the SF Memory cassette
// is generated with the Adafruit Clock Generator
// If you don't have one just plug a wire into CLK0
// (Snes Pin 1) and let the 50/60Hz noise of your
// mains do the rest, works in ~80% of all cases ;)

/******************************************
   Variables
 *****************************************/
// SF Memory status
byte sfmReady = 0;

// SF Memory Menu
boolean hasMenu = true;
byte numGames = 0;

// Arrays that hold game info
int gameSize[8];
int saveSize[8];
byte gameAddress[8];
byte gameVersion[8];
char gameCode[8][10];
boolean hirom[8];

/******************************************
  Menu
*****************************************/
// SFM menu items
static const char sfmMenuItem1[] PROGMEM = "Game Menu";
static const char sfmMenuItem2[] PROGMEM = "Flash Menu";
static const char sfmMenuItem3[] PROGMEM = "Reset";
static const char* const menuOptionsSFM[] PROGMEM = {sfmMenuItem1, sfmMenuItem2, sfmMenuItem3};

// SFM flash menu items
static const char sfmFlashMenuItem1[] PROGMEM = "Read Flash";
static const char sfmFlashMenuItem2[] PROGMEM = "Write Flash";
static const char sfmFlashMenuItem3[] PROGMEM = "Print Mapping";
static const char sfmFlashMenuItem4[] PROGMEM = "Read Mapping";
static const char sfmFlashMenuItem5[] PROGMEM = "Write Mapping";
static const char sfmFlashMenuItem6[] PROGMEM = "Back";
static const char* const menuOptionsSFMFlash[] PROGMEM = {sfmFlashMenuItem1, sfmFlashMenuItem2, sfmFlashMenuItem3, sfmFlashMenuItem4, sfmFlashMenuItem5, sfmFlashMenuItem6};

// SFM game menu items
static const char sfmGameMenuItem1[] PROGMEM = "Read Sram";
static const char sfmGameMenuItem2[] PROGMEM = "Read Game";
static const char sfmGameMenuItem3[] PROGMEM = "Write Sram";
static const char sfmGameMenuItem4[] PROGMEM = "Switch Game";
static const char sfmGameMenuItem5[] PROGMEM = "Reset";
static const char* const menuOptionsSFMGame[] PROGMEM = {sfmGameMenuItem1, sfmGameMenuItem2, sfmGameMenuItem3, sfmGameMenuItem4, sfmGameMenuItem5};

void sfmMenu() {
  // create menu with title and 3 options to choose from
  unsigned char mainMenu;
  // Copy menuOptions out of progmem
  convertPgm(menuOptionsSFM, 3);
  mainMenu = question_box("SF Memory", menuOptions, 3, 0);

  // wait for user choice to come back from the question box menu
  switch (mainMenu)
  {
    // Game menu
    case 0:
      sfmGameMenu();
      break;
    // Flash menu
    case 1:
      mode = mode_SFM_Flash;
      break;
    // Reset
    case 2:
      asm volatile ("  jmp 0");
      break;
  }
}

void sfmGameMenu() {
  // Switch to hirom all
  if (send_SFM(0x04) == 0x2A) {
    delay(300);

    // Fill arrays with data
    getGames();

    if (hasMenu) {
      // Create submenu options
      char menuOptionsSFMGames[8][20];
      for (int i = 0; i < (numGames); i++) {
        strncpy(menuOptionsSFMGames[i], gameCode[i], 10);
      }

      // Create menu with title and numGames options to choose from
      unsigned char gameSubMenu;
      // wait for user choice to come back from the question box menu
      gameSubMenu = question_box("Select Game", menuOptionsSFMGames, numGames, 0);

      // Switch to game
      send_SFM(gameSubMenu + 0x80);
      delay(200);
      // Check for successfull switch
      byte timeout = 0;
      while (readBank_SNES(0, 0x2400) != 0x7D) {
        delay(200);
        // Try again
        send_SFM(gameSubMenu + 0x80);
        delay(200);
        timeout++;
        // Abort, something is wrong
        if (timeout == 5) {
          display_Clear();
          print_Msg(F("Game "));
          print_Msg(gameSubMenu + 0x80, HEX);
          println_Msg(F(" Timeout"));
          println_Msg(readBank_SNES(0, 0x2400), HEX);
          println_Msg(F(""));
          print_Error(F("Powercycle SFM cart"), true);
        }
      }
      // Copy gameCode to romName in case of japanese chars in romName
      strcpy(romName, gameCode[gameSubMenu + 1]);

      // Print info
      getCartInfo_SNES();
      mode = mode_SFM_Game;
    }
    else {
      // No menu so switch to only game
      // Switch to game
      send_SFM(0x80);
      delay(200);

      // Copy gameCode to romName in case of japanese chars in romName
      strcpy(romName, gameCode[0]);

      // Print info
      getCartInfo_SNES();
      mode = mode_SFM_Game;
    }
  }
  else {
    print_Error(F("Switch to HiRom failed"), false);
  }
}

void sfmGameOptions() {
  // create menu with title and 5 options to choose from
  unsigned char gameSubMenu;
  // Copy menuOptions out of progmem
  convertPgm(menuOptionsSFMGame, 5);
  gameSubMenu = question_box("SFM Game Menu", menuOptions, 5, 0);

  // wait for user choice to come back from the question box menu
  switch (gameSubMenu)
  {
    // Read sram
    case 0:
      display_Clear();
      // Change working dir to root
      sd.chdir("/");
      readSRAM();
      break;

    // Read rom
    case 1:
      display_Clear();
      // Change working dir to root
      sd.chdir("/");
      readROM_SNES();
      compare_checksum();
      break;

    // Write sram
    case 2:
      display_Clear();
      // Change working dir to root
      sd.chdir("/");
      writeSRAM(1);
      unsigned long wrErrors;
      wrErrors = verifySRAM();
      if (wrErrors == 0) {
        println_Msg(F("Verified OK"));
        display_Update();
      }
      else {
        print_Msg(F("Error: "));
        print_Msg(wrErrors);
        println_Msg(F(" bytes "));
        print_Error(F("did not verify."), false);
      }
      break;

    // Switch game
    case 3:
      sfmGameMenu();
      break;

    // Reset
    case 4:
      asm volatile ("  jmp 0");
      break;
  }
  if (gameSubMenu != 3) {
    println_Msg(F(""));
    println_Msg(F("Press Button..."));
    display_Update();
    wait();
  }
}

void sfmFlashMenu() {
  // create menu with title and 6 options to choose from
  unsigned char flashSubMenu;
  // Copy menuOptions out of progmem
  convertPgm(menuOptionsSFMFlash, 6);
  flashSubMenu = question_box("SFM Flash Menu", menuOptions, 6, 0);

  // wait for user choice to come back from the question box menu
  switch (flashSubMenu)
  {
    // Read Flash
    case 0:
      // Clear screen
      display_Clear();

      // Reset to root directory
      sd.chdir("/");

      // Reset to HIROM ALL
      romType = 1;
      print_Msg(F("Switch to HiRom..."));
      display_Update();
      if (send_SFM(0x04) == 0x2A) {
        println_Msg(F("OK"));
        display_Update();

        // Reset flash
        resetFlash_SFM(0xC0);
        resetFlash_SFM(0xE0);

        flashSize = 4194304;
        numBanks = 64;

        // Get name, add extension and convert to char array for sd lib
        EEPROM_readAnything(10, foldern);
        sprintf(fileName, "SFM%d", foldern);
        strcat(fileName, ".bin");
        sd.mkdir("NP", true);
        sd.chdir("NP");
        // write new folder number back to eeprom
        foldern = foldern + 1;
        EEPROM_writeAnything(10, foldern);

        // Read flash
        readFlash_SFM();
      }
      else {
        print_Error(F("Switch to HiRom failed"), false);
      }
      break;

    // Write Flash
    case 1:
      // Clear screen
      display_Clear();

      // Print warning
      println_Msg(F("Attention"));
      println_Msg(F("This will erase your"));
      println_Msg(F("NP Cartridge."));
      println_Msg("");
      println_Msg(F("Press Button"));
      println_Msg(F("to continue"));
      display_Update();
      wait();

      // Clear screen
      display_Clear();

      filePath[0] = '\0';
      sd.chdir("/");
      // Launch file browser
      fileBrowser("Select 4MB file");
      display_Clear();
      sprintf(filePath, "%s/%s", filePath, fileName);
      flashSize = 2097152;
      numBanks = 32;
      println_Msg(F("Writing 1st rom"));
      display_Update();
      // Program 1st flashrom
      write_SFM(0xC0, 0);
      display_Clear();
      println_Msg(F("Writing 2nd rom"));
      display_Update();
      // Program 2nd flashrom
      write_SFM(0xE0, 2097152);
      break;

    // Print mapping
    case 2:
      // Clear screen
      display_Clear();

      // Reset to root directory
      sd.chdir("/");

      // Reset to HIROM ALL
      romType = 1;
      print_Msg(F("Switch to HiRom..."));
      display_Update();
      if (send_SFM(0x04) == 0x2A) {
        println_Msg(F("OK"));
        display_Update();
        idFlash_SFM(0xC0);
        if (strcmp(flashid, "c2f3") == 0) {
          idFlash_SFM(0xE0);
          if (strcmp(flashid, "c2f3") == 0) {
            // Reset flash
            resetFlash_SFM(0xC0);
            resetFlash_SFM(0xE0);
            delay(100);
            // Clear screen
            display_Clear();
            printMapping();
            resetFlash_SFM(0xC0);
            resetFlash_SFM(0xE0);
          }
          else {
            print_Error(F("Error: Wrong Flash ID"), true);
          }
        }
        else {
          print_Error(F("Error: Wrong Flash ID"), true);
        }
      }
      else {
        print_Error(F("failed"), false);
      }
      break;

    // Read mapping
    case 3:
      // Clear screen
      display_Clear();

      // Reset to root directory
      sd.chdir("/");

      // Reset to HIROM ALL
      romType = 1;
      print_Msg(F("Switch to HiRom..."));
      display_Update();
      if (send_SFM(0x04) == 0x2A) {
        println_Msg(F("OK"));
        display_Update();
        idFlash_SFM(0xC0);
        if (strcmp(flashid, "c2f3") == 0) {
          idFlash_SFM(0xE0);
          if (strcmp(flashid, "c2f3") == 0) {
            // Reset flash
            resetFlash_SFM(0xC0);
            resetFlash_SFM(0xE0);
            delay(100);
            readMapping();
            resetFlash_SFM(0xC0);
            resetFlash_SFM(0xE0);
          }
          else {
            print_Error(F("Error: Wrong Flash ID"), true);
          }
        }
        else {
          print_Error(F("Error: Wrong Flash ID"), true);
        }
      }
      else {
        print_Error(F("failed"), false);
      }
      break;

    // Write mapping
    case 4:
      // Clear screen
      display_Clear();

      // Print warning
      println_Msg(F("Attention"));
      println_Msg(F("This will erase your"));
      println_Msg(F("NP Cartridge."));
      println_Msg("");
      println_Msg(F("Press Button"));
      println_Msg(F("to continue"));
      display_Update();
      wait();

      // Clear screen
      display_Clear();

      // Reset to root directory
      sd.chdir("/");

      // Erase mapping
      eraseMapping(0xD0);
      eraseMapping(0xE0);
      print_Msg(F("Blankcheck..."));
      display_Update();
      if (blankcheckMapping_SFM()) {
        println_Msg(F("OK"));
        display_Update();
      }
      else {
        println_Msg(F("Nope"));
        break;
      }

      // Clear screen
      display_Clear();

      // Clear filepath
      filePath[0] = '\0';

      // Reset to root directory
      sd.chdir("/");

      // Launch file browser
      fileBrowser("Select MAP file");
      display_Clear();
      sprintf(filePath, "%s/%s", filePath, fileName);
      display_Update();

      // Write mapping
      writeMapping_SFM(0xD0, 0);
      writeMapping_SFM(0xE0, 256);
      break;

    // Go back
    case 5:
      mode = mode_SFM;
      break;
  }
  if (flashSubMenu != 5) {
    println_Msg(F(""));
    println_Msg(F("Press Button..."));
    display_Update();
    wait();
  }
}

// Read the games from the menu area
void getGames() {
  // Set data pins to input
  dataIn();
  // Set control pins to input
  controlIn_SNES();

  // Check if menu is present
  byte menuString[] = {0x4D, 0x45, 0x4E, 0x55, 0x20, 0x50, 0x52, 0x4F, 0x47, 0x52, 0x41, 0x4D};
  for (int i = 0; i < 12; i++) {
    if (menuString[i] != readBank_SNES(0xC0, 0x7FC0 + i)) {
      hasMenu = false;
    }
  }

  if (hasMenu) {
    // Count number of games
    for (word i = 0x0000; i < 0xE000; i += 0x2000) {
      if (readBank_SNES(0xC6, i) == numGames )
        numGames++;
    }

    // Get game info
    for (int i = 0; i < numGames; i++) {
      // Read starting address and size
      gameAddress[i] = 0xC0 + readBank_SNES(0xC6, i * 0x2000 + 0x01) * 0x8;
      gameSize[i] = readBank_SNES(0xC6, i * 0x2000 + 0x03) * 128;
      saveSize[i] = readBank_SNES(0xC6, i * 0x2000 + 0x05) / 8;

      //check if hirom
      if (readBank_SNES(gameAddress[i], 0xFFD5) == 0x31) {
        hirom[i] = true;
      }
      else if (readBank_SNES(gameAddress[i], 0xFFD5) == 0x21) {
        hirom[i] = true;
      }
      else {
        hirom[i] = false;
      }

      if (hirom[i]) {
        gameVersion[i] = readBank_SNES(gameAddress[i], 0xFFDB);
      } else {
        gameVersion[i] = readBank_SNES(gameAddress[i], 0x7FDB);
      }

      // Read game code
      byte myByte = 0;
      byte myLength = 0;
      for (int j = 0; j < 9; j++) {
        myByte = readBank_SNES(0xC6, i * 0x2000 + 0x07 + j);
        // Remove funny characters
        if (((char(myByte) >= 44 && char(myByte) <= 57) || (char(myByte) >= 65 && char(myByte) <= 122)) && myLength < 9) {
          gameCode[i][myLength] = char(myByte);
          myLength++;
        }
      }
      // End char array in case game code is less than 9 chars
      gameCode[i][myLength] = '\0';
    }
  }
  else {
    //check if hirom
    if (readBank_SNES(0xC0, 0xFFD5) == 0x31) {
      hirom[0] = true;
    }
    else {
      hirom[0] = false;
    }

    if (hirom[0]) {
      gameVersion[0] = readBank_SNES(0xC0, 0xFFDB);
      gameCode[0][0] = 'G';
      gameCode[0][1] = 'A';
      gameCode[0][2] = 'M';
      gameCode[0][3] = 'E';
      gameCode[0][4] = '-';
      gameCode[0][5] = char(readBank_SNES(0xC0, 0xFFB2));
      gameCode[0][6] = char(readBank_SNES(0xC0, 0xFFB3));
      gameCode[0][7] = char(readBank_SNES(0xC0, 0xFFB4));
      gameCode[0][8] = char(readBank_SNES(0xC0, 0xFFB5));
      gameCode[0][9] = '\0';

      byte romSizeExp = readBank_SNES(0xC0, 0xFFD7) - 7;
      gameSize[0] = 1;
      while (romSizeExp--)
        gameSize[0] *= 2;
    }
    else {
      gameVersion[0] = readBank_SNES(0xC0, 0x7FDB);
      gameCode[0][0] = 'G';
      gameCode[0][1] = 'A';
      gameCode[0][2] = 'M';
      gameCode[0][3] = 'E';
      gameCode[0][4] = '-';
      gameCode[0][5] = char(readBank_SNES(0xC0, 0x7FB2));
      gameCode[0][6] = char(readBank_SNES(0xC0, 0x7FB3));
      gameCode[0][7] = char(readBank_SNES(0xC0, 0x7FB4));
      gameCode[0][8] = char(readBank_SNES(0xC0, 0x7FB5));
      gameCode[0][9] = '\0';

      byte romSizeExp = readBank_SNES(0xC0, 0x7FD7) - 7;
      gameSize[0] = 1;
      while (romSizeExp--)
        gameSize[0] *= 2;
    }
  }
}

/******************************************
   Setup
 *****************************************/
void setup_SFM() {
  // Set cicrstPin(PG1) to Output
  DDRG |= (1 << 1);
  // Output a high signal to disable snesCIC if installed
  PORTG |= (1 << 1);
  // Set cichstPin(PG0) to Input
  DDRG &= ~(1 << 0);

  // Adafruit Clock Generator
  clockgen.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  clockgen.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  clockgen.set_pll(SI5351_PLL_FIXED, SI5351_PLLB);
  // Half Clock
  //clockgen.set_freq(1073863600ULL, SI5351_CLK0);
  // Full Clock
  clockgen.set_freq(2147727200ULL, SI5351_CLK0);
  // CIC Clock
  //clockgen.set_freq(307200000ULL, SI5351_CLK2);
  clockgen.output_enable(SI5351_CLK0, 1);
  clockgen.output_enable(SI5351_CLK1, 0);
  clockgen.output_enable(SI5351_CLK2, 0);

  // Set Address Pins to Output
  //A0-A7
  DDRF = 0xFF;
  //A8-A15
  DDRK = 0xFF;
  //BA0-BA7
  DDRL = 0xFF;

  // Set Control Pins to Output RST(PH0) CS(PH3) WR(PH5) RD(PH6)
  DDRH |= (1 << 0) | (1 << 3) | (1 << 5) | (1 << 6);
  // Output a high signal on all pins, pins are active low therefore everything is disabled now
  PORTH |= (1 << 0) | (1 << 3) | (1 << 5) | (1 << 6);

  // Set IRQ(PH4) to Input
  DDRH &= ~(1 << 4);
  // Activate Internal Pullup Resistors
  PORTH |= (1 << 4);

  // Set Data Pins (D0-D7) to Input
  DDRC = 0x00;
  // Enable Internal Pullups
  PORTC = 0xFF;

  // Wait until all is stable
  delay(200);

  // Switch to HiRom All
  byte timeout = 0;
  send_SFM(0x04);
  delay(100);
  while (readBank_SNES(0, 0x2400) != 0x2A) {
    delay(100);
    // Try again
    send_SFM(0x04);
    delay(100);
    timeout++;
    // Abort, something is wrong
    if (timeout == 5) {
      println_Msg(F("Hirom All Timeout"));
      println_Msg(F(""));
      println_Msg(F(""));
      print_Error(F("Powercycle SFM cart"), true);
    }
  }
}

/******************************************
   29F1601 flashrom functions (NP)
 *****************************************/
// Reset the MX29F1601 flashrom, startbank is 0xC0 for first and 0xE0 for second flashrom
void resetFlash_SFM(int startBank) {
  // Configure control pins
  controlOut_SNES();
  // Set data pins to output
  dataOut();

  // Reset command sequence
  if (romType) {
    writeBank_SNES(startBank, 0x5555L * 2, 0xaa);
    writeBank_SNES(startBank, 0x2AAAL * 2, 0x55);
    writeBank_SNES(startBank, 0x5555L * 2, 0xf0);
  }
  else {
    writeBank_SNES(1, 0x8000 + 0x1555L * 2, 0xaa);
    writeBank_SNES(0, 0x8000 + 0x2AAAL * 2, 0x55);
    writeBank_SNES(1, 0x8000 + 0x1555L * 2, 0xf0);
  }
}

// Print flashrom manufacturer and device ID
void idFlash_SFM(int startBank) {
  // Configure control pins
  controlOut_SNES();
  // Set data pins to output
  dataOut();

  if (romType) {
    // ID command sequence
    writeBank_SNES(startBank, 0x5555L * 2, 0xaa);
    writeBank_SNES(startBank, 0x2AAAL * 2, 0x55);
    writeBank_SNES(startBank, 0x5555L * 2, 0x90);

    // Set data pins to input again
    dataIn();
    // Set control pins to input
    controlIn_SNES();

    // Read the two id bytes into a string
    sprintf(flashid, "%x%x", readBank_SNES(startBank, 0x00), readBank_SNES(startBank, 0x02));
  }
  else {
    writeBank_SNES(1, 0x8000 + 0x1555L * 2, 0xaa);
    writeBank_SNES(0, 0x8000 + 0x2AAAL * 2, 0x55);
    writeBank_SNES(1, 0x8000 + 0x1555L * 2, 0x90);

    // Set data pins to input again
    dataIn();
    // Set control pins to input
    controlIn_SNES();

    // Read the two id bytes into a string
    sprintf(flashid, "%x%x", readBank_SNES(0, 0x8000), readBank_SNES(0, 0x8000 + 0x02));
  }
}

// Write the flashroms by reading a file from the SD card, pos defines where in the file the reading/writing should start
void writeFlash_SFM(int startBank, uint32_t pos) {
  display_Clear();
  print_Msg(F("Writing Bank 0x"));
  print_Msg(startBank, HEX);
  print_Msg(F("..."));
  display_Update();

  // Open file on sd card
  if (myFile.open(filePath, O_READ)) {

    // Seek to a new position in the file
    if (pos != 0)
      myFile.seekCur(pos);

    // Configure control pins
    controlOut_SNES();
    // Set data pins to output
    dataOut();

    if (romType) {
      // Write hirom
      for (int currBank = startBank; currBank < startBank + numBanks; currBank++) {
        // Fill SDBuffer with 1 page at a time then write it repeat until all bytes are written
        for (unsigned long currByte = 0; currByte < 0x10000; currByte += 128) {
          myFile.read(sdBuffer, 128);
          // Write command sequence
          writeBank_SNES(startBank, 0x5555L * 2, 0xaa);
          writeBank_SNES(startBank, 0x2AAAL * 2, 0x55);
          writeBank_SNES(startBank, 0x5555L * 2, 0xa0);

          for (byte c = 0; c < 128; c++) {

            // Write one byte of data
            writeBank_SNES(currBank, currByte + c, sdBuffer[c]);

            if (c == 127) {
              // Write the last byte twice or else it won't write at all
              writeBank_SNES(currBank, currByte + c, sdBuffer[c]);
            }
          }
          // Wait until write is finished
          busyCheck_SFM(startBank);
        }
      }
    }
    else {
      // Write lorom
      for (int currBank = 0; currBank < numBanks; currBank++) {
        for (unsigned long currByte = 0x8000; currByte < 0x10000; currByte += 128) {
          myFile.read(sdBuffer, 128);
          // Write command sequence
          writeBank_SNES(1, 0x8000 + 0x1555L * 2, 0xaa);
          writeBank_SNES(0, 0x8000 + 0x2AAAL * 2, 0x55);
          writeBank_SNES(1, 0x8000 + 0x1555L * 2, 0xa0);

          for (byte c = 0; c < 128; c++) {
            // Write one byte of data
            writeBank_SNES(currBank, currByte + c, sdBuffer[c]);

            if (c == 127) {
              // Write the last byte twice or else it won't write at all
              writeBank_SNES(currBank, currByte + c, sdBuffer[c]);
            }
          }
          // Wait until write is finished
          busyCheck_SFM(startBank);
        }
      }
    }
    // Close the file:
    myFile.close();
    println_Msg("");
  }
  else {
    print_Error(F("Can't open file on SD"), true);
  }
}

// Delay between write operations based on status register
void busyCheck_SFM(byte startBank) {
  // Set data pins to input
  dataIn();
  // Set control pins to input and therefore pull CE low and latch status register content
  controlIn_SNES();

  // Read register
  readBank_SNES(startBank, 0x0000);

  // Read D7 while D7 = 0
  //1 = B00000001, 1 << 7 = B10000000, PINC = B1XXXXXXX (X = don't care), & = bitwise and
  while (!(PINC & (1 << 7))) {
    // CE or OE must be toggled with each subsequent status read or the
    // completion of a program or erase operation will not be evident.
    // Switch RD(PH6) to HIGH
    PORTH |= (1 << 6);

    // one nop ~62.5ns
    __asm__("nop\n\t");

    // Switch RD(PH6) to LOW
    PORTH &= ~(1 << 6);

    // one nop ~62.5ns
    __asm__("nop\n\t");
  }

  // Configure control pins
  controlOut_SNES();
  // Set data pins to output
  dataOut();
}

// Erase the flashrom to 0xFF
void eraseFlash_SFM(int startBank) {

  // Configure control pins
  controlOut_SNES();
  // Set data pins to output
  dataOut();

  if (romType) {
    // Erase command sequence
    writeBank_SNES(startBank, 0x5555L * 2, 0xaa);
    writeBank_SNES(startBank, 0x2AAAL * 2, 0x55);
    writeBank_SNES(startBank, 0x5555L * 2, 0x80);
    writeBank_SNES(startBank, 0x5555L * 2, 0xaa);
    writeBank_SNES(startBank, 0x2AAAL * 2, 0x55);
    writeBank_SNES(startBank, 0x5555L * 2, 0x10);
  }
  else {
    writeBank_SNES(1, 0x8000 + 0x1555L * 2, 0xaa);
    writeBank_SNES(0, 0x8000 + 0x2AAAL * 2, 0x55);
    writeBank_SNES(1, 0x8000 + 0x1555L * 2, 0x80);
    writeBank_SNES(1, 0x8000 + 0x1555L * 2, 0xaa);
    writeBank_SNES(0, 0x8000 + 0x2AAAL * 2, 0x55);
    writeBank_SNES(1, 0x8000 + 0x1555L * 2, 0x10);
  }

  // Wait for erase to complete
  busyCheck_SFM(startBank);
}

// Check if an erase succeeded, return 1 if blank and 0 if not
byte blankcheck_SFM(int startBank) {

  // Set data pins to input again
  dataIn();
  // Set control pins to input
  controlIn_SNES();

  byte blank = 1;
  if (romType) {
    for (int currBank = startBank; currBank < startBank + numBanks; currBank++) {
      for (unsigned long currByte = 0; currByte < 0x10000; currByte++) {
        if (readBank_SNES(currBank, currByte) != 0xFF) {
          currBank =  startBank + numBanks;
          blank = 0;
        }
      }
    }
  }
  else {
    for (int currBank = 0; currBank < numBanks; currBank++) {
      for (unsigned long currByte = 0x8000; currByte < 0x10000; currByte++) {
        if (readBank_SNES(currBank, currByte) != 0xFF) {
          currBank = numBanks;
          blank = 0;
        }
      }
    }
  }
  return blank;
}

// Check if a write succeeded, returns 0 if all is ok and number of errors if not
unsigned long verifyFlash_SFM(int startBank, uint32_t pos) {
  unsigned long  verified = 0;

  // Open file on sd card
  if (myFile.open(filePath, O_READ)) {

    // Set file starting position
    myFile.seekCur(pos);

    // Set data pins to input
    dataIn();
    // Set control pins to input
    controlIn_SNES();

    if (romType) {
      for (int currBank = startBank; currBank < startBank + numBanks; currBank++) {
        for (unsigned long currByte = 0; currByte < 0x10000; currByte += 512) {
          // Fill SDBuffer
          myFile.read(sdBuffer, 512);
          for (int c = 0; c < 512; c++) {
            if (readBank_SNES(currBank, currByte + c) != sdBuffer[c]) {
              verified++;
            }
          }
        }
      }
    }
    else {
      for (int currBank = 0; currBank < numBanks; currBank++) {
        for (unsigned long currByte = 0x8000; currByte < 0x10000; currByte += 512) {
          // Fill SDBuffer
          myFile.read(sdBuffer, 512);
          for (int c = 0; c < 512; c++) {
            if (readBank_SNES(currBank, currByte + c) != sdBuffer[c]) {
              verified++;
            }
          }
        }
      }
    }
    // Close the file:
    myFile.close();
  }
  else {
    // SD Error
    verified = 999999;
    print_Error(F("Can't open file on SD"), false);
  }
  // Return 0 if verified ok, or number of errors
  return verified;
}

// Read flashroms and save them to the SD card
void readFlash_SFM() {
  // Set data pins to input
  dataIn();
  // Set control pins to input
  controlIn_SNES();

  print_Msg(F("Saving as NP/"));
  print_Msg(fileName);
  println_Msg(F("..."));
  display_Update();

  // Open file on sd card
  if (!myFile.open(fileName, O_RDWR | O_CREAT)) {
    print_Error(F("Can't create file on SD"), true);
  }
  if (romType) {
    for (int currBank = 0xC0; currBank < 0xC0 + numBanks; currBank++) {
      for (unsigned long currByte = 0; currByte < 0x10000; currByte += 512) {
        for (int c = 0; c < 512; c++) {
          sdBuffer[c] = readBank_SNES(currBank, currByte + c);
        }
        myFile.write(sdBuffer, 512);
      }
    }
  }
  else {
    for (int currBank = 0; currBank < numBanks; currBank++) {
      for (unsigned long currByte = 0x8000; currByte < 0x10000; currByte += 512) {
        for (int c = 0; c < 512; c++) {
          sdBuffer[c] = readBank_SNES(currBank, currByte + c);
        }
        myFile.write(sdBuffer, 512);
      }
    }
  }
  // Close the file:
  myFile.close();
  println_Msg("");
  println_Msg(F("Finished reading"));
  display_Update();
}

// Display protected sectors/banks as 0xc2 and unprotected as 0x00
void readSectorProtection_SFM(byte startBank) {

  // Configure control pins
  controlOut_SNES();
  // Set data pins to output
  dataOut();

  // Display Sector Protection Status
  writeBank_SNES(startBank, 0x5555L * 2, 0xaa);
  writeBank_SNES(startBank, 0x2AAAL * 2, 0x55);
  writeBank_SNES(startBank, 0x5555L * 2, 0x90);

  // Configure control pins
  controlIn_SNES();
  // Set data pins to output
  dataIn();
  display_Clear();
  for (int i = 0; i <= 0x1F; i++) {
    print_Msg(F("Sector: 0x"));
    print_Msg(startBank + i, HEX);
    print_Msg(F(" Sector Protect: 0x"));
    println_Msg(readBank_SNES(startBank + i, 0x04), HEX);
  }
  display_Update();
}

// Read the current mapping from the hidden "page buffer" and print it
void printMapping() {
  // Switch to write
  dataOut();
  controlOut_SNES();

  // Reset to defaults
  writeBank_SNES(0xC0, 0x0000, 0x38);
  writeBank_SNES(0xC0, 0x0000, 0xd0);
  // Read Extended Status Register (GSR and PSR)
  writeBank_SNES(0xC0, 0x0000, 0x71);
  // Page Buffer Swap
  writeBank_SNES(0xC0, 0x0000, 0x72);
  // Read Page Buffer
  writeBank_SNES(0xC0, 0x0000, 0x75);

  // Switch to read
  dataIn();
  controlIn_SNES();

  // Read the mapping out of the first chip
  char buffer[3];

  for (int currByte = 0xFF00; currByte < 0xFF50; currByte += 10) {
    for (int c = 0; c < 10; c++) {
      itoa (readBank_SNES(0xC0, currByte + c), buffer, 16);
      for (int i = 0; i < 2 - strlen(buffer); i++) {
        print_Msg("0");
      }
      // Now print the significant bits
      print_Msg(buffer);
    }
    println_Msg("");
  }
  display_Update();

  // Switch to write
  dataOut();
  controlOut_SNES();

  // Reset Flash
  writeBank_SNES(0xC0, 0x5555L * 2, 0xaa);
  writeBank_SNES(0xC0, 0x2AAAL * 2, 0x55);
  writeBank_SNES(0xC0, 0x5555L * 2, 0xf0);

  // Reset Flash
  writeBank_SNES(0xE0, 0x5555L * 2, 0xaa);
  writeBank_SNES(0xE0, 0x2AAAL * 2, 0x55);
  writeBank_SNES(0xE0, 0x5555L * 2, 0xf0);

  // Switch to read
  dataIn();
  controlIn_SNES();
}

// Read the current mapping from the hidden "page buffer"
void readMapping() {

  // Switch to write
  dataOut();
  controlOut_SNES();

  // Reset to defaults
  writeBank_SNES(0xC0, 0x0000, 0x38);
  writeBank_SNES(0xC0, 0x0000, 0xd0);
  // Read Extended Status Register (GSR and PSR)
  writeBank_SNES(0xC0, 0x0000, 0x71);
  // Page Buffer Swap
  writeBank_SNES(0xC0, 0x0000, 0x72);
  // Read Page Buffer
  writeBank_SNES(0xC0, 0x0000, 0x75);

  // Switch to read
  dataIn();
  controlIn_SNES();

  // Get name, add extension and convert to char array for sd lib
  EEPROM_readAnything(10, foldern);
  sprintf(fileName, "NP%d", foldern);
  strcat(fileName, ".MAP");
  sd.mkdir("NP", true);
  sd.chdir("NP");

  // write new folder number back to eeprom
  foldern = foldern + 1;
  EEPROM_writeAnything(10, foldern);

  //open file on sd card
  if (!myFile.open(fileName, O_RDWR | O_CREAT)) {
    print_Error(F("SD Error"), true);
  }

  // Read the mapping info out of the 1st chip
  for (unsigned long currByte = 0xFF00; currByte <= 0xFFFF; currByte++) {
    myFile.write(readBank_SNES(0xC0, currByte));
  }

  // Switch to write
  dataOut();
  controlOut_SNES();

  // Reset to defaults
  writeBank_SNES(0xE0, 0x0000, 0x38);
  writeBank_SNES(0xE0, 0x0000, 0xd0);
  // Read Extended Status Register (GSR and PSR)
  writeBank_SNES(0xE0, 0x0000, 0x71);
  // Page Buffer Swap
  writeBank_SNES(0xE0, 0x0000, 0x72);
  // Read Page Buffer
  writeBank_SNES(0xE0, 0x0000, 0x75);

  // Switch to read
  dataIn();
  controlIn_SNES();

  // Read the mapping info out of the 1st chip
  for (unsigned long currByte = 0xFF00; currByte <= 0xFFFF; currByte++) {
    myFile.write(readBank_SNES(0xE0, currByte));
  }

  // Close the file:
  myFile.close();

  // Switch to write
  dataOut();
  controlOut_SNES();

  // Reset Flash
  writeBank_SNES(0xC0, 0x5555L * 2, 0xaa);
  writeBank_SNES(0xC0, 0x2AAAL * 2, 0x55);
  writeBank_SNES(0xC0, 0x5555L * 2, 0xf0);

  // Reset Flash
  writeBank_SNES(0xE0, 0x5555L * 2, 0xaa);
  writeBank_SNES(0xE0, 0x2AAAL * 2, 0x55);
  writeBank_SNES(0xE0, 0x5555L * 2, 0xf0);

  // Switch to read
  dataIn();
  controlIn_SNES();

  // Signal end of process
  print_Msg(F("Saved to NP/"));
  println_Msg(fileName);
  display_Update();
}

void eraseMapping(byte startBank) {

  if (unlockHirom()) {
    // Get ID
    idFlash_SFM(startBank);
    if (strcmp(flashid, "c2f3") == 0) {
      resetFlash_SFM(startBank);

      // Switch to write
      dataOut();
      controlOut_SNES();

      // Prepare to erase/write Page Buffer
      writeBank_SNES(startBank, 0x5555L * 2, 0xaa);
      writeBank_SNES(startBank, 0x2AAAL * 2, 0x55);
      writeBank_SNES(startBank, 0x5555L * 2, 0x77);
      // Erase Page Buffer
      writeBank_SNES(startBank, 0x5555L * 2, 0xaa);
      writeBank_SNES(startBank, 0x2AAAL * 2, 0x55);
      writeBank_SNES(startBank, 0x5555L * 2, 0xe0);

      // Wait until complete
      busyCheck_SFM(startBank);

      // Switch to read
      dataIn();
      controlIn_SNES();
    }
    else {
      print_Error(F("Error: Wrong Flash ID"), true);
    }
  }
  else {
    print_Error(F("Unlock failed"), true);
  }
}

// Check if the current mapping is all 0xFF
byte blankcheckMapping_SFM() {
  byte blank = 1;

  // Switch to write
  dataOut();
  controlOut_SNES();

  // Reset to defaults
  writeBank_SNES(0xC0, 0x0000, 0x38);
  writeBank_SNES(0xC0, 0x0000, 0xd0);
  // Read Extended Status Register (GSR and PSR)
  writeBank_SNES(0xC0, 0x0000, 0x71);
  // Page Buffer Swap
  writeBank_SNES(0xC0, 0x0000, 0x72);
  // Read Page Buffer
  writeBank_SNES(0xC0, 0x0000, 0x75);

  // Switch to read
  dataIn();
  controlIn_SNES();

  // Read the mapping info out of the 1st chip
  for (unsigned long currByte = 0xFF00; currByte <= 0xFFFF; currByte++) {
    if (readBank_SNES(0xC0, currByte) != 0xFF) {
      blank = 0;
    }
  }

  // Switch to write
  dataOut();
  controlOut_SNES();

  // Reset to defaults
  writeBank_SNES(0xE0, 0x0000, 0x38);
  writeBank_SNES(0xE0, 0x0000, 0xd0);
  // Read Extended Status Register (GSR and PSR)
  writeBank_SNES(0xE0, 0x0000, 0x71);
  // Page Buffer Swap
  writeBank_SNES(0xE0, 0x0000, 0x72);
  // Read Page Buffer
  writeBank_SNES(0xE0, 0x0000, 0x75);

  // Switch to read
  dataIn();
  controlIn_SNES();

  // Read the mapping info out of the 1st chip
  for (unsigned long currByte = 0xFF00; currByte <= 0xFFFF; currByte++) {
    if (readBank_SNES(0xE0, currByte) != 0xFF) {
      blank = 0;
    }
  }

  // Switch to write
  dataOut();
  controlOut_SNES();

  // Reset Flash
  writeBank_SNES(0xC0, 0x5555L * 2, 0xaa);
  writeBank_SNES(0xC0, 0x2AAAL * 2, 0x55);
  writeBank_SNES(0xC0, 0x5555L * 2, 0xf0);

  // Reset Flash
  writeBank_SNES(0xE0, 0x5555L * 2, 0xaa);
  writeBank_SNES(0xE0, 0x2AAAL * 2, 0x55);
  writeBank_SNES(0xE0, 0x5555L * 2, 0xf0);

  // Switch to read
  dataIn();
  controlIn_SNES();

  return blank;
}

void writeMapping_SFM(byte startBank, uint32_t pos) {

  if (unlockHirom()) {
    // Get ID
    idFlash_SFM(startBank);
    if (strcmp(flashid, "c2f3") == 0) {
      resetFlash_SFM(startBank);

      // Switch to write
      dataOut();
      controlOut_SNES();

      // Open file on sd card
      if (myFile.open(filePath, O_READ)) {

        // Seek to a new position in the file
        if (pos != 0)
          myFile.seekCur(pos);

        // Write to Page Buffer
        for (unsigned long currByte = 0xFF00; currByte < 0xFFFF; currByte += 128) {
          // Prepare to erase/write Page Buffer
          writeBank_SNES(startBank, 0x5555L * 2, 0xaa);
          writeBank_SNES(startBank, 0x2AAAL * 2, 0x55);
          writeBank_SNES(startBank, 0x5555L * 2, 0x77);
          // Write Page Buffer Command
          writeBank_SNES(startBank, 0x5555L * 2, 0xaa);
          writeBank_SNES(startBank, 0x2AAAL * 2, 0x55);
          writeBank_SNES(startBank, 0x5555L * 2, 0x99);

          myFile.read(sdBuffer, 128);

          for (byte c = 0; c < 128; c++) {
            writeBank_SNES(startBank, currByte + c, sdBuffer[c]);
            // Write last byte twice
            if (c == 127) {
              writeBank_SNES(startBank, currByte + c, sdBuffer[c]);
            }
          }
          busyCheck_SFM(startBank);
        }

        // Close the file:
        myFile.close();
        println_Msg("");
      }
      else {
        print_Error(F("Can't open file on SD"), false);
      }

      // Switch to read
      dataIn();
      controlIn_SNES();
    }
    else {
      print_Error(F("Error: Wrong Flash ID"), true);
    }
  }
  else {
    print_Error(F("Unlock failed"), true);
  }
}

/******************************************
  SF Memory functions
*****************************************/
// Switch to HiRom All and unlock Write Protection
boolean unlockHirom() {
  romType = 1;
  print_Msg(F("Switch to HiRom..."));
  display_Update();
  if (send_SFM(0x04) == 0x2A) {
    println_Msg(F("OK"));
    display_Update();
    // Unlock Write Protection
    print_Msg(F("Enable Write..."));
    display_Update();
    send_SFM(0x02);
    if (readBank_SNES(0, 0x2401) == 0x4) {
      println_Msg(F("OK"));
      display_Update();
      return 1;
    }
    else {
      println_Msg(F("failed"));
      display_Update();
      return 0;
    }
  }
  else {
    println_Msg(F("failed"));
    display_Update();
    return 0;
  }
}

// Send a command to the MX15001 chip
byte send_SFM(byte command) {
  // Switch to write
  dataOut();
  controlOut_SNES();

  // Write command
  writeBank_SNES(0, 0x2400, 0x09);

  // Switch to read
  dataIn();
  controlIn_SNES();

  // Read status
  sfmReady = readBank_SNES(0, 0x2400);

  // Switch to write
  dataOut();
  controlOut_SNES();

  writeBank_SNES(0, 0x2401, 0x28);
  writeBank_SNES(0, 0x2401, 0x84);

  // NP_CMD_06h, send this only if above read has returned 7Dh, not if it's already returning 2Ah
  if (sfmReady == 0x7D) {
    writeBank_SNES(0, 0x2400, 0x06);
    writeBank_SNES(0, 0x2400, 0x39);
  }

  // Write the command
  writeBank_SNES(0, 0x2400, command);

  // Switch to read
  dataIn();
  controlIn_SNES();

  // Read status
  sfmReady = readBank_SNES(0, 0x2400);
  return sfmReady;
}

// This function will erase and program the NP cart from a 4MB file off the SD card
void write_SFM(int startBank, uint32_t pos) {
  // Switch NP cart's mapping
  if (unlockHirom()) {
    // Get ID
    idFlash_SFM(startBank);
    if (strcmp(flashid, "c2f3") == 0) {
      print_Msg(F("Flash ID: "));
      println_Msg(flashid);
      display_Update();
      resetFlash_SFM(startBank);
      delay(1000);
      // Erase flash
      print_Msg(F("Blankcheck..."));
      display_Update();
      if (blankcheck_SFM(startBank)) {
        println_Msg(F("OK"));
        display_Update();
      }
      else {
        println_Msg(F("Nope"));
        display_Clear();
        print_Msg(F("Erasing..."));
        display_Update();
        eraseFlash_SFM(startBank);
        resetFlash_SFM(startBank);
        println_Msg(F("Done"));
        print_Msg(F("Blankcheck..."));
        display_Update();
        if (blankcheck_SFM(startBank)) {
          println_Msg(F("OK"));
          display_Update();
        }
        else {
          print_Error(F("Could not erase flash"), true);
        }
      }
      // Write flash
      writeFlash_SFM(startBank, pos);

      // Reset flash
      resetFlash_SFM(startBank);

      // Checking for errors
      print_Msg(F("Verifying..."));
      display_Update();
      writeErrors = verifyFlash_SFM(startBank, pos);
      if (writeErrors == 0) {
        println_Msg(F("OK"));
        display_Update();
      }
      else {
        print_Msg(F("Error: "));
        print_Msg(writeErrors);
        println_Msg(F(" bytes "));
        print_Error(F("did not verify."), true);
      }
    }
    else {
      print_Error(F("Error: Wrong Flash ID"), true);
    }
  }
  else {
    print_Error(F("Unlock failed"), true);
  }
}

/******************************************
  GB Memory Cassette
******************************************/

/******************************************
  Menu
*****************************************/
// GBM menu items
static const char gbmMenuItem1[] PROGMEM = "Read ID";
static const char gbmMenuItem2[] PROGMEM = "Read Flash";
static const char gbmMenuItem3[] PROGMEM = "Erase Flash";
static const char gbmMenuItem4[] PROGMEM = "Blankcheck";
static const char gbmMenuItem5[] PROGMEM = "Write Flash";
static const char gbmMenuItem6[] PROGMEM = "Read Mapping";
static const char gbmMenuItem7[] PROGMEM = "Write Mapping";
static const char* const menuOptionsGBM[] PROGMEM = {gbmMenuItem1, gbmMenuItem2, gbmMenuItem3, gbmMenuItem4, gbmMenuItem5, gbmMenuItem6, gbmMenuItem7};

void gbmMenu() {
  // create menu with title and 7 options to choose from
  unsigned char mainMenu;
  // Copy menuOptions out of progmem
  convertPgm(menuOptionsGBM, 6);
  mainMenu = question_box("GB Memory Menu", menuOptions, 6, 0);

  // wait for user choice to come back from the question box menu
  switch (mainMenu)
  {
    // Read Flash ID
    case 0:
      // Clear screen
      display_Clear();
      readFlashID_GBM();
      break;

    // Read Flash
    case 1:
      // Clear screen
      display_Clear();
      // Print warning
      println_Msg(F("Attention"));
      println_Msg(F("Always power cycle"));
      println_Msg(F("cartreader directly"));
      println_Msg(F("before reading"));
      println_Msg("");
      println_Msg(F("Press Button"));
      println_Msg(F("to continue"));
      display_Update();
      wait();
      // Clear screen
      display_Clear();

      // Reset to root directory
      sd.chdir("/");

      // Enable access to ports 0120h
      send_GBM(0x09);
      // Map entire flashrom
      send_GBM(0x04);
      // Disable ports 0x0120...
      send_GBM(0x08);
      // Read 1MB rom
      readROM_GBM(64);
      break;

    // Erase Flash
    case 2:
      // Clear screen
      display_Clear();
      // Print warning
      println_Msg(F("Attention"));
      println_Msg(F("This will erase your"));
      println_Msg(F("NP Cartridge."));
      println_Msg("");
      println_Msg("");
      println_Msg(F("Press Button"));
      println_Msg(F("to continue"));
      display_Update();
      wait();
      // Clear screen
      display_Clear();
      eraseFlash_GBM();
      break;

    // Blankcheck Flash
    case 3:
      // Clear screen
      display_Clear();
      if (blankcheckFlash_GBM()) {
        println_Msg(F("OK"));
        display_Update();
      }
      else {
        println_Msg(F("ERROR"));
        display_Update();
      }
      break;

    // Write Flash
    case 4:
      // Clear screen
      display_Clear();

      filePath[0] = '\0';
      sd.chdir("/");
      // Launch file browser
      fileBrowser("Select 1MB file");
      display_Clear();
      sprintf(filePath, "%s/%s", filePath, fileName);

      // Write rom
      writeFlash_GBM();
      break;

    // Read mapping
    case 5:
      // Clear screen
      display_Clear();

      // Reset to root directory
      sd.chdir("/");

      // Read mapping
      readMapping_GBM();
      break;

    // Write mapping
    case 6:
      // Clear screen
      display_Clear();

      // Print warning
      println_Msg(F("Attention"));
      println_Msg(F("This will erase your"));
      println_Msg(F("NP Cartridge."));
      println_Msg("");
      println_Msg(F("Press Button"));
      println_Msg(F("to continue"));
      display_Update();
      wait();

      // Clear screen
      display_Clear();

      // Reset to root directory
      sd.chdir("/");

      // Erase mapping
      eraseMapping_GBM();
      print_Msg(F("Blankcheck..."));
      display_Update();
      if (blankcheckMapping_GBM()) {
        println_Msg(F("OK"));
        display_Update();
      }
      else {
        println_Msg(F("Nope"));
        break;
      }

      // Clear screen
      display_Clear();

      // Clear filepath
      filePath[0] = '\0';

      // Reset to root directory
      sd.chdir("/");

      // Launch file browser
      fileBrowser("Select MAP file");
      display_Clear();
      sprintf(filePath, "%s/%s", filePath, fileName);
      display_Update();

      // Write mapping
      writeMapping_GBM();
      break;
  }
  println_Msg(F(""));
  println_Msg(F("Press Button..."));
  display_Update();
  wait();
}

/******************************************
  Setup
*****************************************/
void setup_GBM() {
  // Set RST(PH0) to Input
  DDRH &= ~(1 << 0);
  // Activate Internal Pullup Resistors
  PORTH |= (1 << 0);

  // Set Address Pins to Output
  //A0-A7
  DDRF = 0xFF;
  //A8-A15
  DDRK = 0xFF;

  // Set Control Pins to Output RST(PH0) CS(PH3) WR(PH5) RD(PH6)
  DDRH |= (1 << 3) | (1 << 5) | (1 << 6);
  // Output a high signal on all pins, pins are active low therefore everything is disabled now
  PORTH |= (1 << 3) | (1 << 5) | (1 << 6);

  // Set Data Pins (D0-D7) to Input
  DDRC = 0x00;

  delay(400);

  // Check for Nintendo Power GB Memory cart
  byte timeout = 0;

  // First byte of NP register is always 0x21
  while (readByte_GBM(0x120) != 0x21) {
    // Enable ports 0x120h (F2)
    send_GBM(0x09);
    __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
    timeout++;
    if (timeout > 10) {
      println_Msg(F("Error: Time Out"));
      print_Error(F("Please power cycle"), true);
    }
  }
}

/**********************
  LOW LEVEL
**********************/
// Read one word out of the cartridge
byte readByte_GBM(word myAddress) {
  // Set data pins to Input
  DDRC = 0x0;
  PORTF = myAddress & 0xFF;
  PORTK = (myAddress >> 8) & 0xFF;

  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  // Switch CS(PH3) and RD(PH6) to LOW
  PORTH &= ~(1 << 3);
  PORTH &= ~(1 << 6);

  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  // Read
  byte tempByte = PINC;

  // Switch CS(PH3) and RD(PH6) to HIGH
  PORTH |= (1 << 6);
  PORTH |= (1 << 3);

  return tempByte;
}

// Write one word to data pins of the cartridge
void writeByte_GBM(word myAddress, byte myData) {
  // Set data pins to Output
  DDRC = 0xFF;
  PORTF = myAddress & 0xFF;
  PORTK = (myAddress >> 8) & 0xFF;
  PORTC = myData;

  // Pull CS(PH3) and write(PH5) low
  PORTH &= ~(1 << 3);
  PORTH &= ~(1 << 5);

  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  // Pull CS(PH3) and write(PH5) high
  PORTH |= (1 << 5);
  PORTH |= (1 << 3);

  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  // Set data pins to Input (or read errors??!)
  DDRC = 0x0;
}

/**********************
  HELPER FUNCTIONS
**********************/
void printSdBuffer(word startByte, word numBytes) {
  for (int currByte = 0; currByte < numBytes; currByte += 10) {
    for (byte c = 0; c < 10; c++) {
      // Convert to char array so we don't lose leading zeros
      char currByteStr[2];
      sprintf(currByteStr, "%02X", sdBuffer[startByte + currByte + c]);
      print_Msg(currByteStr);
    }
    // Add a new line every 10 bytes
    println_Msg("");
  }
  display_Update();
}

void readROM_GBM(word numBanks) {
  println_Msg(F("Reading Rom..."));
  display_Update();

  // Get name, add extension and convert to char array for sd lib
  EEPROM_readAnything(10, foldern);
  sprintf(fileName, "GBM%d", foldern);
  strcat(fileName, ".bin");
  sd.mkdir("NP", true);
  sd.chdir("NP");
  // write new folder number back to eeprom
  foldern = foldern + 1;
  EEPROM_writeAnything(10, foldern);

  // Open file on sd card
  if (!myFile.open(fileName, O_RDWR | O_CREAT)) {
    print_Error(F("Can't create file on SD"), true);
  }
  else {
    // Read rom
    word currAddress = 0;

    for (word currBank = 1; currBank < numBanks; currBank++) {
      // Set rom bank
      writeByte_GBM(0x2100, currBank);

      // Switch bank start address
      if (currBank > 1) {
        currAddress = 0x4000;
      }

      for (; currAddress < 0x7FFF; currAddress += 512) {
        for (int currByte = 0; currByte < 512; currByte++) {
          sdBuffer[currByte] = readByte_GBM(currAddress + currByte);
        }
        myFile.write(sdBuffer, 512);
      }
    }

    // Close the file:
    myFile.close();

    // Signal end of process
    print_Msg(F("Saved to NP/"));
    println_Msg(fileName);
    display_Update();
  }
}

/**********************
  GB Memory Functions
**********************/
void send_GBM(byte myCommand) {
  switch (myCommand) {
    case 0x01:
      //CMD_01h -> ???
      writeByte_GBM(0x0120, 0x01);
      writeByte_GBM(0x013F, 0xA5);
      break;

    case 0x02:
      //CMD_02h -> Write enable Step 2
      writeByte_GBM(0x0120, 0x02);
      writeByte_GBM(0x013F, 0xA5);
      break;

    case 0x03:
      //CMD_03h -> Undo write Step 2
      writeByte_GBM(0x0120, 0x03);
      writeByte_GBM(0x013F, 0xA5);
      break;

    case 0x04:
      //CMD_04h -> Map entire flashrom (MBC4 mode)
      writeByte_GBM(0x0120, 0x04);
      writeByte_GBM(0x013F, 0xA5);
      break;

    case 0x05:
      //CMD_05h -> Map menu (MBC5 mode)
      writeByte_GBM(0x0120, 0x05);
      writeByte_GBM(0x013F, 0xA5);
      break;

    case 0x08:
      //CMD_08h -> disable writes/reads to/from special Nintendo Power registers (those at 0120h..013Fh)
      writeByte_GBM(0x0120, 0x08);
      writeByte_GBM(0x013F, 0xA5);
      break;

    case 0x09:
      //CMD_09h Wakeup -> re-enable access to ports 0120h..013Fh
      writeByte_GBM(0x0120, 0x09);
      writeByte_GBM(0x0121, 0xAA);
      writeByte_GBM(0x0122, 0x55);
      writeByte_GBM(0x013F, 0xA5);
      break;

    case 0x0A:
      //CMD_0Ah -> Write enable Step 1
      writeByte_GBM(0x0120, 0x0A);
      writeByte_GBM(0x0125, 0x62);
      writeByte_GBM(0x0126, 0x04);
      writeByte_GBM(0x013F, 0xA5);
      break;

    case 0x10:
      //CMD_10h -> disable writes to normal MBC registers (such like 2100h)
      writeByte_GBM(0x0120, 0x10);
      writeByte_GBM(0x013F, 0xA5);
      break;

    case 0x11:
      //CMD_11h -> re-enable access to MBC registers like 0x2100
      writeByte_GBM(0x0120, 0x11);
      writeByte_GBM(0x013F, 0xA5);
      break;

    default:
      print_Error(F("Unknown Command"), true);
      break;
  }
}

void send_GBM(byte myCommand, word myAddress, byte myData) {
  byte myAddrLow = myAddress & 0xFF;
  byte myAddrHigh = (myAddress >> 8) & 0xFF;

  switch (myCommand) {
    case 0x0F:
      // CMD_0Fh -> Write address/byte to flash
      writeByte_GBM(0x0120, 0x0F);
      writeByte_GBM(0x0125, myAddrHigh);
      writeByte_GBM(0x0126, myAddrLow);
      writeByte_GBM(0x0127, myData);
      writeByte_GBM(0x013F, 0xA5);
      break;

    default:
      print_Error(F("Unknown Command"), true);
      break;
  }
}

void switchGame_GBM(byte myData) {
  // Enable ports 0x0120 (F2)
  send_GBM(0x09);

  //CMD_C0h -> map selected game without reset
  writeByte_GBM(0x0120, 0xC0 & myData);
  writeByte_GBM(0x013F, 0xA5);
}

void resetFlash_GBM() {
  // Enable ports 0x0120 (F2)
  send_GBM(0x09);

  // Send reset command
  writeByte_GBM(0x2100, 0x01);
  send_GBM(0x0F, 0x5555, 0xAA);
  send_GBM(0x0F, 0x2AAA, 0x55);
  send_GBM(0x0F, 0x5555, 0xF0);
  delay(100);
}

boolean readFlashID_GBM() {
  // Enable ports 0x0120 (F2)
  send_GBM(0x09);

  writeByte_GBM(0x2100, 0x01);
  // Read ID command
  send_GBM(0x0F, 0x5555, 0xAA);
  send_GBM(0x0F, 0x2AAA, 0x55);
  send_GBM(0x0F, 0x5555, 0x90);

  // Read the two id bytes into a string
  sprintf(flashid, "%02X%02X", readByte_GBM(0), readByte_GBM(1));
  if (strcmp(flashid, "C289") == 0) {
    print_Msg(F("Flash ID: "));
    println_Msg(flashid);
    display_Update();
    resetFlash_GBM();
    return 1;
  }
  else {
    print_Msg(F("Flash ID: "));
    println_Msg(flashid);
    print_Error(F("Unknown Flash ID"), true);
    resetFlash_GBM();
    return 0;
  }
}

void readMapping_GBM() {
  // Enable ports 0x0120
  send_GBM(0x09);

  // Set WE and WP
  send_GBM(0x0A);
  send_GBM(0x2);

  // Enable hidden mapping area
  writeByte_GBM(0x2100, 0x01);
  send_GBM(0x0F, 0x5555, 0xAA);
  send_GBM(0x0F, 0x2AAA, 0x55);
  send_GBM(0x0F, 0x5555, 0x77);
  send_GBM(0x0F, 0x5555, 0xAA);
  send_GBM(0x0F, 0x2AAA, 0x55);
  send_GBM(0x0F, 0x5555, 0x77);

  // Read mapping
  println_Msg(F("Reading Mapping..."));
  display_Update();

  // Get name, add extension and convert to char array for sd lib
  EEPROM_readAnything(10, foldern);
  sprintf(fileName, "GBM%d", foldern);
  strcat(fileName, ".map");
  sd.mkdir("NP", true);
  sd.chdir("NP");
  // write new folder number back to eeprom
  foldern = foldern + 1;
  EEPROM_writeAnything(10, foldern);

  // Open file on sd card
  if (!myFile.open(fileName, O_RDWR | O_CREAT)) {
    print_Error(F("Can't create file on SD"), true);
  }
  else {
    for (byte currByte = 0; currByte < 128; currByte++) {
      sdBuffer[currByte] = readByte_GBM(currByte);
    }
    myFile.write(sdBuffer, 128);

    // Close the file:
    myFile.close();

    // Signal end of process
    printSdBuffer(0, 20);
    printSdBuffer(102, 20);
    println_Msg("");
    print_Msg(F("Saved to NP/"));
    println_Msg(fileName);
    display_Update();
  }

  // Reset flash to leave hidden mapping area
  resetFlash_GBM();
}

void eraseFlash_GBM() {
  println_Msg(F("Erasing..."));
  display_Update();

  //enable access to ports 0120h
  send_GBM(0x09);
  // Enable write
  send_GBM(0x0A);
  send_GBM(0x2);

  // Unprotect sector 0
  send_GBM(0x0F, 0x5555, 0xAA);
  send_GBM(0x0F, 0x2AAA, 0x55);
  send_GBM(0x0F, 0x5555, 0x60);
  send_GBM(0x0F, 0x5555, 0xAA);
  send_GBM(0x0F, 0x2AAA, 0x55);
  send_GBM(0x0F, 0x5555, 0x40);

  // Wait for unprotect to complete
  while ((readByte_GBM(0) & 0x80) != 0x80) {}

  // Send erase command
  send_GBM(0x0F, 0x5555, 0xaa);
  send_GBM(0x0F, 0x2AAA, 0x55);
  send_GBM(0x0F, 0x5555, 0x80);
  send_GBM(0x0F, 0x5555, 0xaa);
  send_GBM(0x0F, 0x2AAA, 0x55);
  send_GBM(0x0F, 0x5555, 0x10);

  // Wait for erase to complete
  while ((readByte_GBM(0) & 0x80) != 0x80) {}

  // Reset flashrom
  resetFlash_GBM();
}

boolean blankcheckFlash_GBM() {
  print_Msg(F("Blankcheck..."));
  display_Update();

  //enable access to ports 0120h (F2)
  send_GBM(0x09);

  // Map entire flashrom
  send_GBM(0x04);
  // Disable ports 0x0120...
  send_GBM(0x08);

  // Read rom
  word currAddress = 0;

  for (byte currBank = 1; currBank < 64; currBank++) {
    // Set rom bank
    writeByte_GBM(0x2100, currBank);

    // Switch bank start address
    if (currBank > 1) {
      currAddress = 0x4000;
    }

    for (; currAddress < 0x7FFF; currAddress++) {
      if (readByte_GBM(currAddress) != 0xFF) {
        return 0;
      }
    }
  }
  return 1;
}

void writeFlash_GBM() {
  print_Msg(F("Writing..."));
  display_Update();

  // Open file on sd card
  if (myFile.open(filePath, O_READ)) {
    // Get rom size from file
    fileSize = myFile.fileSize();
    if ((fileSize / 0x4000) > 64) {
      print_Error(F("File is too big."), true);
    }

    // Enable access to ports 0120h
    send_GBM(0x09);
    // Enable write
    send_GBM(0x0A);
    send_GBM(0x2);

    // Map entire flash rom
    send_GBM(0x4);

    // Set bank for unprotect command, writes to 0x5555 need odd bank number
    writeByte_GBM(0x2100, 0x1);

    // Disable ports 0x2100 and 0x120 or else those addresses will not be writable
    send_GBM(0x10);
    send_GBM(0x08);

    // Unprotect sector 0
    writeByte_GBM(0x5555, 0xAA);
    writeByte_GBM(0x2AAA, 0x55);
    writeByte_GBM(0x5555, 0x60);
    writeByte_GBM(0x5555, 0xAA);
    writeByte_GBM(0x2AAA, 0x55);
    writeByte_GBM(0x5555, 0x40);

    // Check if flashrom is ready for writing or busy
    while ((readByte_GBM(0) & 0x80) != 0x80) {}

    // first bank: 0x0000-0x7FFF,
    word currAddress = 0x0;

    // Write 63 banks
    for (byte currBank = 0x1; currBank < (fileSize / 0x4000); currBank++) {
      // Blink led
      PORTB ^= (1 << 4);

      // all following banks: 0x4000-0x7FFF
      if (currBank > 1) {
        currAddress = 0x4000;
      }

      // Write single bank in 128 byte steps
      for (; currAddress < 0x7FFF; currAddress += 128) {
        // Fill SD buffer
        myFile.read(sdBuffer, 128);

        // Enable access to ports 0x120 and 0x2100
        send_GBM(0x09);
        send_GBM(0x11);

        // Set bank
        writeByte_GBM(0x2100, 0x1);

        // Disable ports 0x2100 and 0x120 or else those addresses will not be writable
        send_GBM(0x10);
        send_GBM(0x08);

        // Write flash buffer command
        writeByte_GBM(0x5555, 0xAA);
        writeByte_GBM(0x2AAA, 0x55);
        writeByte_GBM(0x5555, 0xA0);

        // Wait until flashrom is ready again
        while ((readByte_GBM(0) & 0x80) != 0x80) {}

        // Enable access to ports 0x120 and 0x2100
        send_GBM(0x09);
        send_GBM(0x11);

        // Set bank
        writeByte_GBM(0x2100, currBank);

        // Disable ports 0x2100 and 0x120 or else those addresses will not be writable
        send_GBM(0x10);
        send_GBM(0x08);

        // Fill flash buffer
        for (word currByte = 0; currByte < 128; currByte++) {
          writeByte_GBM(currAddress + currByte, sdBuffer[currByte]);
        }
        // Execute write
        writeByte_GBM(currAddress + 127, 0xFF);

        // Wait for write to complete
        while ((readByte_GBM(currAddress) & 0x80) != 0x80) {}
      }
    }
    // Close the file:
    myFile.close();
    println_Msg(F("Done"));
  }
  else {
    print_Error(F("Can't open file"), false);
  }
}

void eraseMapping_GBM() {

}

boolean blankcheckMapping_GBM() {

}

void writeMapping_GBM() {

}

//******************************************
// End of File
//******************************************
