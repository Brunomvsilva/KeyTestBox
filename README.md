# KeytestBox

> A modular embedded platform designed to integrate automotive HMI key-testing systems in production lines.

> This project was developed at Preh Portugal as part of my Master’s thesis.

---

## Introduction 

Automotive HMI, such as climate and central control panels, rely on high-quality tactile feedback. **Force–displacement** key tests quantify that feel essencially by measuring the relation between **applied force** and **key travel**.

Images below display the **main components** of a key test system

<div align="center">
<table>
  <tr>
    <td style="padding-right: 40px;">
      <img src="Images/SystemKeytest.png" width="210">
    </td>
    <td style="padding-left: 40px;">
      <img src="Images/Diagrama_SmacLoadCell.png" width="155">
    </td>
  </tr>
</table>
</div>

### During a test:
- The key is pressed by a **SMAC linear motor**  
- **Force** is measured using a **load cell with an amplifier**  
- **Key travel (displacement)** is obtained from the **SMAC internal encoder**  
- The **Unit Under Test (UUT)** sends data via **CAN/LIN** during the test, such as:  
  - **Key state (pressed / released)**
  - **Lever position or angle** (e.g. gear selector tests)  
  - **Selected gear value**  
  - **UUT Hall sensors or internal position data**  
  - Other diagnostic or status information


### Typical Key Test Output

The **Force–Displacement** curve is the most common way to characterize how a key or button behaves when it is pressed and released. It represents the force applied to the key as a function of its travel (displacement).

<p align="center">
  <img src="Images/Force-strokeCurve.png" width="300">
</p>

Curves differ depending on the type of key being tested.
During a test, the key is pressed (force increases) until it reaches a maximum point, and then released. From this curve, mechanical parameters of the key can be extracted:

| Symbol | Description |
|--------|-------------|
| **S1** | Displacement at which the key actuates (snap/collapse point). |
| **F1** | Actuation force – force required to trigger the snap of the membrane or key mechanism at S1. |
| **S2** | Displacement where electrical contact occurs with the PCB. |
| **F2** | Contact force – minimum force required to close the electrical contact at S2. |
| **F3** | Return force – force at which the electrical contact is released during the return movement. |
| **F4** | Force difference between actuation and release states. |
| **F5** | Maximum applied force during the test (should not exceed mechanical limits). |
| **Ss** | Maximum travel / end of mechanical stroke. |

One of the most important metrics derived from this curve is the **Snap Ratio (SR)**, which indicates how strong the tactile feedback is:

$$
SR=\dfrac{F_1-F_2}{F_1}\times 100\%
$$

- A higher **Snap Ratio** = stronger tactile feedback (more noticeable “click”).
- A lower **Snap Ratio** = softer feedback, which may feel smoother but less perceptible.
- Ideal **SR** values depend on the application.

---

## Incremental vs Continuous Tests

The flowcharts below provide a simplified comparison between the two testing methods.  
The main difference is that in **Incremental Tests**, the actuator stops at each step to acquire a sample, while in **Continuous Tests**, the actuator moves without stopping and data is sampled in real time.

<table align="center">
  <tr>
    <td align="center">
      <img src="Images/IncrementalTestFlowchart.png" width="275"><br>
      <em>Incremental Test Flowchart</em>
    </td>
    <td align="center">
      <img src="Images/ContinuousTestFlowchart.png" width="250"><br>
      <em>Continuous Test Flowchart</em>
    </td>
  </tr>
</table>

---

## Incremental Tests

### Incremental Test System Example (without the KeyTestBox)

<p align="center">
  <img src="Images/sistema_atual.png" alt="Current System Architecture" width="550">
</p>

- **Force** data is acquired by the host PC via a USB Data Acquisition (DAQ) device from National Instruments.
- **Position** data is acquired by RS232 from the actuator controller
- **Key State or info** data is acquired through a CAN/LIN-USB converter

### Advantages:
- Low cost
- Easier to implement

### Disadvantages
- **low test resolution** (low sample density)
- May miss the sampling of the **snap point**, especially on **short-travel keys**
- Can fail to meet client requirements

### Incremental Test Output Example

<p align="center">
  <img src="Images/IncTestGraph.png" alt="Inc Test Graph" width="480">
</p>


---

## Continuous Tests

### Continuous Test System (Without KeyTestBox)

<p align="center">
  <img src="Images/sistema_cDAQ-1.png" alt="Continuous Test System" width="550">
</p>

In continuous testing, the actuator **moves without stopping**, and all signals are sampled in real time. This enables a smoother force–displacement curve and higher data accuracy.

- **Force** is measured by a load cell and continuously acquired via the DAQ system  
- **Position (travel)** is obtained from the SMAC actuator encoder in real time  
- The **Unit Under Test (UUT)** transmits its state (CAN/LIN), such as key/lever status or sensor values, during movement  
- All data is timestamped and saved by the DAQ system for later processing  

> While this setup delivers high accuracy and resolution, it uses sophisticated hardware and is **significantly more expensive**.

### Advantages
- **High sample density** → more precise force–travel curve  
- **Accurate snap detection**  
- **Faster test execution** → important in industrial production environments  

