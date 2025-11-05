# KeytestBox — Automotive Key Testing System

> A modular, embedded “black-box” platform to acquire **force–displacement** curves and **electrical key status** (CAN/LIN) for automotive HMI panels, while **cutting the cost** of continuous-quality testing.

---

## Introduction (Context)

Automotive HMI (climate and central console panels) relies on high-quality tactile feedback for safety and user experience. Force–displacement key tests quantify that feel by measuring the relation between **applied force** and **key travel**, along with on/off electrical state. Preh Portugal engineers these tests for multiple OEMs and models on production lines.

Key tactile quality is commonly assessed with a **linear actuator** (press/release), a **load cell** (force), and an **encoder** (position), optionally complemented by **CAN/LIN** messages from the device under test. Data can be acquired directly by a host PC or by an intermediate embedded node and forwarded over **Ethernet**. :contentReference[oaicite:1]{index=1}

---

## Problem Statement

- **Incremental tests** (pause & sample) are cheaper but often **miss snap events** on **short-travel keys** due to low sample density.  
- **Continuous tests** (sample while moving) give **higher resolution** and better results but current solutions are **significantly more expensive** and harder to scale across stations.  
- Per-station **software adaptation** remains manual and time-consuming for product-specific setups.

**Primary improvement targeted:** deliver **continuous-quality data at much lower cost**, with a uniform Ethernet command interface.

---

## Motivation

Standardizing and encapsulating the test logic reduces engineering effort, accelerates delivery, and enables broader deployment of continuous-quality testing across lines—**improving throughput and lowering cycle time** without sacrificing accuracy.

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


