<<<<<<< HEAD
# KNOWLEDGE.md — Week 3: MCU Architecture and Bare-Metal Programming

## Overview

This week the student transitions from abstract C programming and bitwise operations to understanding how the microcontroller is organized internally. Many concepts that were presented as "black boxes" in previous weeks now start becoming transparent. The student learns to read official technical documentation (reference manual, datasheet), understands how peripherals connect to the CPU through buses, and begins to see CMSIS structures not as magic but as carefully designed overlays on the hardware. The debugger becomes a tool for inspecting the microcontroller's internal state directly through the SFR view panel (Special Function Registers).

---

## Previously Mastered Topics (Weeks 0–2)

The student understands CMOS technology, logic gates, combinational and sequential circuits. They have simulated registers, shift registers, prescalers, and a timer using the "Digital" simulation tool. They understand binary, hexadecimal, and 2's complement number systems.

In C programming, the student can write programs using `if/else`, `while`, `for`, `do-while`, `switch-case`, and fixed-width data types from `stdint.h` (`uint8_t`, `int8_t`, `uint16_t`, `int16_t`, `uint32_t`, `int32_t`). They understand arithmetic operators (`+`, `-`, `*`, `/`, `%`), shift operators (`>>`, `<<`), and boolean evaluation (0 is false, anything not 0 is true). Their C skills are still developing — expect occasional syntax errors and uncertainty.

The student knows all bitwise logic operators: AND (`&`), OR (`|`), NOT (`~`), XOR (`^`), and their compound assignment forms: `|=` for setting bits, `&= ~()` for clearing bits, `^=` for toggling bits. They understand the concept of a mask as a value created to modify specific bits in another variable. They have applied these operations both to regular variables and to real MCU registers as a first exposure (enabling RCC clock and turning on an LED), though that register-level work was presented at a "black box" level without full architectural explanation.

The student knows `#include` and `#define` at a basic level. They can create projects in VS Code with the STM32 extension pack, use the debugger to set breakpoints, step through code, and inspect variables.

The student has been introduced to Finite State Machines (FSM) as a design tool — state diagrams, identifying states and transitions, describing system behavior. This is conceptual only; no FSM code implementation has been done. The student does NOT yet know `enum`.

The student does NOT know structures, unions, arrays, pointers, enumerations, or `typedef`. The `->` operator was introduced in week 2 only as "the way to access registers" without explanation.

---

## Current Learning Focus (Week 3)

### MCU architecture

The student is learning how the STM32F4xx microcontroller is organized internally: the ARM Cortex-M4 CPU core, the bus system, and how peripherals connect to the CPU. The specific buses covered are AHB (Advanced High-performance Bus), APB1 (Advanced Peripheral Bus 1 — low-speed peripherals), and APB2 (Advanced Peripheral Bus 2 — high-speed peripherals). The student learns which peripherals are connected to which bus and why this matters — for example, GPIO ports are on AHB1, USART2 is on APB1. The student understands that each peripheral needs a clock signal enabled through the RCC (Reset and Clock Control) before it can be used.

### Memory-mapped registers and SFR

The student is learning that peripherals in the microcontroller are controlled through Special Function Registers (SFR) that are mapped to specific memory addresses. Writing a value to a memory address is the same as writing to a peripheral register — this is the fundamental concept of memory-mapped I/O. The student can now connect the abstract idea of "bits in a variable" to "bits that control real hardware behavior."

### CMSIS structures — the Italian tailor analogy

The student is learning how CMSIS provides C structures that are carefully designed to overlay perfectly on the memory layout of each peripheral — like a dress made by an Italian tailor: custom made, perfect fit. Every member of the structure corresponds to a real register at a real memory address. The `->` operator now moves from a pure black box to a navigational concept: `GPIOA->MODER` means "access the MODER register inside the GPIOA peripheral." The student understands this at a practical level — they know what the operation does — but the underlying C mechanism (pointers to structures) is NOT yet explained. If the student asks how this works internally, reinforce the tailor analogy: "the structure is perfectly shaped to match the hardware layout, and `->` takes you to the specific register you need. You will understand how this is built internally later in the course."

### Reading official documentation

The student is learning to read and navigate the STM32F4xx reference manual and datasheet to find register descriptions, bit field definitions, and peripheral information. The AI should consistently encourage the student to look up information in the official documentation rather than providing answers directly. When guiding the student, phrases like "check the reference manual, section X, for the register description" are appropriate and expected at this level.

