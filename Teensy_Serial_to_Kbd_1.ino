#include <Keyboard.h>
//#include <SerialCommands.h>
#include <SerialCommands.h>
#include <Arduino.h>

char banner_str[] = "CMD_SERIAL Andrew's Fast Serial Command to USB Keyboard Actor v1.4";
/*
handle:
 keys:
   MOD_GUI_LEFT (I think is CMD)
   MOD_ALT_LEFT (I think is OPT)

'opt'      (startup manager)  MODIFIERKEY_ALT   (was: KEY_LEFT_ALT)
'cmdr'     (MacOS Recovery)   MODIFIERKEY_GUI   (was: KEY_LEFT_GUI) +
'optcmdr'  (Who Knows?)
'optn'     ? (Use default boot image (OLD)) MODIFIERKEY_ALT, KEY_N
'optd'     ? (Start Apple Diags "over internet")  MODIFIERKEY_ALT, KEY_D
'd'        (Start Apple Diags)          KEY_D
'n'        (Start Netboot  (OLD))       KEY_N
'optcmdpr' (Reset NVRAM)
'shift'    (Safe Mode)        MODIFIERKEY_SHIFT
'cmdv'     (Verbose Mode)
't'        (Target disk mode)
*/

#define MODIFIERKEY_OPT MODIFIERKEY_ALT
#define MODIFIERKEY_CMD MODIFIERKEY_GUI

const int ledPin = 13;

char serial_command_buffer_[32];

#define USB_KBD Serial
#define CMD_SERIAL Serial1

char const terminator[] = "\r";
char const delim[] = " ";
SerialCommands serial_commands_(&CMD_SERIAL, serial_command_buffer_, sizeof(serial_command_buffer_), terminator, delim, true);

unsigned long next_finish_millis = 0;
unsigned long next_status_blink_millis = 0;

uint8_t  keys_pressed=0;
bool k_mode = true;    // true = Continual press
char *KmodeStrings[] = {"Press/Release",
                        "Continual Press"};
bool reboot_mode=true;  // true = reboot after commands longer than 1 Sec
char *RmodeStrings[] = {"No", "Yes"};

void Restart(void)
{
    #define RESTART_ADDR       0xE000ED0C
    #define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
    #define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

    // 0000101111110100000000000000100
    // Assert [2]SYSRESETREQ
    WRITE_RESTART(0x5FA0004);
}

void USB_KBD_Serial_Startup(void) 
{
  USB_KBD.begin(57600); 
}

void assign_next_completion_time(int secs) {
    if (secs == -1) {
        next_finish_millis = 0;
        return;
    }

    if (secs == 0) {
        secs = 1;
    } else if (secs > 180) {
        secs = 180;
    }

    next_finish_millis = millis() + (1000 * secs);
}

void status_blink(void) {
  if (! next_finish_millis) { 
    for (int x=3; x--; ){
      digitalWrite(ledPin, HIGH); 
      delay(100);
      digitalWrite(ledPin, LOW); 
      delay(100);
    }
  } 
}

void cmd_unrecognized(SerialCommands* sender, const char* cmd)
{
  int cmd_len = sizeof(cmd);

  CMD_SERIAL.print("Unrecognized command [ ");
  for (int x=0; x < cmd_len; x++) {
      CMD_SERIAL.print("0x");
      CMD_SERIAL.print(cmd[x], HEX);
      if (isPrintable(cmd[x])) {
        CMD_SERIAL.print("("); 
        CMD_SERIAL.print(cmd[x]); 
        CMD_SERIAL.print(")"); 
      }
      CMD_SERIAL.print("  ");
  }
  
  CMD_SERIAL.print("] ");
  CMD_SERIAL.print(sizeof(cmd));
  CMD_SERIAL.println(" chars.");
  serial_commands_.ClearBuffer();
}


