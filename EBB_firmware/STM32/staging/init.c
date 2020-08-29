

#include <p18cxxx.h>
#include "init.h"
#include "fifo.h"
#include <stdio.h>
#include "HardwareProfile.h"
#include "usbser.h"
#include "analog.h"
#include "utility.h"
#include "parse.h"
#include "isr.h"
#include "delays.h"
#include "solenoid.h"
#include "servo.h"
#include "ebb.h"
#include "stepper.h"
#include "serial.h"
#include "analog.h"

void UserInit(void)
{
  int  i, j;

  // Make all of PORTa inputs
  LATA = 0x00;
  TRISA = 0xFF;
  // Make all of PORTB inputs
  LATB = 0x00;
  TRISB = 0xFF;
  // Make all of PORTC inputs
  LATC = 0x00;
  TRISC = 0xFF;

  DEBUG_INIT()          // This needs to be AFTER we make all pins inputs
#if defined(BOARD_3BB)
  SerialInit();
#endif

  // Initialize LED I/Os to outputs
  mInitAllLEDs();
  // Initialize switch as an input
  mInitSwitch();

  // Start off always using "OK" acknowledge.
  g_ack_enable = TRUE;

  // Use our own special output function for STDOUT
  stdout = _H_USER;

  fifo_Init();

  // Initialize Timer4 (1ms ISR for Serv2, timing, and pulse commands)
  // The prescaler will be at 16
  T4CONbits.T4CKPS1 = 1;
  T4CONbits.T4CKPS0 = 1;
  // We want the TMR4 post scaler to be a 3
  T4CONbits.T4OUTPS3 = 0;
  T4CONbits.T4OUTPS2 = 0;
  T4CONbits.T4OUTPS1 = 1;
  T4CONbits.T4OUTPS0 = 0;
  // Set our reload value
  PR4 = kPR4_RELOAD;

  usbser_Init();
  
/// TODO: IS this for RCServo2, or can we get rid of it?
  // Enable TMR0 for our RC timing operation
//  T0CONbits.PSA = 1;        // Do NOT use the prescaler
//  T0CONbits.T0CS = 0;       // Use internal clock
//  T0CONbits.T08BIT = 0;     // 16 bit timer
//  INTCONbits.TMR0IF = 0;    // Clear the interrupt flag
//  INTCONbits.TMR0IE = 0;    // And clear the interrupt enable
//  INTCON2bits.TMR0IP = 0;   // Low priority

  // Enable interrupt priorities
  RCONbits.IPEN = 1;          // Enable both high and low interrupt priorities
  T4CONbits.TMR4ON = 0;       // Disable Timer4 (??)

  PIE3bits.TMR4IE = 1;        // Turn on Timer4 match interrupt enable
  IPR3bits.TMR4IP = 0;        // Timer4 interrupt priority = LOW
  
  // Set up TMR1 for our 25KHz High ISR for stepping
  T1CONbits.RD16 = 1;       // Set 16 bit mode
  T1CONbits.TMR1CS1 = 0;    // System clocked from Fosc/4
  T1CONbits.TMR1CS0 = 0;
  T1CONbits.T1CKPS1 = 0;    // Use 1:1 Prescale value
  T1CONbits.T1CKPS0 = 0;
  T1CONbits.T1OSCEN = 0;    // Don't use external osc
  T1CONbits.T1SYNC = 0;
  TMR1H = TIMER1_H_RELOAD;  //
  TMR1L = TIMER1_L_RELOAD;  // Reload for 25Khz ISR fire

  T1CONbits.TMR1ON = 1;     // Turn the timer on
 

  IPR1bits.TMR1IP = 1;      // Use high priority interrupt
  PIR1bits.TMR1IF = 0;      // Clear the interrupt
  PIE1bits.TMR1IE = 1;      // Turn on the interrupt

  
//  PORTA = 0;
#if defined(BOARD_EBB)
  RefRA0_IO_TRIS = INPUT_PIN;
#endif
//  PORTB = 0;
//  INTCON2bits.RBPU = 0;   // Turn on weak-pull ups for port B
//  PORTC = 0;              // Start out low
//  TRISC = 0x80;           // Make portC output except for PortC bit 7, USB bus sense
//  PORTD = 0;
//  TRISD = 0;
//  PORTE = 0;
//  TRISE = 0;

#if defined(BOARD_EBB)
  MS1_IO = 1;
  MS1_IO_TRIS = OUTPUT_PIN;
  MS2_IO = 1;
  MS2_IO_TRIS = OUTPUT_PIN;
  MS3_IO  = 1;
  MS3_IO_TRIS = OUTPUT_PIN;

  Enable1IO = 1;
  Enable1IO_TRIS = OUTPUT_PIN;
  Enable2IO = 1;
  Enable2IO_TRIS = OUTPUT_PIN;
#elif defined(BOARD_3BB)
  /// TODO: Default EN high (DISABLED), later, on driver init, set low
  DEBUG_A0_SET()
  EnableIO = 1;
  EnableIO_TRIS = OUTPUT_PIN;

#endif
  
  Step1IO = 0;
  Step1IO_TRIS = OUTPUT_PIN;
  Dir1IO = 0;
  Dir1IO_TRIS = OUTPUT_PIN;
  Step2IO = 0;
  Step2IO_TRIS = OUTPUT_PIN;
  Dir2IO = 0;
  Dir2IO_TRIS = OUTPUT_PIN;
#if defined(BOARD_3BB)
  Step3IO = 0;
  Step3IO_TRIS = OUTPUT_PIN;
  Dir3IO = 0;
  Dir3IO_TRIS = OUTPUT_PIN;
#endif
  
  // For bug in VUSB divider resistor, set RC7 as output and set high
  // Wait a little while to charge up
  // Then set back as an input
  // The idea here is to get the schmidt trigger input RC7 high before
  // we make it an input, thus getting it above the 2.65V ST threshold
  // And allowing VUSB to keep the logic level on the pin high at 2.5V
  #if defined(USE_USB_BUS_SENSE_IO)
    tris_usb_bus_sense = OUTPUT_PIN; // See HardwareProfile.h
    USB_BUS_SENSE = 1;
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    Delay1TCY();
    tris_usb_bus_sense = INPUT_PIN;
    USB_BUS_SENSE = 0;
  #endif
  gUseSolenoid = TRUE;

#if defined(BOARD_EBB)
  // Set up pen up/down direction as output
  PenUpDownIO = 0;
  PenUpDownIO_TRIS = OUTPUT_PIN;
    
  // Set up RC Servo power control to be off
  RCServoPowerIO = RCSERVO_POWER_OFF;
  RCServoPowerIO_TRIS = OUTPUT_PIN;
#endif
  
  solenoid_Init();
  
  // Default RB0 to be an input, with the pull-up enabled, for use as alternate
  // PAUSE button (just like PRG)
  // Except for v1.1 hardware, use RB2
  TRISBbits.TRISB0 = 1;
  INTCON2bits.RBPU = 0; // Turn on all of PortB pull-ups
  UseAltPause = TRUE;

  TRISBbits.TRISB3 = 0;   // Make RB3 an output (for engraver)
  PORTBbits.RB3 = 0;          // And make sure it starts out off
    
  // Clear out global stepper positions
  parseCSCommand();
    
  servo_Init();
  
  analogInit();
  
  // Initialize INT1 to map to RP13 (RC2) which is SCALED_V+.
  RPINR1 = 13;
  ScaledVPlusIO_TRIS = INPUT_PIN;
  
  // Set IN1 to be a low priority interrupt and turn it on for either rising
  // or falling edge, whatever state RC2 is currently not in
  INTCON3bits.INT1IP = 0;
//  if (ScalaedVPlusIO)
//  {
//    INTCON2bits.INTEDG1 = 0;
//  }
//  else
//  {
  // Always look for a rising edge on DIAG
    INTCON2bits.INTEDG1 = 1;
//  }
  // If we already have V+ power, then trigger an immediate driver init
  if (ScalaedVPlusIO)
  {
    DriversNeedInit = TRUE;
  }
    
  INTCON3bits.INT1IF = 0;     // Clear the flag
  INTCON3bits.INT1IE = 1;     // Turn on the interrupt
  
  INTCONbits.GIEH = 1;  // Turn high priority interrupts on
  INTCONbits.GIEL = 1;  // Turn low priority interrupts on

  // Turn on the Timer4
  T4CONbits.TMR4ON = 1; 
  
  // If there's a name in FLASH for us, copy it over to the USB Device
  // descriptor before we enumerate
  populateDeviceStringWithName();
}