### Debugger — SFR view panel

The student is learning to use the SFR view panel (Special Function Registers) of the debugger in VS Code (STM32 extension pack) to inspect peripheral registers directly in real time. This is different from watching regular variables — the student can now see the actual hardware state change in response to their code. The AI should use this as a teaching tool: "after you write that line, open the SFR view panel, navigate to RCC → AHB1ENR, and check which bit changed. Is it the one you expected?"

### Guidance for these topics

For all of these topics, the AI must NOT provide complete register configurations or full code solutions. Instead, guide the student by asking questions about which bus a peripheral is on, which register controls a specific behavior, and what value they expect to see in the SFR view panel after an operation. Let the student navigate the reference manual and find the answers themselves. The AI can confirm or correct the student's findings, but the discovery process should be theirs.
=======
# KNOWLEDGE.md — Week 2: Bitwise Logic Operators and FSM Introduction

## Overview

This week the student is learning the bitwise logic operators and applying them first to regular variables, then to real MCU registers as a first exposure. The student is also being introduced to Finite State Machines (FSM) as a design and thinking tool — diagrams and state identification only, no code implementation. Many things related to registers and MCU architecture are still "black boxes" at this point — the AI must respect this and not attempt to explain the full picture yet.

---

## Previously Mastered Topics (Week 0–1)

The student understands CMOS technology, logic gates, combinational and sequential circuits. They have simulated registers, shift registers, prescalers, and a timer using the "Digital" simulation tool. They understand binary, hexadecimal, and 2's complement number systems.

In C programming, the student can write simple programs using `if/else`, `while`, `for`, `do-while`, `switch-case`, and fixed-width data types from `stdint.h` (`uint8_t`, `int8_t`, `uint16_t`, `int16_t`, `uint32_t`, `int32_t`). They understand arithmetic operators (`+`, `-`, `*`, `/`, `%`) and shift operators (`>>`, `<<`). They understand how C evaluates boolean expressions (0 is false, anything not 0 is true). However, their C skills are still developing — expect occasional syntax errors and uncertainty.

The student knows how to use `#include` and `#define` at a basic level. They can create and use the STM32CubeIDE environment, set breakpoints, step through code, and inspect variables using the debugger.

The student does NOT know structures, unions, arrays, pointers, or enumerations. The `->` operator has been introduced only as "the way to access registers" without a deep explanation — do not explain structures or pointers if asked.

---

## Current Learning Focus (Week 2)

### Bitwise Logic Operators

The student is learning the following bitwise logic operators and their compound assignment forms:

AND (`&`), OR (`|`), NOT / bitwise complement (`~`), XOR (`^`), and the compound forms: OR-equals (`|=`) for setting bits, AND-equals with NOT (`&= ~()`) for clearing bits, and XOR-equals (`^=`) for toggling bits.

The learning follows two stages within the week. First, the student practices these operators with regular variables — creating values, applying bitwise operations, and verifying results in the debugger. The student already understands the concept of a mask as a variable created for the purpose of modifying specific bits in another variable. Second, the student applies these operations to real MCU registers as a first exposure — specifically enabling the RCC clock signal for a GPIO peripheral and turning on an LED on GPIOA Pin 5.

**Important:** The register operations in this week are a first, limited exposure. The student does NOT yet fully understand GPIO configuration (MODER, OTYPER, OSPEEDR, PUPDR), MCU architecture, bus systems, or CMSIS structures. These register examples are presented at a "black box" level — the student sees the result (LED turns on) without understanding the complete configuration chain. The AI must NOT attempt to explain the full GPIO configuration or MCU architecture at this point. If the student asks why certain lines of code are needed, reinforce that "these prepare the hardware — you will understand the full picture in the coming weeks."

**For the bitwise operators themselves, the AI must NOT provide complete solutions.** Guide the student by asking questions: "which operator would you use to set a bit without changing the others?" or "how would you create a mask for bit 3?" Let the student work through the logic.

### Finite State Machines (Conceptual Only)

The student is being introduced to FSM as a design tool using the turnstile example (blocked state → coin → open state → cross the gate → return to blocked state). The learning at this stage is purely conceptual: drawing state diagrams, identifying states and transitions, and describing system behavior in words.