void do_usb_kbd_clear(void)
{
  //Note: Every call to Next moves the pointer to next parameter
  CMD_SERIAL.println("in do_usb_kbd_clear:");

  Keyboard.releaseAll();
  digitalWrite(ledPin, LOW);
  Keyboard.end();

  if (reboot_mode) {
    Restart(); 
  }
  next_finish_millis = 0;
  return;
}

int duration_check_set(SerialCommands* sender)
{
  int secs=1;

  char* secs_str = sender->Next();
  if (secs_str == NULL) {
    sender->GetSerial()->println("No duration specified, will use 1 sec");
  } else {
    secs = atoi(secs_str);
  }

  digitalWrite(ledPin, HIGH);
  assign_next_completion_time(secs);
  return (secs);
}

void do_mode_continual_press(SerialCommands* sender)
{
    k_mode = true;
    CMD_SERIAL.print("mode: supposed CONTINUAL Key Press\n> ");
    do_printHelp();
}

void do_mode_press_release(SerialCommands* sender)
{
    k_mode = false;
    CMD_SERIAL.print("mode: continual Press/Release\n> ");
    do_printHelp();
}

void do_toggle_k_mode(SerialCommands* sender)
{
    k_mode = !k_mode;
    do_printHelp();
}

void do_toggle_reboot_after_cmd(SerialCommands* sender)
{
    reboot_mode = !reboot_mode;
    do_printHelp();
}


SerialCommand do_questMk_("?", do_printHelp);
SerialCommand do_h_("h", do_printHelp);
SerialCommand do_help_("help", do_printHelp);

SerialCommand do_usb_kbd_opt_("opt", do_usb_kbd_opt);
SerialCommand do_usb_kbd_cmdr_("cmdr", do_usb_kbd_cmdr);
SerialCommand do_usb_kbd_optd_("optd", do_usb_kbd_optd);
SerialCommand do_usb_kbd_d_("d", do_usb_kbd_d);
SerialCommand do_usb_kbd_n_("n", do_usb_kbd_n);
SerialCommand do_usb_kbd_optcmdpr_("optcmdpr", do_usb_kbd_optcmdpr);
SerialCommand do_usb_kbd_shift_("shift", do_usb_kbd_shift);
SerialCommand do_usb_kbd_cmdv_("cmdv", do_usb_kbd_cmdv);
SerialCommand do_usb_kbd_t_("t", do_usb_kbd_t);
SerialCommand do_mode_continual_press_("kp", do_mode_continual_press);
SerialCommand do_mode_press_release_("kr", do_mode_press_release);
SerialCommand do_toggle_k_mode_ ("tkm", do_toggle_k_mode);
SerialCommand do_toggle_reboot_after_cmd_ ("trb", do_toggle_reboot_after_cmd);


void do_usb_kbd_opt(SerialCommands* sender)
{
  int secs = duration_check_set(sender);

  CMD_SERIAL.print("do_usb_kbd_opt: ");
  CMD_SERIAL.print(secs);
  CMD_SERIAL.println(" sec.");

  USB_KBD_Serial_Startup();

  keys_pressed = MODIFIERKEY_OPT;
}

void do_usb_kbd_cmdr(SerialCommands* sender)
{
  int secs = duration_check_set(sender);

  CMD_SERIAL.print("do_usb_kbd_cmdr: ");
  CMD_SERIAL.print(secs);
  CMD_SERIAL.println(" sec.");

  USB_KBD_Serial_Startup();

  if (k_mode) {
    Keyboard.press(MODIFIERKEY_CMD);
    Keyboard.press(KEY_R);
  } else {
    keys_pressed = (MODIFIERKEY_CMD | KEY_R);
  }
}

void do_usb_kbd_optd(SerialCommands* sender)
{
  int secs = duration_check_set(sender);

  CMD_SERIAL.print("do_usb_kbd_optd: ");
  CMD_SERIAL.print(secs);
  CMD_SERIAL.println(" sec.");

  USB_KBD_Serial_Startup();

  if (k_mode) {
      Keyboard.press(MODIFIERKEY_OPT);
      Keyboard.press(KEY_R);
  } else {
      keys_pressed = (MODIFIERKEY_OPT | KEY_R);
  }
}

