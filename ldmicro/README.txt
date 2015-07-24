
LDmicro is a ladder logic editor, simulator and compiler for 8-bit
microcontrollers. It can generate native code for Atmel AVR and Microchip
PIC16 CPUs from a ladder diagram.

This program is free software; see COPYING.txt.

I started work on LDmicro in October 2004.

At present, I receive a steady stream of questions and feature requests,
that I do not have time to address. I hope that others who find this
program interesting or useful may be able to complete this work.


BUILDING LDMICRO
================

LDmicro is built using the Microsoft Visual C++ compiler. If that is
installed correctly, then you should be able to just run

    make.bat

and see everything build. The regression tests (however inadequate;
see below) will run automatically after the program is built.

Various source and header files are generated automatically. The perl
scripts to do this are included with this distribution, but it's necessary
to have a perl.exe in your path somewhere.

The makefile accepts an argument, D=LANG_XX, where XX is the language
code. make.bat supplies that argument automatically, as LANG_EN (English).


HIGH-LEVEL ARCHITECTURE
=======================

LDmicro is a compiler. Unlike typical compilers, it does not accept a text
file as its input; instead, the program is edited graphically, within
LDmicro itself. The editor represents the program as a tree structure,
where each node represents either an instruction (like relays contacts
or a coil) or a series or parallel subcircuit.

Based on this tree structure, intcode.cpp generates an intermediate
`instruction list' representation of the program. This intermediate code
is then passed to a processor-specific back-end, which generates code
for the target.

The program may also be simulated; in that case, rather than compiling
the intermediate code to a .hex file, we interpret the intermediate code
and display the results in real time.

The intermediate code is useful, because there are fewer intermediate
code ops than high-level ladder logic instructions.

See INTERNALS.txt for some discussion of LDmicro's internals.


POSSIBLE ENHANCEMENTS
=====================

If you are interested in contributing to LDmicro, then I believe that
the following work would be useful. The list is in ascending order
of difficulty.

    * Better regression tests. There is a mechanism for this now (see the
      reg/... directory), but very few tests. A regression test is just
      an LDmicro program; I verify that the same input .ld file generates
      the same output .hex file. The code generators are stupid enough
      that these don't break all that often (no fancy optimizers that
      reorder everything based on a small change to the input).

    * Thorough tests of the existing microcontroller targets. This
      requires appropriate hardware to test against. When I don't have
      the hardware myself, I will sometimes test against a simulator,
      but this is not as good as the real thing. The simulation of
      peripherals is often approximate.

      Even if no bugs are found, the test programs can become regression
      tests, and we can mark those targets that have been tested as
      `known good'.

    * More targets for PIC16 and AVR architectures. In general, this
      should be very easy; just add the appropriate entries in
      mcutable.h. Some small changes to pic16.cpp or avr.cpp may be
      required, if the peripheral registers move around.

      I am not interested in new targets that have not been tested
      against real hardware.

    * The code generator has regression tests now. It would be nice to
      also have regression tests for the display code. We could do this
      using the "Export As Text" logic, since that calls the exact same
      code that's used to display the program on-screen.

    * Bitwise instructions that work on integers (e.g., let internal
      relay Rfoo equal the seventh bit of integer variable x, set c =
      a bitwise-and b, etc).

      This would require new intermediate code ops, and thus changes to
      both the PIC and AVR back ends, and thus testing of those changes.
      Aside from that, it's very straightforward.

      These new intcode ops could then be used by higher-level ELEM_XXX
      ops, for example to do bit-banged synchronous serial protocols
      (I2C, SPI), or to talk to an HD44780-type LCD.

    * Intermediate code ops for look-up tables. At present, these are
      implemented stupidly, as a chain of if-then statements. This would
      require changes to all the code generators.

      Look-up tables are important, not just for the `look-up table'
      instruction, but also for the `formatted string over serial'
      instruction, where the string is stored in a look-up table.

    * New architecture targets. An architecture target translates from
      LDmicro's intermediate code (intcode) to a binary for that
      architecture. This means that it includes a code generator and
      an assembler. The PIC16 and AVR targets are about 1500 lines of
      code each.

      A PIC18 target would be nice. I am not interested in a PIC18
      back-end that does not exploit the additional addressing modes
      and instructions (like rjmps); it would be easy to hack up the
      PIC16 back end to generate something that would run on a PIC18,
      but not useful.

      An 8051 target is the other obvious choice, maybe also for the
      MSP430.

    * A Linux port. I am told that the current version runs perfectly
      under WINE, but a native GTK port would be nice. That would be
      a lot of work.

      I am not interested in porting to a cross-platform GUI toolkit, and
      thus abandoning the native Win32 user interface. In general, I think
      that those toolkits end up looking equally wrong on all platforms.

      If I were to undertake this myself, then I would probably end up
      writing a miniature compatibility layer for the widgets that I
      actually use, that mapped on to GTK under Linux and Win32 under
      Windows, and so on. I've seen attempts at general-purpose libraries
      to do that, but they look extremely complex for what they do.

In general, I am not interested in changes that involve linking against
non-standard libraries. (By non-standard, I mean anything that's not
distributed with the operating system.)


FOREIGN-LANGUAGE TRANSLATIONS
=============================

Foreign-language translations of the documentation and user interface
are always useful. At present, we have English, German, and French.

These do not require changes to the source code; a lookup table of
translated strings is automatically generated from lang-*.txt during
the build process. If you are interested in preparing a translation,
but do not wish to set up tools to build LDmicro, then just mail me the
lang-xx.txt, and I can build ldmicro-xx.exe for you.

At present, LDmicro does not use Unicode. This means that translations
into languages not written in the Latin alphabet are tricky. This could
be changed, of course.


FINAL
=====

I will always respond to bug reports. If LDmicro is behaving incorrectly
on some target, then please attach the simplest example of a program
that's misbehaving, and a description of the incorrect behavior.

Please contact me if you have any questions.


Jonathan Westhues
user jwesthues, at host cq.cx

near Seattle, Oct 21, 2007