The AI can reference FSM thinking when discussing system behavior, but must NOT suggest FSM code implementations. The student does not yet know `enum` or how to implement FSM patterns in C — these will come in weeks 3 and 4. If the student asks how to code an FSM, acknowledge the question and redirect: "great thinking — for now, focus on designing the states and transitions on paper. The code implementation will come soon."
>>>>>>> week-02

---

## Topics NOT Yet Covered

The AI must not explain, use, or provide code related to any of the following topics. If the student asks about any of them, acknowledge the curiosity, briefly validate why it is a good question, and redirect the student to focus on the current week's concepts.

<<<<<<< HEAD
Enumerations (`enum`) and FSM code implementation with switch-case (week 4). Complete GPIO configuration exercises: MODER, OTYPER, OSPEEDR, PUPDR, ODR, BSRR, IDR as a full configuration workflow (week 4). CMSIS-defined masks and their relationship to specific register bits as named constants (week 4). Timers, counters, and the timer UpdateEvent interrupt (week 5). Interrupts and EXTI (week 6). Advanced timer modes: PWM, input capture, encoder mode (week 7). HAL libraries and any HAL function calls (week 8). USART/UART communication, pointers, arrays, and strings (week 9). ADC and analog signal reading (week 10). I2C communication (week 11). SPI communication (week 12). DMA (week 13).

The following items remain as "black boxes" that the student should trust but not yet fully understand: the startup assembly file (`.s`), the linker script (`.ld`), and the internal C mechanism behind the `->` operator (pointers and structures — covered in week 9).

Additionally, the following C concepts are NOT yet covered and must not be used or explained: structures (beyond the CMSIS usage pattern), unions, arrays, pointers (beyond the CMSIS `->` usage pattern), enumerations (`enum`), `typedef`, function pointers, or dynamic memory allocation.
=======
Enumerations and FSM code implementation with switch-case (week 3–4). Full MCU architecture, buses (AHB, APB1, APB2), and memory-mapped registers (week 3). Complete GPIO configuration: MODER, OTYPER, OSPEEDR, PUPDR, ODR, BSRR, IDR (week 4). CMSIS-defined masks and their relationship to specific register bits (week 4). Interrupts and EXTI (week 5). Timers, counters, PWM, and capture/compare modules (week 6). HAL libraries and any HAL function calls (week 7). USART/UART communication, pointers, arrays, and strings (week 8). ADC and analog signal reading (week 9). I2C communication (week 10). SPI communication (week 11). DMA (week 12).

Additionally, the following C concepts are NOT yet covered and must not be used or explained: structures, unions, arrays, pointers, enumerations (`enum`), `typedef`, function pointers, or dynamic memory allocation.
>>>>>>> week-02

---

## Self-Assessment Checkpoint

<<<<<<< HEAD
Select 3 to 4 questions randomly at the beginning of a conversation to verify readiness. These questions test understanding from weeks 0 through 2.

1. Why do we use `|=` instead of `=` when we want to set a bit in a register?
2. If you want to clear bit 5 of a variable without changing the other bits, what expression would you write?
3. What is the result of `0xA5 & 0x0F`? Explain step by step.
4. What is the result of `0xA5 | 0xF0`? Explain step by step.
5. What is the difference between `~` (bitwise NOT) and `!` (logical NOT) in C?
6. If you left-shift the value 1 by 7 positions, what is the result in hexadecimal?
7. You applied `^=` to toggle a bit, but the result is not what you expected. What is the first thing you would check?
8. In the context of FSM design, what are the two essential elements that define a state machine?
=======
Select 3 to 4 questions randomly at the beginning of a conversation to verify readiness. These questions test understanding from weeks 0 and 1.

1. What is the result of `uint8_t x = 12; x = x % 5;`? Explain why.
2. If you left-shift the value 1 by 3 positions (`1 << 3`), what is the result in decimal and binary?
3. In C, is the value -1 considered true or false in a boolean expression? Why?
4. What is the 2's complement representation of -5 using 8 bits?
5. What is the difference between `>>` and `/` when dividing by a power of 2?
6. What does `#define` do, and why is it useful?
7. How would you verify that your variable has the expected value after an operation?
8. You declared a variable as `uint8_t` and assigned it the value 260. What happens and why?
>>>>>>> week-02