void do_usb_kbd_d(SerialCommands* sender)
{
  int secs = duration_check_set(sender);

  CMD_SERIAL.print("do_usb_kbd_d: ");
  CMD_SERIAL.print(secs);
  CMD_SERIAL.println(" sec.");

  USB_KBD_Serial_Startup();

  if (k_mode) {
    Keyboard.press(KEY_D);
  } else {
    keys_pressed = KEY_D;
  }
}

void do_usb_kbd_n(SerialCommands* sender)
{
  int secs = duration_check_set(sender);

  CMD_SERIAL.print("do_usb_kbd_n: ");
  CMD_SERIAL.print(secs);
  CMD_SERIAL.println(" sec.");

  USB_KBD_Serial_Startup();

  if (k_mode) {
    Keyboard.press(KEY_N);
  } else {
    keys_pressed = KEY_N;
  }
}

void do_usb_kbd_optcmdpr(SerialCommands* sender)
{
  int secs = duration_check_set(sender);

  CMD_SERIAL.print("do_usb_kbd_optcmdpr: ");
  CMD_SERIAL.print(secs);
  CMD_SERIAL.println(" sec.");

  USB_KBD_Serial_Startup();

  if (k_mode) {
      Keyboard.press(MODIFIERKEY_OPT);
      Keyboard.press(MODIFIERKEY_CMD);
      Keyboard.press(KEY_P);
      Keyboard.press(KEY_R);
  } else {
      keys_pressed = (MODIFIERKEY_OPT | MODIFIERKEY_CMD | KEY_P | KEY_R);
  }
}

void do_usb_kbd_shift(SerialCommands* sender)
{
  int secs = duration_check_set(sender);

  CMD_SERIAL.print("do_usb_kbd_shift: ");
  CMD_SERIAL.print(secs);
  CMD_SERIAL.println(" sec.");

  USB_KBD_Serial_Startup();

  if (k_mode) {
      Keyboard.press(MODIFIERKEY_SHIFT);
  } else {
      keys_pressed = MODIFIERKEY_SHIFT;
  }
}

void do_usb_kbd_cmdv(SerialCommands* sender)
{
  int secs = duration_check_set(sender);

  CMD_SERIAL.print("do_usb_kbd_cmdv: ");
  CMD_SERIAL.print(secs);
  CMD_SERIAL.println(" sec.");

  USB_KBD_Serial_Startup();

  if (k_mode) {
      Keyboard.press(MODIFIERKEY_CMD);
      Keyboard.press(KEY_V);
  } else {
      keys_pressed = (MODIFIERKEY_CMD | KEY_V);
  }
}

void do_usb_kbd_t(SerialCommands* sender)
{
  int secs = duration_check_set(sender);

  CMD_SERIAL.print("do_usb_kbd_t: ");
  CMD_SERIAL.print(secs);
  CMD_SERIAL.println(" sec.");

  USB_KBD_Serial_Startup();

  if (k_mode) {
      Keyboard.press(KEY_T);
  } else {
      keys_pressed = KEY_T;
  }
}

void do_usb_clear(SerialCommands* sender)
{
  //Note: Every call to Next moves the pointer to next parameter
  CMD_SERIAL.println("do_usb_clear:");

  USB_KBD_Serial_Startup();

  keys_pressed = 0;
  Keyboard.releaseAll();
  digitalWrite(ledPin, LOW);

  next_finish_millis = 0;
}

