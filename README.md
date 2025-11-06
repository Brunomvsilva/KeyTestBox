# KeytestBox

> A modular embedded platform designed to integrate automotive HMI key-testing systems in production lines.

> This project was developed at Preh Portugal as part of a Master’s thesis.

---

## Introduction 

Automotive HMI (climate and central console panels) relies on high-quality tactile feedback for safety and user experience. **Force–displacement** key tests quantify that feel essencially by measuring the relation between **applied force** and **key travel**.

Images below display the main components of the system

<div align="center">

<table bgcolor="#e6f2ff"> <!-- light blue -->
  <tr>
    <td><img src="Images/SystemKeytest.png" width="200"></td>
    <td><img src="Images/Diagrama_SmacLoadCell.png" width="200"></td>
  </tr>
</table>

</div>





- A Key is **pressed** using a SMAC Motor
- **Force** is measured using a Load Cell and an Amplifier
- **Key travel** is given by the SMAC encoder
- **The Unit Under Test** sends data during the test via **CAN/LIN**, such as:
    - If the Key is pressed (On/Off)
    - 


### Incremental vs Continuous Tests

**Incremental Tests**

The actuator moves in **small fixed steps** and stops at each position to sample values.

**Characteristics:**
- Low cost, simple to implement  
- Data only acquired when the actuator is stopped → **low resolution**  
- May miss the **snap point**, especially on **short-travel keys**  

**Data acquisition in current systems:**
- **Force** → USB DAQ (National Instruments) connected to the host PC  
- **Position** → Requested via RS232 command (TP – Tell Position) from the actuator controller  
- **Key state / electrical info** → Received via CAN/LIN to USB converter  


### Systems without the KeyTestBox

<p align="center">
  <img src="Images/sistema_atual.png" alt="Current System Architecture" width="650">
</p>

**This system is usually used for Incremental Tests** 

- **Force** data is acquired by the host PC via a USB Data Acquisition (DAQ) device from National Instruments.
- **Position** data is acquired by RS232 from the actuator controller
- **Key State or info** data is acquired through a CAN/LIN-USB converter

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