### Disadvantages
- **Substantially higher cost** due to DAQ modules and synchronization hardware.

### Continuous Test Output Example

<p align="center">
  <img src="Images/TesteContinuoGrafico.png" alt="Continuous Test Graph" width="450">
</p>

---

## Problem Statement

In industrial test setups, especially for testing mechanical keys, the most common approach is the **incremental test method**. This approach is simple and cost-effective, and it can work well in many cases.

However, **in several real-world situations, incremental testing is not sufficient or may not work at all**. These limitations arise due to the way the test equipment communicates and the type of measurements required:

---

### Problem 1: Delay when reading UUT data via CAN or LIN

Many devices under test (UUTs) only report their internal state (e.g., whether a button is pressed or not) when the host explicitly **requests the data** using a communication protocol like **CAN** or **LIN**. This process introduces a delay between the request and the response.

If the actuator is moving during this time (as in a continuous test), by the time the host receives the data, the key is already in a **different position**, and the measured force has also changed. This causes a mismatch between the **key state**, **position**, and **force**, making the data unreliable.

<p align="center">
  <img src="Images/SmacSampleDelay-1.png" alt="Sampling delay during continuous motion (request/response CAN/LIN)" width="470">
</p>

---

### Problem 2: Low resolution and missed events in incremental tests

Given the problem 1, a good solution would be to **stop the actuator** and only then **request the data via CAN or LIN**, then wait for the response.  
However, as said before, **incremental testing has limited resolution**. Since the actuator only samples at discrete steps, fast or short events — such as the **snap point** (a sudden change in force when pressing a key) — may occur between two sample points and go undetected.

This is particularly problematic for **short-travel keys**, where the full keypress happens in a very small movement range. In such cases, a small number of samples is not enough to capture the key’s behavior accurately.

---

### Consequence: Forced to use expensive continuous test systems

To solve these problems, test stations often switch to **continuous testing**, which are significantly more expensive

---

## Solution, Main Goals and Requirements

To overcome the limitations of incremental testing and avoid the high cost of commercial continuous testing systems, a new solution is required — one that enables **real-time, synchronized testing** without the complexity and price of traditional DAQ systems.

The **KeyTestBox** is proposed as a **low-cost**, **modular**, and **reusable** platform suitable for production testing environments.

### Main Goals 

- **Real-time acquisition** of:
  - **Force** (via load cell)
  - **Position** (via SMAC encoder)
  - **UUT data** (via CAN or LIN protocols)

### Hardware Requirements

- 24 VDC power supply  
- CAN interface (supports baud rate up to 1 Mbps)  
- LIN interface (supports baud rate up to 19 200 bps)  
- Ethernet port (10/100 Mbps)  
- Two RS232 interfaces  
- Minimum of five differential analog inputs (−10 V to +10 V) **(Mainly to read the 2 Load Cells but some extra were added)**  
- Two linear encoder counters  
- Microcontroller-based system
- PCB design

---

### Software Requirements

- Start and stop a test routine upon request  
- Ability to export the acquired test data  
- Capability to execute external commands during the test routine  
- Use of the MQTT (Message Queuing Telemetry Transport) protocol for communication with the host  
- Firmware developed in C


---

## Hardware Overview

<p align="center">
  <img src="Images/HardwareDesignOverview.png" alt="overviewHardware" width="600">
</p>

---

## Final Hardware

[Full **Hardware Schematic**](PCB/keytestbox_schematic.pdf)

### Main Components

- STM32F767VIT6
- LAN8742A
- 2x MAX1032 (ADC)
- MCP2021 (LIN)
- MCP2551 (CAN)
- MAX232 (RS232 Converter) 
- STLINK-V3 Debugger SWD
 
<table align="center">
  <tr>
    <td align="center">
      <img src="Images/PCB photo.png" alt="Real PCB Photo" width="300"><br>
      <em>Real KeytestBox Image</em>
    </td>
    <td align="center">
      <img src="Images/KeytestBox3dModel.png" alt="3D Model of KeyTestBox" width="300"><br>
      <em>3D model of the KeyTestBox enclosure</em>
    </td>
  </tr>
</table>


---

## Software and Tools used

- STM32Cube IDE
- FreeRTOS
- LwIP IP Stack
- Mosquitto MQTT
- Percepio Tracealyzer

---

## MQTT Commands Implemented

<p align="center">
  <img src="Images/KeyTestBox_Commands_EN.png" alt="MQTT Commands" width="800">
</p>

---

## Tests and Results

Main tests were done to a lever from the UUT shown below, which is one of the most demanding tests for the following reasons:
  - Lever has a long travel distance, arround 12 mm, while buttons are only arround 3 mm 
  - Has **two snap points** per test (2 going forward and 2 going backwards)
  - Lots of CAN data to receive from UUT 

### UUT Lever tested (highlighted in red) and Testbench setup

<p align="center">
  <img src="Images/Screenshot%20from%202025-11-0804-06-26.png" alt="TB setup" width="800">
</p>

Images below show some result graphs