void setup() {
  pinMode(ledPin, OUTPUT);
//  USB_KBD.begin(57600);
  CMD_SERIAL.begin(115200);
  for (int x=3; x--; ){
    digitalWrite(ledPin, HIGH); 
    delay(300);
    digitalWrite(ledPin, LOW); 
    delay(300);
  }

//  USB_KBD.println("USB_KBD Andrew's Fast Serial Command to USB Keyboard Actor v1.1");
  
  serial_commands_.SetDefaultHandler(cmd_unrecognized);
  serial_commands_.AddCommand(&do_questMk_);
  serial_commands_.AddCommand(&do_h_);
  serial_commands_.AddCommand(&do_help_);
  serial_commands_.AddCommand(&do_usb_kbd_opt_);
  serial_commands_.AddCommand(&do_usb_kbd_cmdr_);
  serial_commands_.AddCommand(&do_usb_kbd_optd_);
  serial_commands_.AddCommand(&do_usb_kbd_d_);
  serial_commands_.AddCommand(&do_usb_kbd_n_);
  serial_commands_.AddCommand(&do_usb_kbd_optcmdpr_);
  serial_commands_.AddCommand(&do_usb_kbd_shift_);
  serial_commands_.AddCommand(&do_usb_kbd_cmdv_);
  serial_commands_.AddCommand(&do_usb_kbd_t_);
  serial_commands_.AddCommand(&do_mode_continual_press_);
  serial_commands_.AddCommand(&do_mode_press_release_);
  serial_commands_.AddCommand(&do_toggle_k_mode_);
  serial_commands_.AddCommand(&do_toggle_reboot_after_cmd_);

  do_printHelp();
}

unsigned long  throttle=0;
unsigned long  throttle_cnt=0;

void loop() {
  if (next_status_blink_millis == 0 || next_status_blink_millis < millis() ) {
    next_status_blink_millis = millis() + 20000;
    status_blink();
  }
   
  if (next_finish_millis && next_finish_millis < millis()) {
    next_finish_millis = 0;
    do_usb_kbd_clear();
    CMD_SERIAL.print("> ");
    USB_KBD.end();
  }

  if (keys_pressed) {
//    Keyboard.press(keys_pressed);
//    Keyboard.write(keys_pressed);
    Keyboard.set_modifier(keys_pressed);
    Keyboard.send_now();
  }

  serial_commands_.ReadSerial();
  
//  serial_commands_.ClearBuffer();
}

void do_printHelp(void)
{
  CMD_SERIAL.println(banner_str);

  CMD_SERIAL.println("key(s) held pressed during reboot:"); 
  CMD_SERIAL.println("  <seq> [secs]    where <seq> is one of:");
  CMD_SERIAL.println("  cmd, opt, shift, (plus another or a letter)");
  CMD_SERIAL.println("");        
  CMD_SERIAL.println("   opt      (startup manager) uses un-changable 'kr' only");
  CMD_SERIAL.println("   cmdr     (MacOS Recovery) NOT YET");
  CMD_SERIAL.println("   optcmdr  (Who Knows?)");
  CMD_SERIAL.println("   optd     (Use default boot image (OLD))");
  CMD_SERIAL.println("   d        (Start Apple Diags)");
  CMD_SERIAL.println("   optd     (Start Apple Diags 'over internet')");
  CMD_SERIAL.println("   n        (Start Netboot  (OLD))");
  CMD_SERIAL.println("   optcmdpr (Reset NVRAM)");
  CMD_SERIAL.println("   shift    (Safe Mode)");
  CMD_SERIAL.println("   cmdv     (Verbose Mode)");
  CMD_SERIAL.println("   t        (Target disk mode) ONLY FROM POWER-UP\n");
  CMD_SERIAL.println("   kp       supposed CONTINUAL Key Press");
  CMD_SERIAL.println("   kr       continual Press/Release\n");
  CMD_SERIAL.print  ("   tkm      toggle Key Mode currently ");
  CMD_SERIAL.println(KmodeStrings[(int)k_mode]);
  CMD_SERIAL.print  ("   trb      toggle ReBoot after cmd > 1 sec (currently ");
  CMD_SERIAL.print  (RmodeStrings[(int)reboot_mode]);
  CMD_SERIAL.println(")\n");
  CMD_SERIAL.print("> ");
}
