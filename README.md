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

**During a test:**
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

## Incremental Tests

**Advantages:**
- Low cost
- Easier to implement

**Disadvantages**
- **low test resolution** (low sample density)
- May miss the sampling of the **snap point**, especially on **short-travel keys**
- Can fail to meet client requirements

### Incremental Test Output Example

<p align="center">
  <img src="Images/IncTestGraph.png" alt="Inc Test Graph" width="480">
</p>


### Incremental Key Test System Example (without the KeyTestBox)

<p align="center">
  <img src="Images/sistema_atual.png" alt="Current System Architecture" width="550">
</p>

- **Force** data is acquired by the host PC via a USB Data Acquisition (DAQ) device from National Instruments.
- **Position** data is acquired by RS232 from the actuator controller
- **Key State or info** data is acquired through a CAN/LIN-USB converter

## Continuous Tests

Unlike incremental tests, continuous testing does **not stop the actuator during data acquisition**. The actuator moves at a constant speed, while force, position and UUT state are sampled in real time.

### Advantages:
- **High resolution** (more samples along the key travel)
- **Better detection of snap point and curve shape**
- **Faster test cycle time** → important for production lines

### Disadvantages:
- **Significantly Higher cost**

---

### Continuous Test Output Example

<p align="center">
  <img src="Images/TesteContinuoGrafico.png" alt="Continuous Test Graph" width="300">
</p>

---

### Continuous Key Test System Example (without the KeyTestBox)

<p align="center">
  <img src="Images/sistema_cDAQ.pdf" alt="Continuous Test System" width="250">
</p>

- **Force** is acquired by a load cell → USB DAQ  
- **Position** is provided continuously by the SMAC actuator encoder  
- The **UUT sends data via CAN or LIN** during movement  
- Data is timestamped in the PC (Host) and later combined to generate the full force–displacement curve

> This system provides high accuracy but is expensive and not easily reusable between different test stations.

---

### Problem in Continuous Testing (Without Synchronization)

In systems that use **request/response CAN or LIN messages**, key data is not sent continuously.  
If the actuator is moving and the host requests UUT data, the response arrives **delayed**, causing force and position to no longer correspond to the real key state at that moment.

<p align="center">
  <img src="Images/ContTestSamplingProblem.png" width="450"><br>
  <em>Sampling delay problem during continuous actuator movement</em>
</p>

---

### Why KeyTestBox?

The goal of the **KeyTestBox** is to provide a **low-cost, modular and synchronized solution** for continuous testing:
- Real-time synchronization between **Force + Position + UUT data**
- Works with **CAN / LIN / analog sensors**
- Replaces expensive cDAQ systems
- Designed to be easily integrated into any test station



---

## Problem Statement

- **Incremental tests** are cheaper but can cause some problems or may not meet some usual requirements, such as:
   - **miss snap events** on **short-travel keys** due to low sample density.
   - 
- **Continuous tests** (sample while moving) give **higher resolution** and better results but current solutions are **significantly more expensive**.  

**Primary improvement targeted:** deliver **continuous-quality data at much lower cost**.

---

## Objectives

- Design and build **hardware + firmware** for a unified test box (KeytestBox).  
- Acquire **force**, **position**, and **key electrical status (CAN/LIN)**.  
- Stream results via **Ethernet** (host command interface + data output).  
- Provide a **reusable command-based workflow** instead of per-station code edits. :contentReference[oaicite:5]{index=5}

---

## What KeytestBox Provides

- **Continuous or incremental testing** with consistent data packaging. :contentReference[oaicite:6]{index=6}  
- **Low-cost embedded acquisition** (load cell + encoder + CAN/LIN) vs. high-end lab DAQ. :contentReference[oaicite:7]{index=7}  
- **Ethernet control & data** so the host only speaks commands (no station-specific rewrites). :contentReference[oaicite:8]{index=8}

---

## Images (add your files under `/Images`)
> Replace the placeholders below with your actual image files.

<p align="center">
  <img src="Images/system_overview.png" alt="System overview: actuator, load cell, encoder, CAN/LIN, Ethernet" width="720">
</p>

<p align="center">
  <img src="Images/keytestbox_pcb_top.png" alt="KeytestBox PCB – top view" width="720">
</p>

<p align="center">
  <img src="Images/test_setup_example.jpg" alt="Bench setup with DUT and linear actuator" width="720">
</p>

---

## Repository Pointers


